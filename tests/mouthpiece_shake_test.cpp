#include "../read_mouthpiece/ShakeDetector.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

using mouthpiece::ShakeDetector;

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
      triggers += sample(2.2f * std::sin(i * 0.26f), 0, 1);
    }
    return triggers;
  };

  quiet(100);
  for (int i = 0; i < 300; ++i) assert(!sample(0.3f * std::sin(i * 0.03f), 0, 1));
  for (int i = 0; i < 300; ++i) assert(!sample(std::sin(i * 0.04f), 0, std::cos(i * 0.04f)));
  quiet(100);
  assert(!sample(3.0f, 0, 1));
  quiet(100);
  assert(wave(400) == 1);
  assert(wave(400) == 0);  // Continuous shaking must not retrigger after cooldown.
  now += 400;
  assert(wave(100) == 0);  // A lost sensor interval cannot unlock the same gesture.
  quiet(150);
  assert(wave(23) == 1);  // One back-and-forth cycle, not several repeated shakes.
  quiet(20);
  assert(wave(23) == 0);  // Cooldown and quiet dwell are both required.
  quiet(200);
  assert(wave(23) == 1);

  detector.reset(now);
  assert(!sample(0, 0, 1));
  assert(!sample(std::numeric_limits<float>::quiet_NaN(), 0, 1));
  assert(!sample(std::numeric_limits<float>::max(), 0, 1));
  assert(!sample(0, 0, 1));
  assert(!sample(3, 0, 1));
  now += 1000;
  assert(!sample(-3, 0, 1));
  quiet(150);
  assert(wave(23) == 1);
  assert(!sample(std::numeric_limits<float>::infinity(), 0, 1));
  assert(wave(100) == 0);  // Invalid data must preserve an already latched gesture.

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
  std::cout << "mouthpiece_shake_test: PASS (pickup, rotation, bump, one cycle, continuous shake, cooldown, gap, invalid data, axes, rollover)\n";
}
