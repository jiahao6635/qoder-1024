#include <M5Unified.h>
#include "Controller.h"
#include "ShakeDetector.h"
#include "Ui.h"

mouthpiece::Controller controller;
mouthpiece::ShakeDetector shakeDetector;
M5Canvas canvas(&M5.Display);
mouthpiece::Frame frame{};
bool canvasOk = false;
bool imuOk = false;
bool dirty = true;
bool transitioning = false;
bool recovering = false;
uint32_t transitionStarted = 0;
uint32_t lastImu = 0;
uint32_t lastRender = 0;
uint32_t bootTime = 0;
uint32_t lastIdleStep = 0;

void onEvent(mouthpiece::Event event, uint32_t now) {
  if (event == mouthpiece::Event::None) return;
  dirty = true;
  transitioning = event == mouthpiece::Event::Flipped ||
                  event == mouthpiece::Event::Rescued;
  recovering = event == mouthpiece::Event::Rescued;
  transitionStarted = now;
  // A locked photo keeps a steady final pose, including its hints and expression.
  if (event == mouthpiece::Event::LockChanged) frame.transitionMs = 0;
  Serial.printf("event=%u pack=%u card=%u inner=%d rescue=%d locked=%d\n",
                static_cast<unsigned>(event), static_cast<unsigned>(controller.packIndex),
                static_cast<unsigned>(controller.cardIndex), controller.inner,
                controller.rescue, controller.locked);
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.internal_spk = false;
  cfg.internal_mic = false;
  cfg.fallback_board = m5::board_t::board_M5StickS3;
  M5.begin(cfg);
  Serial.begin(115200);  // Standalone boot never waits for a USB host.
  Serial.setTxTimeoutMs(0);  // A stalled serial monitor must never delay B rescue.
  M5.Display.setRotation(1);
  M5.Display.setBrightness(150);
  M5.BtnA.setDebounceThresh(20);
  M5.BtnB.setDebounceThresh(20);
  canvas.setColorDepth(16);
  canvas.setPsram(false);
  canvasOk = canvas.createSprite(240, 135) != nullptr;
  if (!canvasOk) {
    canvas.setPsram(true);
    canvasOk = canvas.createSprite(240, 135) != nullptr;
  }
  imuOk = M5.Imu.isEnabled();
  bootTime = millis();
  shakeDetector.reset(bootTime);
  frame.imuOk = imuOk;
  if (!canvasOk) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setFont(&fonts::efontCN_12);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawString("画面缓存不足，请重启", 12, 50);
  }
  Serial.printf("Read Mouthpiece | LCD=%ldx%ld | IMU=%d | sprite=%d | Flash=%lu | PSRAM=%lu\n",
                static_cast<long>(M5.Display.width()), static_cast<long>(M5.Display.height()),
                imuOk, canvasOk, static_cast<unsigned long>(ESP.getFlashChipSize()),
                static_cast<unsigned long>(ESP.getPsramSize()));
}

void loop() {
  M5.update();
  const uint32_t now = millis();
  // Buttons are processed before IMU and rendering. B can interrupt every frame.
  onEvent(controller.update(now, M5.BtnA.isPressed(), M5.BtnB.isPressed()), now);
  if (imuOk && now - lastImu >= 10) {
    lastImu = now;
    float ax, ay, az;
    if (M5.Imu.getAccel(&ax, &ay, &az) && shakeDetector.update(now, ax, ay, az)) {
      onEvent(controller.shake(now), now);
    }
  }
  const uint32_t elapsed = now - transitionStarted;
  if (transitioning && elapsed >= 240) {
    transitioning = false;
    dirty = true;
  }
  const uint32_t idleStep = now / 500;
  if (!controller.locked && (idleStep != lastIdleStep ||
                            (transitioning && now - lastRender >= 40))) {
    dirty = true;
  }
  if (dirty && canvasOk) {
    if (!controller.locked) {
      frame.now = transitioning ? now : idleStep * 500;
      frame.transitionMs = transitioning ? static_cast<uint16_t>(elapsed + 1) : 0;
      frame.recovering = recovering;
      frame.hintPage = static_cast<uint8_t>(((now - bootTime) / 4000) % 2);
    }
    mouthpiece::render(canvas, controller, frame);
    canvas.pushSprite(0, 0);
    dirty = false;
    lastRender = now;
    lastIdleStep = idleStep;
  }
  delay(1);  // Yield only; no animation, long-press, or serial-host waiting.
}
