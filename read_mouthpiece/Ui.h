#pragma once

#include <M5Unified.h>
#include <cstdio>
#include <cstring>
#include "Controller.h"

namespace mouthpiece {

struct Frame {
  uint32_t now = 0;
  uint16_t transitionMs = 0;
  bool recovering = false;
  bool imuOk = true;
  uint8_t hintPage = 0;
};

constexpr uint16_t color(unsigned r, unsigned g, unsigned b) {
  return ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3);
}
constexpr uint16_t kGreen = color(177, 242, 198);
constexpr uint16_t kOrange = color(255, 155, 121);
constexpr uint16_t kGreenInk = color(19, 54, 40);
constexpr uint16_t kOrangeInk = color(71, 31, 26);
constexpr uint16_t kPaper = color(255, 249, 226);
constexpr uint16_t kSkin = color(255, 220, 175);

inline void label(lgfx::LGFXBase& c, const char* text, int x, int y,
                  uint16_t ink, int scale = 1) {
  c.setTextColor(ink);
  c.setTextSize(scale);
  c.drawString(text, x, y);
  c.setTextSize(1);
}

struct Lines {
  char first[97] = {};
  char second[97] = {};
  int scale = 2;
  bool fits = true;
};

// Byte offsets always land on UTF-8 boundaries. More than two lines is a build-check error.
inline Lines wrap(lgfx::LGFXBase& c, const char* source, uint8_t preferredBreak = 0) {
  Lines lines;
  size_t offsets[65] = {};
  size_t count = 0, bytes = std::strlen(source);
  if (bytes > 96) { lines.fits = false; return lines; }
  for (size_t i = 0; i < bytes;) {
    if (count == 64) { lines.fits = false; return lines; }
    offsets[count++] = i;
    const unsigned char ch = static_cast<unsigned char>(source[i]);
    i += ch < 0x80 ? 1 : (ch < 0xE0 ? 2 : (ch < 0xF0 ? 3 : 4));
    if (i > bytes) { lines.fits = false; return lines; }
  }
  offsets[count] = bytes;
  c.setFont(&fonts::efontCN_16);
  for (int scale = 2; scale >= 1; --scale) {
    c.setTextSize(scale);
    if (c.textWidth(source) <= 160) {
      std::strcpy(lines.first, source);
      lines.scale = scale;
      c.setTextSize(1);
      return lines;
    }
    if (preferredBreak > 0 && preferredBreak < count) {
      char first[97] = {}, second[97] = {};
      std::memcpy(first, source, offsets[preferredBreak]);
      std::strcpy(second, source + offsets[preferredBreak]);
      const bool punctuation = std::strncmp(second, "，", 3) == 0 || std::strncmp(second, "。", 3) == 0 ||
                               second[0] == ',' || second[0] == '.' || second[0] == '!';
      if (!punctuation && c.textWidth(first) <= 160 && c.textWidth(second) <= 160) {
        std::strcpy(lines.first, first);
        std::strcpy(lines.second, second);
        lines.scale = scale;
        c.setTextSize(1);
        return lines;
      }
    }
    int bestScore = 100000;
    for (size_t split = 1; split < count; ++split) {
      char first[97] = {}, second[97] = {};
      std::memcpy(first, source, offsets[split]);
      std::strcpy(second, source + offsets[split]);
      if (std::strncmp(second, "，", 3) == 0 || std::strncmp(second, "。", 3) == 0 ||
          second[0] == ',' || second[0] == '.' || second[0] == '!') continue;
      const int a = c.textWidth(first), b = c.textWidth(second);
      if (a > 160 || b > 160) continue;
      int score = (a > b ? a - b : b - a) + (a < b ? 2 : 0);
      if (offsets[split] >= 3 && std::strncmp(first + offsets[split] - 3, "，", 3) == 0) score -= 48;
      if (score < bestScore) {
        std::strcpy(lines.first, first);
        std::strcpy(lines.second, second);
        bestScore = score;
      }
    }
    if (bestScore < 100000) {
      lines.scale = scale;
      c.setTextSize(1);
      return lines;
    }
  }
  c.setTextSize(1);
  lines.fits = false;
  return lines;
}

inline void person(lgfx::LGFXBase& c, Face face, const Frame& f, uint16_t ink) {
  const bool blink = f.now % 4000 < 500;
  const bool snap = f.transitionMs > 0 && f.transitionMs < 180;
  const int bounce = snap ? ((f.transitionMs / 60) % 2 ? 0 : -3) : 0;
  int x = 190, y = 40 + bounce;
  c.fillRect(183, 105, 49, 3, ink);
  if (face == Face::Flat) {
    c.fillRect(182, 78, 29, 24, ink);
    c.fillRect(185, 82, 23, 16, kSkin);
    c.fillRect(183, 76, 28, 7, ink);
    c.fillRect(188, 87, 6, 2, ink);
    c.fillRect(200, 87, 6, 2, ink);
    c.fillRect(195, 94, 7, 2, ink);
    c.fillRect(211, 87, 17, 15, ink);
    c.fillRect(213, 89, 12, 9, kPaper);
    c.fillRect(225, 82, 6, 17, ink);
    c.fillRect(228, 78, 6, 7, ink);
    c.fillRect(212, 100, 13, 3, kSkin);
    return;
  }
  if (face == Face::Collapse) y += 7;
  c.fillRect(x + 2, y, 29, 6, ink);
  c.fillRect(x - 1, y + 6, 35, 29, ink);
  c.fillRect(x + 2, y + 8, 29, 24, kSkin);
  c.fillRect(x + 2, y + 5, 19, 7, ink);
  c.fillRect(x + 4, y + 35, 25, 21, ink);
  c.fillRect(x + 7, y + 35, 19, 17, kPaper);
  c.fillRect(x + 14, y + 35, 5, 12, ink);
  c.fillRect(x + 4, y + 56, 8, 7, ink);
  c.fillRect(x + 21, y + 56, 8, 7, ink);
  c.fillRect(x + 1, y + 62, 11, 3, ink);
  c.fillRect(x + 21, y + 62, 11, 3, ink);
  c.fillRect(x - 3, y + 37, 6, 16, ink);
  c.fillRect(x + 30, y + 37, 6, 16, ink);
  c.fillRect(x - 2, y + 50, 4, 6, kSkin);
  c.fillRect(x + 31, y + 50, 4, 6, kSkin);
  if (face == Face::Smile) {
    c.fillRect(x + 7, y + 17, 4, blink ? 2 : 5, ink);
    c.fillRect(x + 23, y + 17, 4, blink ? 2 : 5, ink);
    c.fillRect(x + 11, y + 26, 3, 3, ink);
    c.fillRect(x + 14, y + 29, 9, 2, ink);
    c.fillRect(x + 23, y + 26, 3, 3, ink);
  } else if (face == Face::Collapse) {
    for (int eye = 0; eye < 2; ++eye) {
      const int ex = x + 6 + eye * 16;
      c.fillRect(ex, y + 16, 3, 3, ink);
      c.fillRect(ex + 3, y + 19, 3, 3, ink);
      c.fillRect(ex, y + 22, 3, 3, ink);
    }
    c.fillRect(x + 13, y + 26, 9, 5, ink);
    c.fillRect(x - 7, y + 17, 3, 11, kPaper);
    c.fillRect(x + 36, y + 17, 3, 11, kPaper);
  } else {
    c.fillRect(x + 5, y + 18, 9, 2, ink);
    c.fillRect(x + 21, y + 18, 9, 2, ink);
    c.fillRect(x + 10, y + 20, 3, 3, ink);
    c.fillRect(x + 24, y + 20, 3, 3, ink);
    c.fillRect(x + 13, y + 28, 11, 2, ink);
    if (face == Face::Sweat) {
      c.fillRect(x + 35, y + 9, 3, 5, kPaper);
      c.fillRect(x + 33, y + 14, 7, 6, kPaper);
    }
  }
  if (snap) {
    c.fillRect(180, 35, 3, 11, ink);
    c.fillRect(176, 39, 11, 3, ink);
    c.fillRect(230, 31, 3, 7, ink);
  }
}

inline void render(lgfx::LGFXBase& c, const Controller& state, const Frame& frame) {
  const bool inner = state.inner && !state.rescue;
  const uint16_t bg = inner ? kOrange : kGreen;
  const uint16_t ink = inner ? kOrangeInk : kGreenInk;
  c.fillScreen(bg);
  c.setTextDatum(lgfx::textdatum_t::top_left);
  c.setTextWrap(false);
  c.setFont(&fonts::efontCN_12);
  label(c, state.rescue ? "标准营业中" : state.card().scene, 8, 5, ink);
  char packLabel[40];
  std::snprintf(packLabel, sizeof(packLabel), "%s %u/%u", state.pack().name,
                static_cast<unsigned>(state.cardIndex + 1), static_cast<unsigned>(state.pack().count));
  label(c, packLabel, 232 - c.textWidth(packLabel), 5, ink);
  c.fillRect(8, 22, 224, 2, ink);
  c.fillRect(176, 31, 2, 74, ink);

  const char* mainText = state.rescue ? "热爱工作" : (inner ? state.card().inner : state.card().polite);
  const uint8_t preferredBreak = state.rescue ? 0 : (inner ? state.card().innerBreak : state.card().politeBreak);
  const Lines lines = wrap(c, mainText, preferredBreak);
  const bool two = lines.second[0] != '\0';
  const int lineHeight = lines.scale * 16;
  const int firstY = two ? (lines.scale == 2 ? 29 : 42) : (lines.scale == 2 ? 46 : 54);
  label(c, lines.first, 8, firstY, ink, lines.scale);
  if (two) label(c, lines.second, 8, firstY + lineHeight + 2, ink, lines.scale);
  c.setFont(&fonts::efontCN_12);
  if (!lines.fits) label(c, "文案过长，请缩短", 8, 54, ink);
  const char* note = state.rescue ? "情绪稳定" : (inner ? state.card().innerNote : "");
  if (note && note[0]) label(c, note, 8, 99, ink);
  const Face face = state.rescue ? Face::Smile : (inner ? state.card().innerFace : state.card().politeFace);
  person(c, face, frame, ink);

  c.fillRect(8, 113, 224, 1, ink);
  label(c, "已读，嘴替。", 8, 120, ink);
  const char* hint = frame.hintPage % 2 ? "长A锁定 长B换包" : "A换卡 摇反转 B恢复";
  if (state.rescue) hint = "A换卡 / 体面保持";
  if (!frame.imuOk) hint = "动作未就绪 / A换卡";
  if (state.locked) hint = "拍照锁定 / B恢复";
  label(c, hint, 89, 120, ink);
  if (state.locked) {
    c.drawRect(225, 119, 6, 7, ink);
    c.fillRect(223, 124, 10, 8, ink);
    c.fillRect(227, 126, 2, 3, bg);
  }
}

}  // namespace mouthpiece
