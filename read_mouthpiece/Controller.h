#pragma once

#include <cstdint>
#include "Cards.h"

namespace mouthpiece {

enum class Event { None, CardChanged, Flipped, Rescued, LockChanged, PackChanged };

class Controller {
 public:
  static constexpr uint32_t kHoldA = 800;
  static constexpr uint32_t kHoldB = 1000;
  static constexpr uint32_t kInputGuard = 250;

  size_t packIndex = 0;
  size_t cardIndex = 0;
  bool inner = false;
  bool rescue = false;
  bool locked = false;

  const Card& card() const { return pack().cards[cardIndex]; }
  const Pack& pack() const { return packs[packIndex]; }

  Event update(uint32_t now, bool aDown, bool bDown) {
    Event event = Event::None;
    if (bDown && !bWasDown_) {
      bSince_ = now;
      bConsumed_ = false;
      rescue = true;
      inner = false;
      locked = false;
      guard(now);
      event = Event::Rescued;
    }

    // B acts on press so restoring composure never waits for the hold threshold.
    // Holding it briefly shows rescue before switching packs; release does nothing.
    if (bDown || bWasDown_) {
      if (aDown || aWasDown_) aConsumed_ = true;
      if (!bConsumed_ && now - bSince_ >= kHoldB) {
        bConsumed_ = true;
        packIndex = (packIndex + 1) % kPackCount;
        cardIndex = 0;
        inner = rescue = locked = false;
        guard(now);
        event = Event::PackChanged;
      }
      if (!bDown) guard(now);
    } else {
      if (aDown && !aWasDown_) {
        aSince_ = now;
        aConsumed_ = false;
      }
      if ((aDown || aWasDown_) && !aConsumed_ && now - aSince_ >= kHoldA) {
        aConsumed_ = true;
        locked = !locked;
        guard(now);
        event = Event::LockChanged;
      }
      if (!aDown && aWasDown_) {
        if (!aConsumed_ && !locked) {
          cardIndex = (cardIndex + 1) % pack().count;
          inner = rescue = false;
          event = Event::CardChanged;
        }
        aConsumed_ = false;
        guard(now);
      }
    }

    aWasDown_ = aDown;
    bWasDown_ = bDown;
    return event;
  }

  Event shake(uint32_t now) {
    if (locked || rescue || aWasDown_ || bWasDown_ ||
        (guarded_ && now - lastInput_ < kInputGuard)) {
      return Event::None;
    }
    inner = !inner;
    return Event::Flipped;
  }

 private:
  void guard(uint32_t now) {
    guarded_ = true;
    lastInput_ = now;
  }

  bool aWasDown_ = false;
  bool bWasDown_ = false;
  bool aConsumed_ = false;
  bool bConsumed_ = false;
  bool guarded_ = false;
  uint32_t aSince_ = 0;
  uint32_t bSince_ = 0;
  uint32_t lastInput_ = 0;
};

}  // namespace mouthpiece
