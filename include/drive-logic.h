#pragma once

namespace driving {
struct Input {
  int throttle;
  int steering;
  bool sprint;
  bool precision;
};
struct Output { double left; double right; };
inline double magnitude(double value) { return value < 0 ? -value : value; }
inline double approach(double current, double target, double step) {
  if (current < target) return target - current < step ? target : current + step;
  return current - target < step ? target : current - step;
}
inline double axis(int value) {
  if (value > 100) value = 100;
  if (value < -100) value = -100;
  if (magnitude(value) <= 7) return 0;
  const double normalized = (value > 0 ? value - 7 : value + 7) * (1.0 / 93.0);
  return 0.65 * normalized + 0.35 * normalized * normalized * normalized;
}

constexpr double normalLimit = 50;
constexpr double reverseLimit = 40;
constexpr double precisionLimit = 35;
constexpr double sprintLimit = 100;
constexpr double spinTurnLimit = 40;
constexpr double boostLimit = 75;
constexpr int boostTicks = 25;
constexpr int boostCooldownTicks = 50;

class Controller {
 public:
  void reset() { speed_ = 0; turn_ = 0; boostTicks_ = 0; cooldown_ = 0; }
  Output update(const Input& input, bool cool = true) {
    const double throttle = axis(input.throttle);
    const double steering = axis(input.steering);
    if (cooldown_ > 0) --cooldown_;
    if (throttle == 0 && steering == 0) {
      speed_ = 0; turn_ = 0; boostTicks_ = 0;
      return {0, 0};
    }
    const double limit = input.precision ? precisionLimit : throttle < 0 ? reverseLimit :
                         input.sprint && throttle > 0 ? sprintLimit : normalLimit;
    double target = throttle * limit;
    const bool changingDirection = speed_ * target < 0;
    if (changingDirection) target = 0;

    const bool newTurn = steering != 0 && turn_ * steering <= 0;
    if (cool && !input.precision && cooldown_ == 0 && (newTurn || changingDirection)) {
      boostTicks_ = boostTicks;
      cooldown_ = boostTicks + boostCooldownTicks;
    }
    if (!cool || input.precision) boostTicks_ = 0;
    const bool boosting = boostTicks_ > 0;
    if (boosting) --boostTicks_;

    const double cap = boosting && boostLimit > limit ? boostLimit : limit;
    const double turnLimit = throttle == 0 ? (input.precision ? precisionLimit : boosting ? cap : spinTurnLimit)
                                           : cap * 0.85;
    const double rate = changingDirection ? 12 :
                        magnitude(target) < magnitude(speed_) ? 9 : (boosting ? 15 : 6);
    const double turnRate = boosting ? 30 : 14;

    speed_ = approach(speed_, target, rate);
    turn_ = approach(turn_, steering * turnLimit, turnRate);
    if (turn_ > cap) turn_ = cap;
    else if (turn_ < -cap) turn_ = -cap;

    const double clampedSpeed = speed_ > limit ? limit : speed_ < -limit ? -limit : speed_;
    Output result{clampedSpeed + turn_, clampedSpeed - turn_};
    const double over = result.left > cap ? result.left - cap :
                        result.right > cap ? result.right - cap :
                        result.left < -cap ? result.left + cap :
                        result.right < -cap ? result.right + cap : 0;
    result.left -= over;
    result.right -= over;
    return result;
  }
 private:
  double speed_ = 0;
  double turn_ = 0;
  int boostTicks_ = 0;
  int cooldown_ = 0;
};
}
