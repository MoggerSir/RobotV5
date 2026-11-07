// PROGRAMA PRINCIPAL: conecta el mando, los calculos y los seis motores del robot.
// Recorrido de cada ciclo: leer mandos -> comprobar seguridad -> calcular -> mandar motores -> mostrar estado.
// Para empezar a conducir: ejecutar RobotV5 y pulsar B con los joysticks centrados. X frena y bloquea.
// include trae herramientas definidas en otros archivos. vex.h da acceso al hardware VEX;
// robot-config.h contiene ajustes; los otros dos archivos calculan movimiento y protecciones.
#include "vex.h"
#include "robot-config.h"
#include "drive-logic.h"
#include "mechanism-logic.h"

// Permite escribir motor o brain en vez de vex::motor o vex::brain.
using namespace vex;
// Representa la caja con pantalla del robot. Lo usamos para mostrar instrucciones y diagnostico.
brain Brain;
// Representa el mando principal conectado al Brain; no crea una conexion nueva por si solo.
controller Remote(primary);
// Cada objeto motor representa un puerto fisico. Recibe puerto, cartucho e inversion de sentido.
// Los valores concretos se cambian en include/robot-config.h, no aqui.
motor FrontLeft(config::frontLeftPort, config::wheelGear, config::frontLeftReversed);
motor RearLeft(config::rearLeftPort, config::wheelGear, config::rearLeftReversed);
motor FrontRight(config::frontRightPort, config::wheelGear, config::frontRightReversed);
motor RearRight(config::rearRightPort, config::wheelGear, config::rearRightReversed);
motor ArmMotor(config::armPort, config::armGear, config::armReversed);
motor ClawMotor(config::clawPort, config::clawGear, config::clawReversed);

// Control de las cuatro ruedas. Agrupa las dos izquierdas y las dos derechas.
// public indica funciones que otras partes del programa pueden usar.
class DriveTrain {
 public:
  // Constructor: al crear DriveTrain, forma las dos parejas con los motores existentes.
  DriveTrain() : left_(FrontLeft, RearLeft), right_(FrontRight, RearRight) {}
  // Recibe las velocidades ya calculadas y las manda a las dos parejas.
  void apply(const driving::Output& output) {
    command(left_, output.left);
    command(right_, output.right);
  }
  // brake intenta frenar las ruedas. No es un corte de bateria ni elimina la distancia fisica de frenado.
  void stop() { left_.stop(brake); right_.stop(brake); }
  // Devuelve true solo si detecta las cuatro ruedas. && significa que todas las condiciones deben cumplirse.
  bool installed() {
    return FrontLeft.installed() && RearLeft.installed() &&
           FrontRight.installed() && RearRight.installed();
  }
 private:
  // Esta ayuda interna no necesita datos propios de un DriveTrain.
  // Una orden casi cero frena; las demas hacen girar a la velocidad porcentual indicada.
  // Una velocidad negativa invierte el sentido respecto a forward.
  static void command(motor_group& group, double speed) {
    if (driving::magnitude(speed) < 0.01) group.stop(brake);
    else group.spin(forward, speed, percent);
  }
  motor_group left_;
  motor_group right_;
};

// Adaptador entre un motor fisico y la proteccion del brazo o de la pinza.
// Reutilizamos esta clase porque ambos necesitan las mismas comprobaciones.
class MotorMechanism {
 public:
  // motor& es una referencia: usa el motor existente, sin crear otro en el mismo puerto.
  // Cada objeto construye su propia proteccion con su tiempo maximo.
  MotorMechanism(motor& device, int maxRunMs) : device_(device),
      protection_(maxRunMs, config::stallMs, config::tickMs, config::maxTemperatureC) {}
  // Bloquea el mecanismo y lo detiene. hold intenta mantener la posicion usando corriente;
  // si esta caliente usa brake para evitar la retencion activa.
  void disable() {
    protection_.disable();
    device_.stop(device_.temperature(celsius) >= config::maxTemperatureC ? brake : hold);
  }
  // Convierte el estado de proteccion en un texto corto para la pantalla.
  // const char* apunta a un texto fijo. switch elige el texto segun el estado.
  const char* status() const {
    switch (protection_.state()) {
      case mechanism::State::ready: return "LISTA";
      case mechanism::State::releaseRequired: return "SOLTAR BOTONES";
      case mechanism::State::timedOut: return "LIMITE TIEMPO";
      case mechanism::State::stalled: return "ATASCO";
      case mechanism::State::hot: return "TEMPERATURA";
    }
    return "DESCONOCIDO";
  }
  // Lee velocidad y temperatura reales, consulta la proteccion y aplica la orden permitida.
  void update(double command) {
    const mechanism::Output output = protection_.update(
        command, device_.velocity(percent), device_.temperature(celsius));
    // Cero detiene el mecanismo; si no esta caliente, mantiene su posicion.
    // Una orden distinta de cero mueve en el sentido que indique su signo.
    if (output.speed == 0) device_.stop(output.hot ? brake : hold);
    else device_.spin(forward, output.speed, percent);
  }
 private:
  // Guarda una referencia al motor correspondiente: brazo o pinza, nunca ambos a la vez.
  motor& device_;
  mechanism::Protection protection_;
};

// Punto de entrada: el Brain empieza a ejecutar aqui despues de crear los objetos globales.
int main() {
  // Crea las parejas de ruedas, los dos mecanismos protegidos y el calculador de conduccion.
  DriveTrain wheels;
  MotorMechanism arm(ArmMotor, config::armMaxRunMs);
  MotorMechanism claw(ClawMotor, config::clawMaxRunMs);
  driving::Controller driver;
  // Limita la fuerza de giro de los mecanismos. El cartucho y la mecanica tambien determinan la fuerza real.
  ArmMotor.setMaxTorque(config::armTorque, percent);
  ClawMotor.setMaxTorque(config::clawTorque, percent);
  // bool es un valor de si/no. true al arrancar significa que el robot queda bloqueado hasta pulsar B.
  bool stopped = true;
  // Cuenta ciclos para actualizar la pantalla menos veces y dedicar mas tiempo al control.
  int statusTicks = 0;
  // Escribe las instrucciones una vez. Los dos primeros numeros son posiciones horizontal y vertical en pantalla.
  Brain.Screen.printAt(10, 30, "Izq: conducir | R2: sprint | A: lento");
  Brain.Screen.printAt(10, 55, "Der vertical: brazo | R1/L1: garra");
  Brain.Screen.printAt(10, 80, "X: freno/bloqueo | B: rearmar en reposo");

  // Repite este bloque mientras el programa siga en ejecucion.
  while (true) {
    // Lecturas -100...100: Axis3 = joystick izquierdo vertical; Axis4 = izquierdo horizontal;
    // Axis2 = derecho vertical. const indica que no cambiamos estas lecturas dentro del ciclo.
    const int throttle = Remote.Axis3.position(percent);
    const int steering = Remote.Axis4.position(percent);
    const int armAxis = Remote.Axis2.position(percent);
    // pressing() devuelve true mientras mantienes pulsado el boton. R2 es sprint, R1 abre y L1 cierra.
    const bool sprint = Remote.ButtonR2.pressing();
    const bool open = Remote.ButtonR1.pressing();
    const bool close = Remote.ButtonL1.pressing();
    // Comprueba conexion del mando y presencia de cada motor; son condiciones distintas.
    const bool connected = Remote.installed();
    const bool frontLeftReady = FrontLeft.installed();
    const bool rearLeftReady = RearLeft.installed();
    const bool frontRightReady = FrontRight.installed();
    const bool rearRightReady = RearRight.installed();
    const bool armReady = ArmMotor.installed();
    const bool clawReady = ClawMotor.installed();
    // Solo permite conducir si estan presentes los seis motores. Un fallo de pinza tambien bloquea las ruedas.
    const bool hardwareReady = frontLeftReady && rearLeftReady && frontRightReady &&
                               rearRightReady && armReady && clawReady;
    // Para rearmar, los ejes deben estar dentro de la zona central y R1/L1/R2 sueltos.
    // abs calcula valor absoluto; ! significa no. A no participa en esta comprobacion.
    const bool neutral = abs(throttle) <= 7 && abs(steering) <= 7 && abs(armAxis) <= 7 &&
                         !open && !close && !sprint;
    // Perder conexion, perder un motor o pulsar X activa el bloqueo.
    // El else if hace que esa orden de parada tenga prioridad aunque B tambien este pulsado.
    if (!connected || !hardwareReady || Remote.ButtonX.pressing()) stopped = true;
    else if (stopped && Remote.ButtonB.pressing() && neutral) stopped = false;

    // Con bloqueo: borra la rampa, frena ruedas y detiene brazo/pinza.
    // Conduccion normal solo se ejecuta en la parte else.
    if (stopped) {
      driver.reset();
      wheels.stop();
      arm.disable();
      claw.disable();
    } else {
      // Entrega joystick izquierdo y botones al calculador. El resultado mueve las ruedas.
      wheels.apply(driver.update({throttle, steering, sprint, Remote.ButtonA.pressing()}));
      // Convierte el joystick derecho a una orden proporcional para el brazo.
      // Las ordenes menores que 5 % se toman como cero para evitar movimientos muy pequenos.
      double armCommand = driving::axis(armAxis) * config::armSpeed;
      if (driving::magnitude(armCommand) < 5) armCommand = 0;
      arm.update(armCommand);
      // Si R1 y L1 estan ambos pulsados o ambos sueltos, manda cero.
      // Si solo R1 esta pulsado, abre; si solo L1 esta pulsado, cierra.
      claw.update(open == close ? 0 : open ? config::clawSpeed : -config::clawSpeed);
    }

    // Hasta 50 Hz de control; pantalla una vez cada diez ciclos.
    // Una de cada diez vueltas dibuja el diagnostico. printAt sustituye el texto en esas coordenadas.
    // %-38s reserva espacio para un texto; %d muestra enteros; %4d alinea valores; %2.0f muestra temperatura sin decimales.
    // Los espacios al final borran restos de mensajes anteriores mas largos.
    if (statusTicks == 0) {
      Brain.Screen.printAt(10, 115, true, "%-38s", !connected ? "SIN CONTROL" :
          !hardwareReady ? "FALTA MOTOR: revisa puertos" :
          stopped ? "FRENADO: B con mandos en reposo" : "LISTO PARA MANEJAR");
      // J3/J4/J2 son los valores recibidos del mando. B/X valen 1 si estan pulsados y 0 si no.
      Brain.Screen.printAt(10, 140, "J3:%4d J4:%4d J2:%4d B:%d X:%d    ",
          throttle, steering, armAxis, int(Remote.ButtonB.pressing()), int(Remote.ButtonX.pressing()));
      // Cada numero antes de : es un puerto. Un 1 despues significa motor detectado; un 0, ausente.
      Brain.Screen.printAt(10, 165, "8:%d 9:%d 3:%d 4:%d 10:%d 5:%d    ",
          int(frontLeftReady), int(rearLeftReady), int(frontRightReady),
          int(rearRightReady), int(armReady), int(clawReady));
      // Explica si los mandos permiten rearmar. No indica por si solo que el robot ya este habilitado.
      Brain.Screen.printAt(10, 190, true, "%-38s", neutral
          ? "Mandos centrados: B puede rearmar" : "Para rearmar suelta todos los mandos");
      // La linea PINZA muestra R1/L1, temperatura y estado de proteccion para investigar por que no se mueve.
      Brain.Screen.printAt(10, 215, "PINZA R1:%d L1:%d T:%2.0fC %-14s    ",
          int(open), int(close), ClawMotor.temperature(celsius), claw.status());
    }
    // % calcula el resto de una division: el contador vuelve a cero cada diez vueltas.
    // wait deja tiempo a otras tareas del Brain. Con 20 ms de espera, el ciclo llega como maximo a unas 50 vueltas por segundo.
    statusTicks = (statusTicks + 1) % 10;
    wait(config::tickMs, msec);
  }
}
