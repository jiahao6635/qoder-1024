#pragma once
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "FontMetrics.h"
namespace fonts {
struct Font { int size; };
constexpr Font efontCN_12{12}, efontCN_16{16};
}
namespace lgfx {
enum class textdatum_t { top_left };
class LGFXBase {
 public:
  int size = 12, scale = 1;
  uint16_t ink = 0;
  void setFont(const fonts::Font* font) { size = font->size; }
  void setTextSize(int value) { scale = value; }
  void setTextColor(uint16_t value) { ink = value; }
  void setTextDatum(textdatum_t) {}
  void setTextWrap(bool) {}
  static uint32_t next(const char*& s) {
    auto p = reinterpret_cast<const unsigned char*>(s);
    uint32_t ch = *p++;
    if (ch >= 0xF0) { ch &= 7; for (int i=0;i<3;++i) ch = (ch<<6)|(*p++&63); }
    else if (ch >= 0xE0) { ch &= 15; for (int i=0;i<2;++i) ch = (ch<<6)|(*p++&63); }
    else if (ch >= 0xC0) { ch = ((ch & 31)<<6)|(*p++&63); }
    s = reinterpret_cast<const char*>(p);
    return ch;
  }
  int textWidth(const char* value) {
    int left = 0, right = 0;
    for (const char* s = value; *s;) {
      const auto m = findMetric(size, next(s));
      if (!left && !right && m.offset < 0) left = right = -m.offset * scale;
      right = left + (m.advance > m.width + m.offset ? m.advance : m.width + m.offset) * scale;
      left += m.advance * scale;
    }
    return right;
  }
  void rect(int x,int y,int w,int h,uint16_t color, bool fill) {
    assert(x >= 0 && y >= 0 && w > 0 && h > 0 && x+w <= 240 && y+h <= 135);
    std::printf("{\"op\":\"%s\",\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d,\"c\":%u}\n", fill?"fill":"rect", x,y,w,h,color);
  }
  void fillRect(int x,int y,int w,int h,uint16_t color) { rect(x,y,w,h,color,true); }
  void drawRect(int x,int y,int w,int h,uint16_t color) { rect(x,y,w,h,color,false); }
  void fillScreen(uint16_t color) { fillRect(0,0,240,135,color); }
  void drawString(const char* value,int x,int y) {
    assert(x >= 0 && y >= 0 && x+textWidth(value)<=240 && y+size*scale <=135);
    std::printf("{\"op\":\"text\",\"x\":%d,\"y\":%d,\"size\":%d,\"scale\":%d,\"c\":%u,\"text\":\"",x,y,size,scale,ink);
    for(const char* s=value;*s;++s) { if(*s=='"'||*s=='\\') std::putchar('\\'); std::putchar(*s); }
    std::puts("\"}");
  }
};
}
