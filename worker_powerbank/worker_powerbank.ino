#include <M5Unified.h>
#include "Game.h"
#include "ShakeDetector.h"
#include "Ui.h"

worker::Game game;
worker::ShakeDetector shakeDetector;
M5Canvas canvas(&M5.Display);
bool buffered = false;
bool imuOk = false;
bool cheering = false;
uint32_t cheerStarted = 0;
uint32_t lastFrame = 0;
uint32_t lastImu = 0;

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.internal_spk = true;
  cfg.internal_mic = false;
  cfg.fallback_board = m5::board_t::board_M5StickS3;
  M5.begin(cfg);
  Serial.begin(115200);
  // Never wait for a USB serial host: this is a standalone offline toy.
  M5.Display.setRotation(1);
  M5.Display.setBrightness(150);
  M5.BtnA.setDebounceThresh(20);
  M5.BtnB.setDebounceThresh(20);
  M5.BtnA.setHoldThresh(650);
  M5.BtnB.setHoldThresh(1200);
  M5.Speaker.setVolume(40);
  canvas.setColorDepth(16);
  canvas.setPsram(false);
  buffered = canvas.createSprite(240, 135) != nullptr;
  imuOk = M5.Imu.isEnabled();
  const uint32_t now = millis();
  game.tick(now);
  shakeDetector.reset(now);
  Serial.printf("Worker Powerbank | board=%d | LCD=%ldx%ld | IMU=%d | speaker=%d | sprite=%d | flash=%lu | PSRAM=%lu\n",
                static_cast<int>(M5.getBoard()), static_cast<long>(M5.Display.width()),
                static_cast<long>(M5.Display.height()), imuOk, M5.Speaker.isEnabled(),
                buffered, static_cast<unsigned long>(ESP.getFlashChipSize()),
                static_cast<unsigned long>(ESP.getPsramSize()));
}

void loop() {
  M5.update();
  const uint32_t now = millis();
  game.tick(now);

  // wasClicked excludes holds, so releasing B after a mode switch cannot rest.
  if (M5.BtnB.wasHold()) {
    game.toggleMode(now);
    shakeDetector.reset(now);
    cheering = false;
  } else if (M5.BtnB.wasClicked()) {
    game.pressB(now);
    cheering = false;
  } else if (M5.BtnA.wasClicked()) {
    game.pressA(now);
    cheering = false;
  }

  if (imuOk && now - lastImu >= 10) {
    lastImu = now;
    float ax, ay, az;
    if (M5.Imu.getAccel(&ax, &ay, &az)) {
      // Always track movement; only an active focus round may receive energy.
      if (shakeDetector.update(now, ax, ay, az) && game.shake(now)) {
        cheering = true;
        cheerStarted = now;
      }
    }
    // No new IMU sample is normal; preserve the gesture's cooldown latch.
  }
  if (cheering && (now - cheerStarted >= 1500 || game.state() != worker::State::Focus)) {
    cheering = false;
  }

  const worker::Sound sound = game.takeSound();
  if (sound != worker::Sound::None && M5.Speaker.isEnabled()) {
    M5.Speaker.tone(sound == worker::Sound::Empty ? 660 : 1046, 85);
  }

  if (now - lastFrame >= 50) {
    lastFrame = now;
    lgfx::LGFXBase& target = buffered ? static_cast<lgfx::LGFXBase&>(canvas)
                                    : static_cast<lgfx::LGFXBase&>(M5.Display);
    worker::render(target, game, now, cheering, imuOk);
    if (buffered) canvas.pushSprite(0, 0);
  }
  delay(1);  // Yield to the RTOS; no animation or audio waits in the input loop.
}
