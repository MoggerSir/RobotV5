#include "vex.h"
#include "robot-config.h"
#include "drive-logic.h"
#include "mechanism-logic.h"

using namespace vex;
brain Brain;
controller Remote(primary);
motor FrontLeft(config::frontLeftPort, config::wheelGear, config::frontLeftReversed);
motor RearLeft(config::rearLeftPort, config::wheelGear, config::rearLeftReversed);
motor FrontRight(config::frontRightPort, config::wheelGear, config::frontRightReversed);
motor RearRight(config::rearRightPort, config::wheelGear, config::rearRightReversed);
motor ArmMotor(config::armPort, config::armGear, config::armReversed);
motor ClawMotor(config::clawPort, config::clawGear, config::clawReversed);

class DriveTrain {
 public:
  DriveTrain() : left_(FrontLeft, RearLeft), right_(FrontRight, RearRight) {
    left_.setMaxTorque(config::wheelTorque, percent);
    right_.setMaxTorque(config::wheelTorque, percent);
  }
  void apply(const driving::Output& output) {
    braked_ = false;
    command(left_, leftSpinning_, output.left);
    command(right_, rightSpinning_, output.right);
  }
  void stop() {
    if (!braked_) {
      left_.stop(brake);
      right_.stop(brake);
      leftSpinning_ = false;
      rightSpinning_ = false;
      braked_ = true;
    }
  }
  bool hot() {
    return FrontLeft.temperature(celsius) >= config::maxTemperatureC ||
           RearLeft.temperature(celsius) >= config::maxTemperatureC ||
           FrontRight.temperature(celsius) >= config::maxTemperatureC ||
           RearRight.temperature(celsius) >= config::maxTemperatureC;
  }
 private:
  static void command(motor_group& group, bool& spinning, double speed) {
    if (driving::magnitude(speed) < 0.01) {
      if (spinning) { group.stop(brake); spinning = false; }
    } else {
      group.spin(forward, speed, percent);
      spinning = true;
    }
  }
  motor_group left_;
  motor_group right_;
  bool leftSpinning_ = true;
  bool rightSpinning_ = true;
  bool braked_ = false;
};

class MotorMechanism {
 public:
  MotorMechanism(motor& device, int maxRunMs, int moveTorque, int holdTorque) : device_(device),
      moveTorque_(moveTorque), holdTorque_(holdTorque),
      protection_(maxRunMs, config::stallMs, config::tickMs, config::maxTemperatureC) {}
  void disable() {
    protection_.disable();
    lock(true);
  }
  const char* status() const {
    switch (protection_.state()) {
      case mechanism::State::ready: return "LISTA";
      case mechanism::State::releaseRequired: return "SOLTAR";
      case mechanism::State::timedOut: return "TIEMPO";
      case mechanism::State::stalled: return "ATASCO";
      case mechanism::State::hot: return "CALOR";
    }
    return "DESC";
  }
  void update(double command) {
    const mechanism::Output output = protection_.update(
        command, device_.velocity(percent), device_.temperature(celsius));
    if (output.speed == 0) lock(output.hot);
    else move(output.speed);
  }
 private:
  enum class Lock { none, hold, brake };
  void lock(bool passiveBrake) {
    if (!device_.installed()) { locked_ = Lock::none; return; }
    const Lock wanted = passiveBrake ? Lock::brake : Lock::hold;
    if (locked_ == wanted) return;
    locked_ = wanted;
    device_.setMaxTorque(holdTorque_, percent);
    device_.stop(passiveBrake ? brake : hold);
  }
  void move(double speed) {
    if (locked_ != Lock::none) {
      locked_ = Lock::none;
      device_.setMaxTorque(moveTorque_, percent);
    }
    device_.spin(forward, speed, percent);
  }
  motor& device_;
  const int moveTorque_, holdTorque_;
  mechanism::Protection protection_;
  Lock locked_ = Lock::none;
};

struct Telemetry {
  volatile int throttle = 0, steering = 0, armAxis = 0;
  volatile bool connected = false, hardwareReady = false, stopped = false, neutral = false;
  volatile bool frontLeft = false, rearLeft = false, frontRight = false, rearRight = false;
  volatile bool armReady = false, clawReady = false, open = false, close = false;
  const char* armStatus = "LISTA";
  const char* clawStatus = "LISTA";
} telemetry;

void screenLoop() {
  this_thread::setPriority(thread::threadPrioritylow);
  while (true) {
    Brain.Screen.printAt(10, 115, true, "%-38s", !telemetry.connected ? "SIN CONTROL" :
        !telemetry.hardwareReady ? "FALTA MOTOR: revisa puertos" :
        telemetry.stopped ? "FRENADO: B con mandos en reposo" : "LISTO PARA MANEJAR");
    Brain.Screen.printAt(10, 140, "J3:%4d J4:%4d J2:%4d B:%d X:%d    ",
        telemetry.throttle, telemetry.steering, telemetry.armAxis,
        int(Remote.ButtonB.pressing()), int(Remote.ButtonX.pressing()));
    Brain.Screen.printAt(10, 165, "8:%d 9:%d 3:%d 4:%d 10:%d 5:%d    ",
        int(telemetry.frontLeft), int(telemetry.rearLeft), int(telemetry.frontRight),
        int(telemetry.rearRight), int(telemetry.armReady), int(telemetry.clawReady));
    Brain.Screen.printAt(10, 190, "BRAZO T:%2.0fC %-7s | PINZA T:%2.0fC %-7s   ",
        ArmMotor.temperature(celsius), telemetry.armStatus,
        ClawMotor.temperature(celsius), telemetry.clawStatus);
    if (telemetry.neutral) Brain.Screen.printAt(10, 215, true, "%-38s", "Mandos centrados: B puede rearmar");
    else Brain.Screen.printAt(10, 215, true, "R1:%d L1:%d | X frena, B rearma       ",
        int(telemetry.open), int(telemetry.close));
    this_thread::sleep_for(200);
  }
}

int main() {
  this_thread::setPriority(thread::threadPriorityHigh);
  DriveTrain wheels;
  MotorMechanism arm(ArmMotor, config::armMaxRunMs, config::armTorque, config::armHoldTorque);
  driving::Controller driver;
  bool stopped = false;
  int clawLock = 0;

  Brain.Screen.printAt(10, 30, "Izq: conducir | R2: sprint | A: lento");
  Brain.Screen.printAt(10, 55, "Der vertical: brazo | R1/L1: garra");
  Brain.Screen.printAt(10, 80, "X: freno/bloqueo | B: rearmar en reposo");
  thread uiThread(screenLoop);

  while (true) {
    const int throttle = Remote.Axis3.position(percent);
    const int steering = Remote.Axis4.position(percent);
    const int armAxis = Remote.Axis2.position(percent);
    const bool sprint = Remote.ButtonR2.pressing();
    const bool open = Remote.ButtonR1.pressing();
    const bool close = Remote.ButtonL1.pressing();
    const bool connected = Remote.installed();
    const bool frontLeftReady = FrontLeft.installed();
    const bool rearLeftReady = RearLeft.installed();
    const bool frontRightReady = FrontRight.installed();
    const bool rearRightReady = RearRight.installed();
    const bool armReady = ArmMotor.installed();
    const bool clawReady = ClawMotor.installed();
    const bool hardwareReady = frontLeftReady && rearLeftReady && frontRightReady &&
                               rearRightReady && armReady && clawReady;
    const bool neutral = abs(throttle) <= 7 && abs(steering) <= 7 && abs(armAxis) <= 7 &&
                         !open && !close && !sprint;
    if (!connected || !hardwareReady || Remote.ButtonX.pressing()) stopped = true;
    else if (stopped && Remote.ButtonB.pressing() && neutral) stopped = false;

    if (stopped) {
      driver.reset();
      wheels.stop();
      arm.disable();
      if (clawLock != 2) {
        ClawMotor.stop(brake);
        clawLock = 2;
      }
    } else {
      const bool cool = !wheels.hot();
      wheels.apply(driver.update({throttle, steering, sprint && cool, Remote.ButtonA.pressing()}, cool));
      double armCommand = driving::axis(armAxis) * config::armSpeed;
      if (driving::magnitude(armCommand) < 5) armCommand = 0;
      arm.update(armCommand);
      if (open != close) {
        if (clawLock != 0) {
          ClawMotor.setMaxTorque(config::clawTorque, percent);
          clawLock = 0;
        }
        ClawMotor.spin(forward, open ? config::clawSpeed : -config::clawSpeed, percent);
      } else if (clawLock != 1) {
        ClawMotor.setMaxTorque(config::clawHoldTorque, percent);
        ClawMotor.stop(hold);
        clawLock = 1;
      }
    }

    telemetry.throttle = throttle;
    telemetry.steering = steering;
    telemetry.armAxis = armAxis;
    telemetry.connected = connected;
    telemetry.hardwareReady = hardwareReady;
    telemetry.stopped = stopped;
    telemetry.neutral = neutral;
    telemetry.frontLeft = frontLeftReady;
    telemetry.rearLeft = rearLeftReady;
    telemetry.frontRight = frontRightReady;
    telemetry.rearRight = rearRightReady;
    telemetry.armReady = armReady;
    telemetry.clawReady = clawReady;
    telemetry.open = open;
    telemetry.close = close;
    telemetry.armStatus = arm.status();
    telemetry.clawStatus = open != close ? (open ? "ABRIENDO" : "CERRANDO") : "DIRECTA";

    wait(config::tickMs, msec);
  }
}
