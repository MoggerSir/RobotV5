// AJUSTES DEL ROBOT: empieza por este archivo para revisar el cableado.
// Los numeros de puerto deben coincidir con los conectores del Brain.
// Cambiar un ajuste requiere compilar y volver a cargar el programa.
// Evita que estos ajustes se incluyan dos veces al compilar.
#pragma once
#include "vex.h"

// Agrupa los ajustes bajo el nombre config. Por ejemplo, config::armSpeed.
// const y constexpr indican valores fijos: el programa no los cambia al conducir.
namespace config {
// Puertos confirmados: drivetrain 8,9,3,4; brazo 10; pinza 5.
// Agrupacion de ruedas por verificar: 8/9 izquierda, 3/4 derecha.
// Ruedas izquierdas: front = delantera; rear = trasera. PORT8 es el puerto fisico 8.
const int frontLeftPort = vex::PORT8;
const int rearLeftPort = vex::PORT9;
// Ruedas derechas. Cada pareja recibe la misma orden de velocidad.
const int frontRightPort = vex::PORT3;
const int rearRightPort = vex::PORT4;
// El brazo levanta y baja la pinza. La pinza tiene su propio motor para abrir/cerrar.
const int armPort = vex::PORT10;
const int clawPort = vex::PORT5;
// El cartucho es la pieza de engranajes del motor. Este ajuste describe el instalado;
// no cambia sus engranajes fisicos. ratio18_1 es verde; ratio36_1 es rojo.
constexpr vex::gearSetting wheelGear = vex::gearSetting::ratio18_1; // Verde confirmado
constexpr vex::gearSetting clawGear = vex::gearSetting::ratio36_1; // Rojo confirmado
constexpr vex::gearSetting armGear = vex::gearSetting::ratio36_1; // Rojo supuesto: verificar
// true invierte el sentido de ese motor; false lo deja normal.
// Los lados suelen estar montados en espejo. Si una rueda empuja al reves, revisa su ajuste.
constexpr bool frontLeftReversed = false;
constexpr bool rearLeftReversed = false;
constexpr bool frontRightReversed = true;
constexpr bool rearRightReversed = true;
// Este brazo se invirtio tras la prueba: joystick derecho arriba debe subirlo.
constexpr bool armReversed = true; // Corregido: joystick arriba debe SUBIR
constexpr bool clawReversed = false; // forward debe ABRIR
// Una vuelta del ciclo espera 20 milisegundos. 1000 ms equivalen a un segundo.
constexpr int tickMs = 20;
// Limites de velocidad en porcentaje. El brazo puede ir mas despacio segun el joystick.
constexpr int armSpeed = 35;
// Torque es fuerza de giro. Este porcentaje limita el motor; no mide la fuerza real de la pinza.
constexpr int armTorque = 70;
// La pinza usa velocidad fija mientras mantienes R1 o L1.
constexpr int clawSpeed = 30;
constexpr int clawTorque = 70;
// Tiempo maximo de movimiento continuo antes de exigir soltar el mando.
// Son topes de tiempo, no posiciones: el robot no sabe donde estan los limites mecanicos.
constexpr int armMaxRunMs = 4000;
constexpr int clawMaxRunMs = 1200;
// Si se ordena mover un mecanismo pero casi no gira durante este tiempo, se bloquea.
constexpr int stallMs = 400;
// A esta temperatura en grados Celsius se bloquea el mecanismo hasta enfriar y soltar el mando.
constexpr double maxTemperatureC = 55.0;
}
