#include "../read_mouthpiece/Controller.h"

#include <cassert>
#include <cstring>
#include <iostream>

using namespace mouthpiece;

struct Hands {
  Controller state;
  uint32_t now = 0;

  Event step(uint32_t elapsed, bool a, bool b) {
    now += elapsed;
    return state.update(now, a, b);
  }
  Event tapA() {
    assert(step(30, true, false) == Event::None);
    return step(70, false, false);
  }
  void holdA() {
    assert(step(30, true, false) == Event::None);
    assert(step(Controller::kHoldA, true, false) == Event::LockChanged);
    assert(step(1000, true, false) == Event::None);
    assert(step(20, false, false) == Event::None);
  }
  void holdB() {
    assert(step(30, false, true) == Event::Rescued);
    assert(step(Controller::kHoldB, false, true) == Event::PackChanged);
    assert(step(1000, false, true) == Event::None);
    assert(step(20, false, false) == Event::None);
  }
  Event shake() {
    now += Controller::kInputGuard;
    return state.shake(now);
  }
};

int main() {
  const char* expected[][3] = {
      {"又加需求", "收到，安排", "你来，你来"},
      {"被问进度", "正在推进", "刚刚新建文件夹"},
      {"开会被点名", "我补充两句", "我也刚刚上线"},
      {"被问懂了吗", "理解，理解", "上下文已满"},
      {"被催回复", "深度思考中", "其实在等外卖"},
      {"下班前来活", "明天优先处理", "今日额度用尽"},
      {"认识新朋友", "很高兴认识你", "请先给我提示词"},
      {"同事又来求助", "咱俩谁跟谁", "青梅牛马实锤"},
      {"社交电量不足", "下次一定聊", "请明天再认识我"},
      {"朋友催出门", "马上就到", "正在加载人类"},
      {"思考人生", "深度思考中", "午饭吃什么"},
      {"信息太多", "上下文已满", "请明天再说"},
      {"脑力不足", "今日额度用尽", "咖啡可续费"},
      {"遇到难题", "正在调用工具", "问问旁边的人"},
      {"团队协作", "已开启多智能体", "全组都不会"},
      {"独立思考", "本地模型运行中", "全靠我硬想"},
      {"AI 也没办法", "正在人工接管", "我也不知道"},
      {"AI 干活现场", "龙虾正在干活", "我负责看着"},
  };
  assert(kPackCount == 3);
  assert(packs[0].count == 6 && packs[1].count == 4 && packs[2].count == 8);
  size_t total = 0;
  for (const auto& pack : packs) {
    for (size_t i = 0; i < pack.count; ++i, ++total) {
      const auto& card = pack.cards[i];
      assert(std::strcmp(card.scene, expected[total][0]) == 0);
      assert(std::strcmp(card.polite, expected[total][1]) == 0);
      assert(std::strcmp(card.inner, expected[total][2]) == 0);
      assert(card.politeFace == Face::Smile && card.innerFace != Face::Smile);
      assert(std::strcmp(card.innerNote, total == 8 ? "社交额度用尽" : "") == 0);
    }
  }
  assert(total == 18);

  Hands h;
  auto& s = h.state;
  assert(s.packIndex == 0 && s.cardIndex == 0 && !s.inner && !s.rescue && !s.locked);
  for (size_t p = 0; p < kPackCount; ++p) {
    assert(s.packIndex == p && s.cardIndex == 0);
    const size_t count = s.pack().count;
    for (size_t i = 0; i < count; ++i) {
      assert(s.cardIndex == i && !s.inner);
      assert(h.shake() == Event::Flipped && s.inner);
      assert(h.shake() == Event::Flipped && !s.inner);
      assert(h.shake() == Event::Flipped && s.inner);
      assert(h.tapA() == Event::CardChanged && !s.inner);
    }
    assert(s.cardIndex == 0);
    h.holdB();
    assert(!s.rescue && !s.locked && !s.inner);
  }
  assert(s.packIndex == 0);

  assert(h.shake() == Event::Flipped && s.inner);
  h.holdA();
  assert(s.locked && s.inner);
  assert(h.tapA() == Event::None && s.cardIndex == 0);
  assert(h.shake() == Event::None && s.inner);
  h.holdA();
  assert(!s.locked && s.inner && s.cardIndex == 0);
  assert(h.shake() == Event::Flipped && !s.inner);

  h.holdA();
  assert(s.locked);
  assert(h.step(30, false, true) == Event::Rescued);
  assert(s.rescue && !s.locked && !s.inner);
  assert(h.step(50, false, false) == Event::None);
  assert(h.shake() == Event::None && s.rescue);
  h.holdA();
  assert(s.locked && s.rescue);
  assert(h.tapA() == Event::None && s.rescue);
  h.holdA();
  assert(!s.locked && s.rescue);
  assert(h.tapA() == Event::CardChanged && s.cardIndex == 1 && !s.rescue);
  assert(s.shake(h.now) == Event::None && !s.inner);
  assert(s.shake(h.now + 249) == Event::None);
  assert(h.shake() == Event::Flipped);
  h.holdA();
  h.holdB();
  assert(s.packIndex == 1 && s.cardIndex == 0 && !s.locked && !s.rescue && !s.inner);

  // Both pressed together, A released later: B wins without a stale A action.
  assert(h.step(20, true, true) == Event::Rescued);
  assert(h.step(Controller::kHoldA, true, true) == Event::None);
  assert(h.step(20, true, false) == Event::None);
  assert(h.step(2000, true, false) == Event::None);
  assert(h.step(20, false, false) == Event::None);
  assert(s.rescue && s.cardIndex == 0 && !s.locked);
  assert(h.tapA() == Event::CardChanged && s.cardIndex == 1);

  // B also interrupts an A hold that began earlier, and consumes A's release.
  assert(h.step(20, true, false) == Event::None);
  assert(h.step(300, true, true) == Event::Rescued);
  assert(h.step(1100, true, true) == Event::PackChanged);
  assert(h.step(20, false, false) == Event::None);
  assert(s.packIndex == 2 && s.cardIndex == 0 && !s.rescue && !s.locked);

  assert(h.step(30, true, false) == Event::None);
  assert(s.shake(h.now + 9999) == Event::None);
  assert(h.step(900, false, false) == Event::LockChanged);  // Delayed loop at release.
  assert(s.locked && s.cardIndex == 0);
  assert(h.step(30, false, true) == Event::Rescued);
  assert(h.step(1100, false, false) == Event::PackChanged);
  assert(s.packIndex == 0 && !s.locked && !s.rescue);

  Hands wrap;
  wrap.now = UINT32_MAX - 500;
  wrap.holdA();
  assert(wrap.state.locked);
  wrap.holdB();
  assert(wrap.state.packIndex == 1 && !wrap.state.locked);
  assert(wrap.state.shake(wrap.now + 249) == Event::None);
  assert(wrap.shake() == Event::Flipped);
  std::cout << "mouthpiece_logic_test: PASS (18 exact cards, 6/4/8 wrap, reverse, rescue, lock, holds, B priority, guard, rollover)\n";
}
