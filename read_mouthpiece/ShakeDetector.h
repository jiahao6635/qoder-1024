#pragma once

#include <cmath>
#include <cstdint>

namespace mouthpiece {

class ShakeDetector {
 public:
  static constexpr uint32_t kCooldown = 1200;
  static constexpr uint32_t kQuietTime = 450;

  void reset(uint32_t now) {
    initialized_ = false;
    armed_ = true;
    quiet_ = false;
    peaks_ = 0;
    lastSampleMs_ = now;
  }

  bool update(uint32_t now, float ax, float ay, float az) {
    const uint32_t dt = now - lastSampleMs_;
    if (!std::isfinite(ax) || !std::isfinite(ay) || !std::isfinite(az) ||
        std::fabs(ax) > 32 || std::fabs(ay) > 32 || std::fabs(az) > 32) {
      initialized_ = false;
      peaks_ = 0;
      quiet_ = false;
      lastSampleMs_ = now;
      return false;
    }
    if (!initialized_ || dt > 200) {
      gx_ = ax;
      gy_ = ay;
      gz_ = az;
      initialized_ = true;
      peaks_ = 0;
      quiet_ = false;
      lastSampleMs_ = now;
      return false;
    }
    if (dt < 5) return false;
    lastSampleMs_ = now;

    const float magnitude = std::sqrt(ax * ax + ay * ay + az * az);
    const float dx = ax - gx_, dy = ay - gy_, dz = az - gz_;
    const float dynamicSq = dx * dx + dy * dy + dz * dz;
    const float alpha = static_cast<float>(dt) / (300.0f + dt);
    gx_ += alpha * dx;
    gy_ += alpha * dy;
    gz_ += alpha * dz;

    const bool still = std::fabs(magnitude - 1.0f) < 0.15f && dynamicSq < 0.22f * 0.22f;
    if (still) {
      if (!quiet_) quietSinceMs_ = now;
      quiet_ = true;
    } else {
      quiet_ = false;
    }
    if (!armed_) {
      if (now - triggeredMs_ >= kCooldown && quiet_ && now - quietSinceMs_ >= kQuietTime) {
        armed_ = true;
        peaks_ = 0;
      }
      return false;
    }

    if (peaks_ && now - firstPeakMs_ > 700) peaks_ = 0;
    // Rotation preserves gravity magnitude; a single bump lacks a return impulse.
    if (dynamicSq < 1.25f * 1.25f || std::fabs(magnitude - 1.0f) < 0.45f) return false;
    if (peaks_) {
      if (now - lastPeakMs_ < 90) return false;
      const float dot = dx * px_ + dy * py_ + dz * pz_;
      const float previousSq = px_ * px_ + py_ * py_ + pz_ * pz_;
      if (dot > -0.25f * std::sqrt(dynamicSq * previousSq)) return false;
    } else {
      firstPeakMs_ = now;
    }
    px_ = dx;
    py_ = dy;
    pz_ = dz;
    lastPeakMs_ = now;
    if (++peaks_ < 2) return false;
    armed_ = false;
    quiet_ = false;
    triggeredMs_ = now;
    peaks_ = 0;
    return true;
  }

 private:
  bool initialized_ = false;
  bool armed_ = true;
  bool quiet_ = false;
  uint8_t peaks_ = 0;
  uint32_t lastSampleMs_ = 0;
  uint32_t firstPeakMs_ = 0;
  uint32_t lastPeakMs_ = 0;
  uint32_t triggeredMs_ = 0;
  uint32_t quietSinceMs_ = 0;
  float gx_ = 0, gy_ = 0, gz_ = 1;
  float px_ = 0, py_ = 0, pz_ = 0;
};

}  // namespace mouthpiece
