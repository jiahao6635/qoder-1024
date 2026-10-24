#pragma once

#include <cstdint>

namespace worker {

enum class State { Idle, Focus, Paused, Rest, Empty };
enum class Mode { Demo, Daily };
enum class Sound { None, Empty, Full };

class Game {
 public:
  void tick(uint32_t now) {
    const uint32_t elapsed = now - lastMs_;
    lastMs_ = now;
    if (state_ == State::Focus) {
      if (elapsed >= chargeMs_) {
        chargeMs_ = 0;
        state_ = State::Empty;
        sound_ = Sound::Empty;
      } else {
        chargeMs_ -= elapsed;
      }
    } else if (state_ == State::Rest && !isFull()) {
      const uint32_t missing = workMs() - chargeMs_;
      if (elapsed >= (missing + rechargeRate() - 1) / rechargeRate()) {
        chargeMs_ = workMs();
        sound_ = Sound::Full;
      } else {
        chargeMs_ += elapsed * rechargeRate();
      }
    }
  }

  void pressA(uint32_t now) {
    tick(now);
    if (state_ == State::Focus) {
      state_ = State::Paused;
    } else if (state_ == State::Paused) {
      state_ = State::Focus;
    } else {
      chargeMs_ = workMs();
      state_ = State::Focus;
      sound_ = Sound::None;
    }
  }

  void pressB(uint32_t now) {
    tick(now);
    state_ = State::Rest;
  }

  void toggleMode(uint32_t now) {
    mode_ = mode_ == Mode::Demo ? Mode::Daily : Mode::Demo;
    state_ = State::Idle;
    chargeMs_ = workMs();
    lastMs_ = now;
    sound_ = Sound::None;
  }

  bool shake(uint32_t now) {
    tick(now);
    if (state_ != State::Focus) return false;
    const uint32_t boosted = chargeMs_ + workMs() / 5;
    chargeMs_ = boosted > workMs() ? workMs() : boosted;
    return true;
  }

  Sound takeSound() {
    const Sound result = sound_;
    sound_ = Sound::None;
    return result;
  }

  State state() const { return state_; }
  Mode mode() const { return mode_; }
  int energy() const { return (chargeMs_ * 100 + workMs() - 1) / workMs(); }
  bool isFull() const { return chargeMs_ == workMs(); }

  uint32_t remainingMs() const {
    if (state_ == State::Focus || state_ == State::Paused) return chargeMs_;
    if (state_ == State::Rest) {
      return (workMs() - chargeMs_ + rechargeRate() - 1) / rechargeRate();
    }
    return 0;
  }

 private:
  uint32_t workMs() const { return mode_ == Mode::Demo ? 20000u : 1500000u; }
  uint32_t rechargeRate() const { return mode_ == Mode::Demo ? 2u : 5u; }

  State state_ = State::Idle;
  Mode mode_ = Mode::Demo;
  // Store remaining work time so a frame never loses fractional energy.
  uint32_t chargeMs_ = 20000;
  uint32_t lastMs_ = 0;
  Sound sound_ = Sound::None;
};

}  // namespace worker
