// PROTECCIONES DEL BRAZO Y LA PINZA: decide si se permite una orden de movimiento.
// Trabaja con numeros, no con hardware. Cada mecanismo usa su propia copia y sus propios contadores.
#pragma once
#include "drive-logic.h"

namespace mechanism {
// speed es la orden permitida. hot indica que esta demasiado caliente y se debe evitar la retencion activa.
struct Output { double speed; bool hot; };
// Lista de estados: ready = listo; releaseRequired = soltar el mando;
// timedOut = tiempo agotado; stalled = posible atasco; hot = temperatura elevada.
enum class State { ready, releaseRequired, timedOut, stalled, hot };
// Proteccion compartida del brazo y la garra; no conoce motores ni mandos.
// Una clase junta los datos de proteccion y las funciones que los consultan o cambian.
class Protection {
 public:
  // Constructor: recibe tiempos en milisegundos, duracion de ciclo y temperatura maxima.
  // La parte despues de : guarda esos valores al crear el objeto.
  Protection(int maxRunMs, int stallMs, int tickMs, double maxTemperature)
      : maxRunMs_(maxRunMs), stallMs_(stallMs), tickMs_(tickMs), maxTemperature_(maxTemperature) {}
  // Bloquea el movimiento, borra los contadores y recuerda el motivo.
  // Si no se indica un motivo, exige soltar el mando antes de poder moverse otra vez.
  void disable(State reason = State::releaseRequired) {
    elapsed_ = 0; stalled_ = 0; state_ = reason;
  }
  // Consulta el estado para mostrar un mensaje. const indica que consultarlo no cambia nada.
  State state() const { return state_; }
  // Se llama una vez por ciclo. command es lo pedido; velocity es lo que gira realmente;
  // temperature es la temperatura leida del motor en grados Celsius.
  Output update(double command, double velocity, double temperature) {
    // Primero comprueba el calor, incluso si el mando esta suelto. Enfriar solo no rearma: hay que soltar el mando.
    if (temperature >= maxTemperature_) { disable(State::hot); return {0, true}; }
    // Una orden de cero significa mando suelto o sin peticion de movimiento.
    // Si el motor ya no esta caliente, borra el bloqueo y prepara un nuevo intento.
    if (command == 0) {
      elapsed_ = 0; stalled_ = 0; state_ = State::ready;
      return {0, false};
    }
    // Con un bloqueo activo no deja mover. Cambiar de abrir a cerrar sin pasar por cero no lo evita.
    if (state_ != State::ready) return {0, false};
    // Suma la duracion prevista de un ciclo al tiempo de movimiento.
    // Estos contadores son aproximados: no incluyen el tiempo que tardan las llamadas al SDK.
    elapsed_ += tickMs_;
    // Velocidad real menor que 5 % durante varios ciclos sugiere que el mecanismo esta atascado.
    // No distingue un objeto agarrado de un tope mecanico; solo aplica el limite configurado.
    if (driving::magnitude(velocity) < 5) stalled_ += tickMs_;
    else stalled_ = 0;
    // Detiene por tiempo continuo excesivo, aunque el motor siga girando.
    if (elapsed_ >= maxRunMs_) { disable(State::timedOut); return {0, false}; }
    // Detiene cuando se acumula demasiado tiempo con el motor casi parado.
    if (stalled_ >= stallMs_) { disable(State::stalled); return {0, false}; }
    // Si ninguna proteccion lo impide, devuelve la orden recibida.
    return {command, false};
  }
 // Limites fijos y contadores propios de este mecanismo.
 private:
  const int maxRunMs_, stallMs_, tickMs_;
  const double maxTemperature_;
  int elapsed_ = 0;
  int stalled_ = 0;
  // Arranca bloqueado: primero debe recibir una orden de cero para reconocer que el mando esta suelto.
  State state_ = State::releaseRequired;
};
}
