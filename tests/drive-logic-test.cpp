// PRUEBAS EN LA COMPUTADORA: comprueban cuentas y protecciones sin conectar el robot.
// assert comprueba una condicion: si es falsa, la prueba se detiene y muestra el error.
// Pasar estas pruebas no verifica cableado, sentidos de montaje ni fuerza real de la pinza.
#include "drive-logic.h"
#include "mechanism-logic.h"
// Herramientas para comprobar condiciones, usar calculos numericos y generar enteros de tamano conocido.
#include <cassert>
#include <cmath>
#include <cstdint>

// Abrevia los nombres para no escribir driving:: delante en cada prueba.
using driving::Controller;
using driving::Input;
using driving::Output;

// Simula cien vueltas con la misma entrada para que las rampas lleguen a su objetivo.
// Devuelve la ultima orden de ruedas; no mueve motores fisicos.
Output settle(Controller& driver, Input input) {
  Output result{};
  for (int i = 0; i < 100; ++i) result = driver.update(input);
  return result;
}

// Al ejecutar el archivo de pruebas se entra aqui. No es el main que se carga al Brain.
int main() {
  // Comprueba centro, extremos, lecturas fuera de rango y simetria: izquierda/derecha deben responder igual.
  assert(driving::axis(7) == 0 && driving::axis(-7) == 0);
  assert(driving::axis(100) == 1 && driving::axis(-100) == -1);
  assert(driving::axis(999) == 1 && driving::axis(-999) == -1);
  for (int i = 0; i <= 100; ++i) assert(driving::axis(i) == -driving::axis(-i));

  // Prueba las velocidades normales y los botones: avance, sprint, reversa y prioridad del modo lento.
  // Los cuatro datos de cada entrada van en orden: avance, giro, sprint, lento.
  Controller driver;
  Output out = settle(driver, {100, 0, false, false});
  assert(out.left == 60 && out.right == 60);
  out = settle(driver, {100, 0, true, false});
  assert(out.left == 100 && out.right == 100); // Sprint hacia adelante.
  out = settle(driver, {-100, 0, true, false});
  assert(out.left == -45 && out.right == -45); // R2 no potencia la reversa.
  out = settle(driver, {100, 0, true, true});
  assert(out.left == 35 && out.right == 35); // A domina al sprint.
  out = settle(driver, {0, 0, true, false});
  assert(out.left == 0 && out.right == 0); // R2 solo no acelera.
  out = settle(driver, {0, 100, false, false});
  assert(out.left == 35 && out.right == -35); // Giro en reposo.
  out = settle(driver, {0, -100, false, false});
  assert(out.left == -35 && out.right == 35);

  // Comprueba que el primer paso de aceleracion sea gradual y que invertir la marcha pase por cero.
  driver.reset();
  out = driver.update({100, 0, false, false});
  assert(out.left == 3 && out.right == 3); // Aceleracion gradual.
  settle(driver, {100, 0, false, false});
  bool passedZero = false;
  for (int i = 0; i < 100; ++i) {
    out = driver.update({-100, 0, false, false});
    if (out.left == 0) passedZero = true;
    if (out.left < 0) assert(passedZero);
  }
  driver.reset();
  out = driver.update({0, 0, false, false});
  assert(out.left == 0 && out.right == 0); // Parada borra inercia logica.

  // Dos calculadores reciben giros contrarios. Sus velocidades de izquierda y derecha deben intercambiarse.
  Controller mirror;
  driver.reset();
  for (int i = 0; i < 100; ++i) {
    out = driver.update({100, 65, true, false});
    Output other = mirror.update({100, -65, true, false});
    assert(std::abs(out.left - other.right) < 1e-9);
    assert(std::abs(out.right - other.left) < 1e-9);
  }
  // Generador reproducible de entradas variadas: siempre parte del mismo valor para poder repetir un fallo.
  // Las operaciones con bits (& y >>) extraen datos del numero. No se usa este generador para manejar el robot.
  std::uint32_t seed = 42;
  // Revisa cien mil entradas: las salidas deben ser numeros validos y mantenerse dentro del limite.
  // 1e-9 es un margen minusculo para diferencias de redondeo con decimales.
  for (int i = 0; i < 100000; ++i) {
    seed = seed * 1664525u + 1013904223u;
    Input input{int((seed >> 8) % 401) - 200, int((seed >> 16) % 401) - 200,
                bool(seed & 4), bool(seed & 16)};
    out = driver.update(input);
    const double throttle = driving::axis(input.throttle);
    const double limit = input.precision ? 35 : throttle < 0 ? 45 :
                         input.sprint && throttle > 0 ? 100 : 60;
    assert(std::isfinite(out.left) && std::isfinite(out.right));
    assert(std::abs(out.left) <= limit + 1e-9 && std::abs(out.right) <= limit + 1e-9);
  }

  // Simula una pinza: recibe tiempos, velocidad medida y temperatura inventados para probar casos concretos.
  mechanism::Protection claw(1200, 400, 20, 55);
  assert(claw.update(30, 0, 20).speed == 0); // Arranca bloqueada.
  claw.update(0, 0, 20);
  assert(claw.update(30, 20, 20).speed == 30);
  for (int i = 0; i < 19; ++i) assert(claw.update(30, 0, 20).speed == 30);
  assert(claw.update(30, 0, 20).speed == 0); // Atasco de 400 ms.
  assert(claw.state() == mechanism::State::stalled);
  assert(claw.update(-30, 20, 20).speed == 0); // Invertir no evade el bloqueo.
  claw.update(0, 0, 20);
  for (int i = 0; i < 59; ++i) assert(claw.update(-30, -20, 20).speed == -30);
  assert(claw.update(-30, -20, 20).speed == 0); // Limite de 1,2 s.
  assert(claw.state() == mechanism::State::timedOut);
  claw.update(0, 0, 20);
  // auto deja que C++ deduzca el tipo del resultado. Aqui comprobamos el bloqueo por calor y su rearme.
  auto hot = claw.update(30, 20, 55);
  assert(hot.speed == 0 && hot.hot);
  assert(claw.state() == mechanism::State::hot);
  assert(claw.update(30, 20, 50).speed == 0); // Enfriar requiere soltar mando.
  claw.update(0, 0, 50);
  assert(claw.update(30, 20, 50).speed == 30);
  claw.disable();
  assert(claw.update(30, 20, 20).speed == 0);

  // El brazo usa la misma proteccion pero con otro tiempo maximo. Se comprueba tambien ese caso.
  mechanism::Protection arm(4000, 400, 20, 55);
  arm.update(0, 0, 20);
  for (int i = 0; i < 199; ++i) assert(arm.update(35, 20, 20).speed == 35);
  assert(arm.update(35, 20, 20).speed == 0); // Limite propio del brazo.
}
