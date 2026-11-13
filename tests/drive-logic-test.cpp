#include "drive-logic.h"
#include "mechanism-logic.h"
#include <cassert>
#include <cmath>
#include <cstdint>

using driving::Controller;
using driving::Input;
using driving::Output;

Output settle(Controller& driver, Input input) {
  Output result{};
  for (int i = 0; i < 100; ++i) result = driver.update(input);
  return result;
}

int main() {
  assert(driving::axis(7) == 0 && driving::axis(-7) == 0);
  assert(driving::axis(100) == 1 && driving::axis(-100) == -1);
  assert(driving::axis(999) == 1 && driving::axis(-999) == -1);
  for (int i = 0; i <= 100; ++i) assert(driving::axis(i) == -driving::axis(-i));

  Controller driver;
  Output out = settle(driver, {100, 0, false, false});
  assert(out.left == 50 && out.right == 50);
  out = settle(driver, {100, 0, true, false});
  assert(out.left == 100 && out.right == 100);
  out = settle(driver, {-100, 0, true, false});
  assert(out.left == -40 && out.right == -40);
  out = settle(driver, {100, 0, true, true});
  assert(out.left == 35 && out.right == 35);
  out = settle(driver, {0, 0, true, false});
  assert(out.left == 0 && out.right == 0);
  out = settle(driver, {0, 100, false, false});
  assert(out.left == 40 && out.right == -40);
  out = settle(driver, {0, -100, false, false});
  assert(out.left == -40 && out.right == 40);
  out = settle(driver, {0, 100, false, true});
  assert(out.left == 35 && out.right == -35);

  settle(driver, {100, 0, true, false});
  out = driver.update({0, 0, true, false});
  assert(out.left == 0 && out.right == 0);

  driver.reset();
  out = driver.update({100, 0, false, false});
  assert(out.left == 6 && out.right == 6);
  driver.reset();
  out = driver.update({0, 100, false, false});
  assert(out.left == 30 && out.right == -30);
  double peak = 0;
  for (int i = 1; i < driving::boostTicks; ++i) {
    out = driver.update({0, 100, false, false});
    if (out.left > peak) peak = out.left;
  }
  assert(peak == driving::boostLimit);
  out = settle(driver, {0, 100, false, false});
  assert(out.left == 40 && out.right == -40);
  driver.reset();
  out = driver.update({0, 100, false, false}, false);
  assert(out.left == 14 && out.right == -14);
  out = settle(driver, {0, 100, true, false});
  assert(out.left == 40 && out.right == -40);
  driver.reset();
  driver.update({0, 100, false, false});
  driver.update({0, 0, false, false});
  out = driver.update({0, -100, false, false});
  assert(out.left == -14 && out.right == 14);
  settle(driver, {100, 0, false, false});
  bool passedZero = false;
  for (int i = 0; i < 100; ++i) {
    out = driver.update({-100, 0, false, false});
    if (out.left == 0) passedZero = true;
    if (out.left < 0) assert(passedZero);
  }
  driver.reset();
  out = driver.update({0, 0, false, false});
  assert(out.left == 0 && out.right == 0);

  Controller mirror;
  driver.reset();
  for (int i = 0; i < 100; ++i) {
    out = driver.update({100, 65, true, false});
    Output other = mirror.update({100, -65, true, false});
    assert(std::abs(out.left - other.right) < 1e-9);
    assert(std::abs(out.right - other.left) < 1e-9);
  }
  std::uint32_t seed = 42;
  for (int i = 0; i < 100000; ++i) {
    seed = seed * 1664525u + 1013904223u;
    Input input{int((seed >> 8) % 401) - 200, int((seed >> 16) % 401) - 200,
                bool(seed & 4), bool(seed & 16)};
    out = driver.update(input);
    const double throttle = driving::axis(input.throttle);
    const double limit = input.precision ? 35 :
                         input.sprint && throttle > 0 ? 100 : driving::boostLimit;
    assert(std::isfinite(out.left) && std::isfinite(out.right));
    assert(std::abs(out.left) <= limit + 1e-9 && std::abs(out.right) <= limit + 1e-9);
  }

  using mechanism::Alert;
  mechanism::Monitor arm(400, 20, 50);
  assert(arm.update(35, 0, 20) == Alert::none);
  for (int i = 0; i < 18; ++i) assert(arm.update(35, 0, 20) == Alert::none);
  assert(arm.update(35, 0, 20) == Alert::stalled);
  for (int i = 0; i < 1000; ++i) assert(arm.update(35, 0, 20) == Alert::stalled);
  assert(arm.update(35, 20, 20) == Alert::none);
  for (int i = 0; i < 1000; ++i) assert(arm.update(35, 20, 20) == Alert::none);
  for (int i = 0; i < 30; ++i) arm.update(-35, 0, 20);
  assert(arm.update(0, 0, 20) == Alert::none);
  assert(arm.update(0, 0, 50) == Alert::hot);
  for (int i = 0; i < 30; ++i) assert(arm.update(35, 0, 55) == Alert::hot);
  assert(arm.update(35, 0, 45) == Alert::stalled);
}
