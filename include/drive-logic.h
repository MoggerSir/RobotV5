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

class Controller {
 public:
  void reset() { speed_ = 0; turn_ = 0; boostTicks_ = 0; cooldown_ = 0; }
  Output update(const Input& input) {
    const double throttle = axis(input.throttle);
    const double steering = axis(input.steering);
    const double limit = input.precision ? 35 : throttle < 0 ? 45 :
                         input.sprint && throttle > 0 ? 100 : 60;
    double target = throttle * limit;
    const bool changingDirection = speed_ * target < 0;
    if (changingDirection) target = 0;

    const double baseTurnLimit = throttle == 0 ? (input.precision ? 35 : 50) : limit * 0.85;
    const double rawTurnTarget = steering * baseTurnLimit;

    if (cooldown_ > 0) --cooldown_;
    if (!input.precision && cooldown_ == 0 &&
        (magnitude(rawTurnTarget - turn_) > 14 || (!changingDirection && magnitude(target - speed_) > 18))) {
      boostTicks_ = 4;
      cooldown_ = 16;
    }
    const bool boosting = boostTicks_ > 0;
    if (boosting) --boostTicks_;

    const double turnLimit = boosting && throttle == 0 ? limit : baseTurnLimit;
    const double turnTarget = steering * turnLimit;
    const double rate = changingDirection ? 12 :
                        magnitude(target) < magnitude(speed_) ? 9 : (boosting ? 15 : 6);
    const double turnRate = boosting ? 30 : 14;

    speed_ = approach(speed_, target, rate);
    turn_ = approach(turn_, turnTarget, turnRate);
    if (turn_ > limit) turn_ = limit;
    else if (turn_ < -limit) turn_ = -limit;

    const double clampedSpeed = speed_ > limit ? limit : speed_ < -limit ? -limit : speed_;
    Output result{clampedSpeed + turn_, clampedSpeed - turn_};
    const double over = result.left > limit ? result.left - limit :
                        result.right > limit ? result.right - limit :
                        result.left < -limit ? result.left + limit :
                        result.right < -limit ? result.right + limit : 0;
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
