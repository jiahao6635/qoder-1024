#pragma once

#include <cstddef>
#include <cstdint>

namespace mouthpiece {

enum class Face { Smile, Deadpan, Sweat, Collapse, Flat };

struct Card {
  const char* scene;
  const char* polite;
  const char* inner;
  Face politeFace;
  Face innerFace;
  const char* innerNote;
  uint8_t politeBreak = 0;  // Optional preferred line break after this many characters.
  uint8_t innerBreak = 0;
};

struct Pack {
  const char* name;
  const Card* cards;
  size_t count;
};

// Preset jokes, not measurements of feelings, AI activity, or account quotas.
constexpr Card workerCards[] = {
    {"又加需求", "收到，安排", "你来，你来", Face::Smile, Face::Collapse, ""},
    {"被问进度", "正在推进", "刚刚新建文件夹", Face::Smile, Face::Sweat, ""},
    {"开会被点名", "我补充两句", "我也刚刚上线", Face::Smile, Face::Sweat, "", 0, 4},
    {"被问懂了吗", "理解，理解", "上下文已满", Face::Smile, Face::Deadpan, ""},
    {"被催回复", "深度思考中", "其实在等外卖", Face::Smile, Face::Deadpan, ""},
    {"下班前来活", "明天优先处理", "今日额度用尽", Face::Smile, Face::Flat, "", 2, 2},
};

constexpr Card socialCards[] = {
    {"认识新朋友", "很高兴认识你", "请先给我提示词", Face::Smile, Face::Sweat, ""},
    {"同事又来求助", "咱俩谁跟谁", "青梅牛马实锤", Face::Smile, Face::Deadpan, "", 0, 4},
    {"社交电量不足", "下次一定聊", "请明天再认识我", Face::Smile, Face::Flat, "社交额度用尽"},
    {"朋友催出门", "马上就到", "正在加载人类", Face::Smile, Face::Collapse, "", 0, 4},
};

constexpr Card aiCards[] = {
    {"思考人生", "深度思考中", "午饭吃什么", Face::Smile, Face::Deadpan, ""},
    {"信息太多", "上下文已满", "请明天再说", Face::Smile, Face::Flat, ""},
    {"脑力不足", "今日额度用尽", "咖啡可续费", Face::Smile, Face::Sweat, "", 2, 0},
    {"遇到难题", "正在调用工具", "问问旁边的人", Face::Smile, Face::Sweat, "", 2, 2},
    {"团队协作", "已开启多智能体", "全组都不会", Face::Smile, Face::Collapse, "", 3, 0},
    {"独立思考", "本地模型运行中", "全靠我硬想", Face::Smile, Face::Sweat, ""},
    {"AI 也没办法", "正在人工接管", "我也不知道", Face::Smile, Face::Deadpan, "", 2, 0},
    {"AI 干活现场", "龙虾正在干活", "我负责看着", Face::Smile, Face::Deadpan, "", 2, 0},
};

constexpr Pack packs[] = {
    {"打工人", workerCards, sizeof(workerCards) / sizeof(workerCards[0])},
    {"社交", socialCards, sizeof(socialCards) / sizeof(socialCards[0])},
    {"AI", aiCards, sizeof(aiCards) / sizeof(aiCards[0])},
};
constexpr size_t kPackCount = sizeof(packs) / sizeof(packs[0]);

}  // namespace mouthpiece
