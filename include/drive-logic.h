// CALCULOS PARA CONDUCIR: convierte posiciones del joystick en velocidades de ruedas.
// Este archivo solo hace cuentas. No enciende motores y puede probarse en la computadora.
#pragma once

// Agrupa los calculos de conduccion para no confundirlos con otras partes del programa.
namespace driving {
// struct es un paquete de datos. Estos son los datos que entrega el mando en cada ciclo.
// throttle: adelante/atras (-100 a 100). steering: izquierda/derecha (-100 a 100).
// sprint: R2 pulsado. precision: A pulsado para ir lento.
struct Input {
  int throttle;
  int steering;
  bool sprint;
  bool precision;
};
// Resultado: porcentajes para las ruedas izquierdas (left) y derechas (right).
// Un numero negativo pide girar el motor en el sentido opuesto.
struct Output { double left; double right; };
// Valor absoluto: devuelve el tamano del numero, sin su signo.
// double permite usar decimales; inline permite al compilador integrar funciones pequenas.
inline double magnitude(double value) { return value < 0 ? -value : value; }
// Acerca el valor actual al objetivo sin saltar mas de step en un ciclo.
// Por ejemplo: de 0 hacia 60, con paso 3, primero devuelve 3 y luego 6.
// Asi la aceleracion y el giro cambian de forma gradual.
inline double approach(double current, double target, double step) {
  if (current < target) return target - current < step ? target : current + step;
  return current - target < step ? target : current - step;
}
// Limpia y suaviza un eje del joystick. Primero limita la lectura a -100...100.
inline double axis(int value) {
  if (value > 100) value = 100;
  if (value < -100) value = -100;
  // Zona muerta: pequenas lecturas alrededor del centro se toman como cero para evitar movimientos involuntarios.
  if (magnitude(value) <= 7) return 0;
  // Quita esa zona muerta y convierte el resto del recorrido a -1...1.
  // 93 sale de 100 - 7: el recorrido util despues de descontar el centro.
  const double normalized = (value > 0 ? value - 7 : value + 7) / 93.0;
  // Combina una respuesta recta con una curva: control fino cerca del centro y recorrido completo al extremo.
  return 0.4 * normalized + 0.6 * normalized * normalized * normalized;
}

// Una actualizacion por ciclo: O(1), sin asignaciones dinamicas.
// Esta clase recuerda la velocidad y el giro del ciclo anterior.
// Es un calculador de manejo; no es el objeto que representa el control VEX.
class Controller {
 public:
  // Borra las ordenes anteriores al frenar o perder la conexion. No mide la velocidad fisica del robot.
  void reset() { speed_ = 0; turn_ = 0; }
  // const Input& permite consultar los datos de entrada sin copiarlos ni modificarlos.
  Output update(const Input& input) {
    const double throttle = axis(input.throttle);
    // El signo ? elige entre dos valores: condicion ? si_se_cumple : si_no.
    // A tiene prioridad: 35 %. Reversa: 45 %. R2 con avance: 100 %. Normal: 60 %.
    const double limit = input.precision ? 35 : throttle < 0 ? 45 :
                         input.sprint && throttle > 0 ? 100 : 60;
    // La inclinacion del joystick multiplica ese limite: medio recorrido pide menos velocidad.
    double target = throttle * limit;
    // Si actual y objetivo tienen signos distintos, se esta pidiendo cambiar de marcha.
    // La orden pasa por cero antes de invertir. Esto no garantiza que las ruedas fisicas ya se hayan detenido.
    const bool changingDirection = speed_ * target < 0;
    if (changingDirection) target = 0; // La rampa pasa por cero al invertir marcha.
    // Paso de la rampa: 8 al invertir marcha, 6 al reducir velocidad, 3 al acelerar.
    // Son puntos porcentuales por ciclo; no son segundos ni metros por segundo.
    const double rate = changingDirection ? 8 :
                        magnitude(target) < magnitude(speed_) ? 6 : 3;
    speed_ = approach(speed_, target, rate);
    // Permite girar con el joystick horizontal aun con el robot parado.
    // Parado permite girar sobre su sitio con limite 35 %. En marcha usa el 65 % del limite de conduccion.
    const double turnLimit = throttle == 0 ? 35 : limit * 0.65;
    turn_ = approach(turn_, axis(input.steering) * turnLimit, 6);
    // Para girar a la derecha se suma giro al lado izquierdo y se resta al derecho.
    // Si no hay avance, un lado puede girar hacia adelante y el otro hacia atras.
    Output result{speed_ + turn_, speed_ - turn_};
    // Busca la mayor de las dos ordenes. Si excede el limite, reduce ambas por el mismo factor.
    // Asi conserva la proporcion del giro y evita pedir mas potencia de la permitida.
    double peak = magnitude(result.left);
    if (magnitude(result.right) > peak) peak = magnitude(result.right);
    if (peak > limit) { result.left *= limit / peak; result.right *= limit / peak; }
    return result;
  }
 // Los datos privados solo los cambia esta clase. El guion bajo identifica valores recordados entre ciclos.
 private:
  // Orden longitudinal guardada; no es una lectura de los sensores de velocidad.
  double speed_ = 0;
  // Orden de giro guardada para que la direccion tambien cambie suavemente.
  double turn_ = 0;
};
}
