#pragma once
#include "drive-logic.h"

namespace mechanism {
enum class Alert { none, stalled, hot };
class Monitor {
 public:
  Monitor(int stallMs, int tickMs, double maxTemperature)
      : stallMs_(stallMs), tickMs_(tickMs), maxTemperature_(maxTemperature) {}
  Alert update(double command, double velocity, double temperature) {
    if (command == 0 || driving::magnitude(velocity) >= 5) stalled_ = 0;
    else if (stalled_ < stallMs_) stalled_ += tickMs_;
    if (temperature >= maxTemperature_) return Alert::hot;
    if (stalled_ >= stallMs_) return Alert::stalled;
    return Alert::none;
  }
 private:
  const int stallMs_, tickMs_;
  const double maxTemperature_;
  int stalled_ = 0;
};
}
