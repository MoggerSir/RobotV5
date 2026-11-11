#pragma once
#include "drive-logic.h"

namespace mechanism {
struct Output { double speed; bool hot; };
enum class State { ready, releaseRequired, timedOut, stalled, hot };
class Protection {
 public:
  Protection(int maxRunMs, int stallMs, int tickMs, double maxTemperature)
      : maxRunMs_(maxRunMs), stallMs_(stallMs), tickMs_(tickMs), maxTemperature_(maxTemperature) {}
  void disable(State reason = State::releaseRequired) {
    elapsed_ = 0; stalled_ = 0; state_ = reason;
  }
  State state() const { return state_; }
  Output update(double command, double velocity, double temperature) {
    if (temperature >= maxTemperature_) { disable(State::hot); return {0, true}; }
    if (command == 0) {
      elapsed_ = 0; stalled_ = 0; state_ = State::ready;
      return {0, false};
    }
    if (state_ != State::ready) return {0, true};
    elapsed_ += tickMs_;
    if (driving::magnitude(velocity) < 5) stalled_ += tickMs_;
    else stalled_ = 0;
    if (elapsed_ >= maxRunMs_) { disable(State::timedOut); return {0, true}; }
    if (stalled_ >= stallMs_) { disable(State::stalled); return {0, true}; }
    return {command, false};
  }
 private:
  const int maxRunMs_, stallMs_, tickMs_;
  const double maxTemperature_;
  int elapsed_ = 0;
  int stalled_ = 0;
  State state_ = State::releaseRequired;
};
}
