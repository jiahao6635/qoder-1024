#pragma once

#include <M5Unified.h>
#include "Game.h"

namespace worker {

constexpr uint16_t rgb565(unsigned r, unsigned g, unsigned b) {
  return ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3);
}
constexpr uint16_t kBg = rgb565(17, 26, 46);
constexpr uint16_t kPanel = rgb565(30, 44, 66);
constexpr uint16_t kWhite = rgb565(250, 247, 235);
constexpr uint16_t kMuted = rgb565(188, 204, 219);
constexpr uint16_t kMint = rgb565(115, 244, 190);
constexpr uint16_t kYellow = rgb565(255, 211, 105);
constexpr uint16_t kCoral = rgb565(255, 139, 139);
constexpr uint16_t kSkin = rgb565(255, 212, 174);

inline void text(lgfx::LGFXBase& c, const char* value, int x, int y,
                 uint16_t color = kWhite, int size = 1) {
  c.setTextSize(size);
  c.setTextColor(color);
  c.drawString(value, x, y);
  c.setTextSize(1);
}

inline void character(lgfx::LGFXBase& c, const Game& game, uint32_t now,
                      bool cheering) {
  const State state = game.state();
  const bool resting = state == State::Rest;
  const bool tired = state == State::Focus && game.energy() <= 55;
  const bool desperate = state == State::Focus && game.energy() <= 20;
  const bool blink = now % 3600 < 130;
  const int bounce = (state == State::Paused || state == State::Empty)
                         ? 0 : ((now / (cheering ? 140 : 500)) % 2) * 2;
  const int x = 24;
  const int y = 27 - bounce + (tired ? 3 : 0);
  c.fillRect(15, 93, 66, 3, kPanel);

  if (state == State::Empty) {
    // A face on the desk and feet in the air: exhaustion without flashing.
    c.fillRect(16, 81, 68, 5, kYellow);
    c.fillRect(21, 86, 4, 10, kYellow);
    c.fillRect(76, 86, 4, 10, kYellow);
    c.fillRect(18, 62, 29, 19, kSkin);
    c.fillRect(15, 59, 32, 7, kBg);
    c.fillRect(18, 60, 29, 5, kWhite);
    c.fillRect(25, 70, 7, 3, kBg);
    c.fillRect(38, 70, 7, 3, kBg);
    c.fillRect(46, 68, 23, 13, kMint);
    c.fillRect(68, 65, 13, 7, kMuted);
    c.fillRect(77, 59, 7, 9, kWhite);
    c.fillRect(53, 78, 12, 4, kSkin);
    return;
  }

  // Pixel blocks, no image files or font-dependent icons.
  c.fillRect(x + 5, y, 29, 6, kWhite);
  c.fillRect(x + 2, y + 6, 35, 30, kSkin);
  c.fillRect(x - 1, y + 7, 6, 12, kWhite);
  c.fillRect(x + 5, y + 3, 17, 6, kWhite);
  c.fillRect(x + 29, y + 3, 8, 9, kWhite);
  c.fillRect(x + 5, y + 36, 29, 20, resting ? kYellow : kMint);
  c.fillRect(x + 17, y + 36, 5, 13, kBg);
  c.fillRect(x + 7, y + 56, 10, 9, kMuted);
  c.fillRect(x + 24, y + 56, 10, 9, kMuted);
  c.fillRect(x + 3, y + 63, 14, 4, kWhite);
  c.fillRect(x + 24, y + 63, 14, 4, kWhite);
  c.fillRect(x - 1, y + (cheering ? 22 : 40), 6, cheering ? 18 : 13, kSkin);
  c.fillRect(x + 34, y + (cheering ? 22 : 40), 6, cheering ? 18 : 13, kSkin);

  if (cheering || resting || state == State::Idle) {
    c.fillRect(x + 9, y + 17, 6, blink ? 2 : 5, kBg);
    c.fillRect(x + 25, y + 17, 6, blink ? 2 : 5, kBg);
    c.fillRect(x + 15, y + 27, 10, 3, kBg);
    c.fillRect(x + 12, y + 24, 3, 3, kBg);
    c.fillRect(x + 25, y + 24, 3, 3, kBg);
  } else if (tired) {
    c.fillRect(x + 8, y + 19, 9, 3, kBg);
    c.fillRect(x + 23, y + 19, 9, 3, kBg);
    c.fillRect(x + 16, y + 28, desperate ? 8 : 11, desperate ? 6 : 3, kBg);
    if (desperate) c.fillRect(x + 37, y + 17, 4, 8, kMuted);
  } else {
    c.fillRect(x + 9, y + 17, 5, blink ? 2 : 7, kBg);
    c.fillRect(x + 25, y + 17, 5, blink ? 2 : 7, kBg);
    c.fillRect(x + 16, y + 28, 10, 3, kBg);
  }
  c.fillRect(x + 5, y + 25, 5, 3, kCoral);
  c.fillRect(x + 30, y + 25, 5, 3, kCoral);

  if (resting) {
    c.fillRect(68, 66, 14, 17, kWhite);
    c.fillRect(70, 68, 10, 3, kBg);
    c.drawRect(82, 69, 6, 9, kWhite);
    c.fillRect(72 + (now / 400) % 2 * 3, 56, 3, 6, kMuted);
  } else if (cheering) {
    c.fillRect(78, 35, 3, 13, kYellow);
    c.fillRect(73, 40, 13, 3, kYellow);
    c.fillRect(13, 33, 3, 7, kYellow);
  } else if (state == State::Focus) {
    c.fillRect(10, 78, 17, 12, kMuted);
    c.fillRect(12, 80, 13, 7, kBg);
    c.fillRect(8, 90, 22, 3, kWhite);
  } else if (state == State::Paused) {
    c.fillRect(77, 40, 4, 12, kYellow);
    c.fillRect(85, 40, 4, 12, kYellow);
  }
}

inline void render(lgfx::LGFXBase& c, const Game& game, uint32_t now,
                   bool cheering, bool imuOk) {
  c.fillScreen(kBg);
  c.setTextDatum(top_left);
  c.setTextWrap(false);
  c.setFont(&fonts::efontCN_16);
  text(c, "打工人充电宝", 6, 2);
  c.fillRect(166, 0, 74, 21, kPanel);
  text(c, game.mode() == Mode::Demo ? "演示模式" : "日常模式", 171, 2, kYellow);
  character(c, game, now, cheering);

  const uint16_t accent = game.energy() > 55 ? kMint :
                          (game.energy() > 20 ? kYellow : kCoral);
  text(c, "精力", 99, 29, kMuted);
  char value[16];
  snprintf(value, sizeof(value), "%3d", game.energy());
  text(c, value, 144, 23, accent, 2);
  text(c, "/100", 198, 37, kMuted);
  c.drawRect(99, 59, 129, 16, kMuted);
  c.fillRect(229, 64, 4, 6, kMuted);
  const int fill = 123 * game.energy() / 100;
  if (fill) c.fillRect(102, 62, fill, 10, accent);
  for (int x = 113; x < 225; x += 13) c.fillRect(x, 62, 2, 10, kBg);

  const char* status = "准备就绪";
  const char* message = "按 A 开始，好好工作";
  const char* hint = "B 休息  /  长按 B 切换模式";
  switch (game.state()) {
    case State::Idle: break;
    case State::Focus:
      status = game.energy() > 55 ? "认真工作" :
               (game.energy() > 20 ? "有点累了" : "快撑不住");
      message = cheering ? "还能再撑一下！" :
                (imuOk ? "摇一摇，为自己加油" : "摇动不可用，请检查 IMU");
      hint = "A 暂停  /  B 休息";
      break;
    case State::Paused:
      status = "已暂停";
      message = "歇口气，精力不消耗";
      hint = "A 继续  /  B 休息";
      break;
    case State::Rest:
      status = game.isFull() ? "充满啦" : "咖啡时间";
      message = game.isFull() ? "满血复活" : "休息一下，慢慢充电";
      hint = "A 新一轮  /  长按 B 切换模式";
      break;
    case State::Empty:
      status = "精力耗尽";
      message = "人类需要充电";
      hint = "B 休息  /  A 新一轮";
      break;
  }
  text(c, status, 99, 79, accent);
  if (game.state() == State::Focus || game.state() == State::Paused ||
      (game.state() == State::Rest && !game.isFull())) {
    const unsigned seconds = (game.remainingMs() + 999) / 1000;
    snprintf(value, sizeof(value), "%02u:%02u", seconds / 60, seconds % 60);
    c.setFont(&fonts::efontCN_12);
    text(c, value, 199, 82, kMuted);
    c.setFont(&fonts::efontCN_16);
  }
  c.fillRect(0, 99, 240, 19, kPanel);
  text(c, message, (240 - c.textWidth(message)) / 2, 100,
       cheering ? kYellow : kWhite);
  c.setFont(&fonts::efontCN_12);
  text(c, hint, (240 - c.textWidth(hint)) / 2, 120, kMuted);
}

}  // namespace worker
