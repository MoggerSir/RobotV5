#pragma once
#include "vex.h"

namespace config {
const int frontLeftPort = vex::PORT8;
const int rearLeftPort = vex::PORT9;
const int frontRightPort = vex::PORT3;
const int rearRightPort = vex::PORT4;
const int armPort = vex::PORT10;
const int clawPort = vex::PORT5;
constexpr vex::gearSetting wheelGear = vex::gearSetting::ratio18_1;
constexpr vex::gearSetting clawGear = vex::gearSetting::ratio36_1;
constexpr vex::gearSetting armGear = vex::gearSetting::ratio36_1;
constexpr bool frontLeftReversed = false;
constexpr bool rearLeftReversed = false;
constexpr bool frontRightReversed = true;
constexpr bool rearRightReversed = true;
constexpr bool armReversed = true;
constexpr bool clawReversed = false;
constexpr int tickMs = 10;
constexpr int wheelTorque = 85;
constexpr int armSpeed = 35;
constexpr int armTorque = 70;
constexpr int armHoldTorque = 50;
constexpr int clawSpeed = 80;
constexpr int clawTorque = 100;
constexpr int clawHoldTorque = 80;
constexpr int stallMs = 400;
constexpr double maxTemperatureC = 50.0;
}
