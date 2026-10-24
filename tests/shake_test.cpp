#include "../worker_powerbank/ShakeDetector.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

using worker::ShakeDetector;

int main() {
  ShakeDetector detector;
  uint32_t now = 0;
  auto sample = [&](float ax, float ay, float az) {
    now += 10;
    return detector.update(now, ax, ay, az);
  };
  auto quiet = [&](int samples) {
    for (int i = 0; i < samples; ++i) assert(!sample(0, 0, 1));
  };
  auto wave = [&](int samples) {
    int triggers = 0;
    for (int i = 0; i < samples; ++i) {
      const float x = 2.2f * std::sin(i * 0.26f);
      triggers += sample(x, 0, 1);
    }
    return triggers;
  };
  quiet(100);
  // Slow pickup acceleration, then orientation changes, should not qualify.
  for (int i = 0; i < 300; ++i) assert(!sample(0.3f * std::sin(i * 0.03f), 0, 1));
  for (int i = 0; i < 300; ++i) assert(!sample(std::sin(i * 0.04f), 0, std::cos(i * 0.04f)));
  quiet(100);
  assert(!sample(3.0f, 0, 1));
  quiet(100);
  assert(wave(400) == 1);
  // Cooldown alone is insufficient while the same shaking continues.
  assert(wave(400) == 0);
  now += 400;  // Missing samples must not unlock the same ongoing shake.
  assert(wave(100) == 0);
  quiet(150);
  assert(wave(70) == 1);
  quiet(70);
  assert(wave(70) == 0);  // Still inside 1800 ms cooldown.
  quiet(200);
  assert(wave(70) == 1);

  detector.reset(now);
  assert(!sample(0, 0, 1));
  assert(!sample(std::numeric_limits<float>::quiet_NaN(), 0, 1));
  assert(!sample(std::numeric_limits<float>::max(), 0, 1));
  assert(!sample(0, 0, 1));
  assert(!sample(3, 0, 1));
  now += 1000;
  assert(!sample(-3, 0, 1));  // Lost samples discard the half-completed gesture.
  quiet(150);
  assert(wave(70) == 1);

  detector.reset(UINT32_MAX - 500);
  now = UINT32_MAX - 500;
  quiet(10);
  assert(wave(100) == 1);
  quiet(200);
  assert(wave(100) == 1);

  for (int axis = 0; axis < 3; ++axis) {
    detector.reset(now);
    quiet(100);
    int triggers = 0;
    for (int i = 0; i < 100; ++i) {
      const float v = 2.2f * std::sin(i * 0.26f);
      triggers += sample(axis == 0 ? v : 0, axis == 1 ? v : 0, 1 + (axis == 2 ? v : 0));
    }
    assert(triggers == 1);
  }
  std::cout << "shake_test: passed (pickup, rotation, bump, repeated shake, cooldown, invalid data, rollover)\n";
}
