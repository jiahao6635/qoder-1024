#include "../read_mouthpiece/Ui.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace mouthpiece;
void screen(lgfx::LGFXBase& c, const Controller& state, Frame frame, const char* id) {
  std::printf("{\"screen\":\"%s\"}\n", id);
  render(c, state, frame);
}
int main() {
  lgfx::LGFXBase c;
  Controller state;
  Frame frame;
  frame.now = 1000;
  for (size_t p = 0; p < sizeof(packs)/sizeof(packs[0]); ++p) {
    state.packIndex = p;
    for (size_t i = 0; i < packs[p].count; ++i) {
      state.cardIndex = i;
      for (int inner = 0; inner < 2; ++inner) {
        state.inner = inner;
        const char* text = inner ? state.card().inner : state.card().polite;
        const uint8_t preferredBreak = inner ? state.card().innerBreak : state.card().politeBreak;
        const Lines lines = wrap(c, text, preferredBreak);
        assert(lines.fits && lines.scale == 2);
        char merged[194];
        std::snprintf(merged,sizeof(merged),"%s%s",lines.first,lines.second);
        assert(std::strcmp(merged,text)==0);
        char id[50];
        std::snprintf(id,sizeof(id),"pack%zu-card%zu-%s",p+1,i+1,inner?"inner":"polite");
        screen(c,state,frame,id);
      }
    }
  }
  state = Controller{};
  state.rescue = true;
  screen(c,state,frame,"rescue");
  state.rescue = false;
  state.inner = true;
  state.locked = true;
  screen(c,state,frame,"photo-lock");
  state.locked = false;
  frame.transitionMs = 60;
  screen(c,state,frame,"flip-60ms");
  frame.transitionMs = 0;
  frame.hintPage = 1;
  screen(c,state,frame,"hold-instructions");
  frame.imuOk = false;
  screen(c,state,frame,"imu-unavailable");
  frame.imuOk = true;
  frame.now = 0;
  screen(c,state,frame,"blink");
  const Lines natural = wrap(c, "明天优先处理", 2);
  assert(std::strcmp(natural.first, "明天") == 0);
  assert(std::strcmp(natural.second, "优先处理") == 0);
  assert(wrap(c, "收到，安排", 2).second[0] == '\0');
  assert(wrap(c, "明天优先处理", 64).fits);
  assert(wrap(c, "明天优先处理", 1).scale == 2);
  // Longer user text falls back to a 16px, two-line layout; never loses bytes silently.
  assert(wrap(c,"这是新增文案的较长示例").fits);
  assert(!wrap(c,"这是一段超过屏幕允许容量而且不能用两行正确显示的非常非常长的文案").fits);
}
