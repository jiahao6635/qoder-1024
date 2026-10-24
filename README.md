# Qoder 1024 · M5StickS3 像素小玩具

基于 Arduino、M5Unified 和 M5GFX 的离线互动作品。

| 目录 | 作品 |
| --- | --- |
| [`read_mouthpiece/`](read_mouthpiece/README.md) | **已读，嘴替。** 按一下保持体面，摇一下说出心里话；18 张双面卡片，支持拍照锁定 |
| [`worker_powerbank/`](worker_powerbank/) | **打工人充电宝** 专注掉精力、休息充电、摇动打气 |
| [`hello_qoder/`](hello_qoder/) | **Hello, Qoder** 屏幕入门示例 |

本仓库发布源码、测试与演示说明。文档中的 `output/` 是本地生成目录，固件、日志、硬件验收记录与预览不随 Git 提交；运行构建脚本可重新生成构建产物。下文实机结果为 2026-10-24 的历史验收记录。

## 环境准备

安装 Arduino CLI、支持 ESP32-S3 的 Espressif core，以及 M5Unified / M5GFX 库；历史验证版本见下文。主机检查需要 Bash、支持 C++17 与 sanitizer 的编译器（默认 `clang++`），以及 Python 3.9+。字体检查默认读取 `~/Documents/Arduino/libraries/M5GFX`；若安装在其他目录，可直接运行 Python 检查脚本并传入 `--library /path/to/M5GFX`。

```bash
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "M5Unified@0.2.27" "M5GFX@0.2.34"
bash tools/test-worker.sh
bash tools/test-mouthpiece.sh
# 选择要编译的作品
bash tools/build-worker.sh
bash tools/build-mouthpiece.sh
```

本项目源码使用 [MIT License](LICENSE)。M5Unified、M5GFX 及其内置字体等依赖保留各自许可证，不纳入本项目的 MIT 授权。

## 打工人充电宝

给 M5Stack **M5StickS3** 做的离线像素小玩具。屏幕里的「精力」是游戏数值，**不是设备真实剩余电量**。不用 Wi-Fi、不用账号、不接大模型，也没有震动反馈。

项目源码在 `worker_powerbank/`，保留固件在 `output/worker_powerbank/firmware/`。现有 `hello_qoder/` 示例保持原样。

**2026-10-24 验证结果：编译、上传及基础功能真机验收均通过，可进行现场演示。** 完整固件无编译警告，程序占用 1,118,815 字节（应用分区 33%），静态 RAM 占用 26,648 字节（8%）。状态机、摇动合成信号测试及中文检查通过。重新检测 `/dev/cu.usbmodem1101`，实读 ESP32-S3-PICO-1、8MB Flash、8MB PSRAM；上传成功，四段写入均通过哈希校验。用户随后实际操作并确认以下功能正常：

| 实物验收项目 | 结果 |
| --- | --- |
| 横屏中文、像素小人、演示模式与初始精力 100 | 通过 |
| A 键开始、暂停不掉精力、继续 | 通过 |
| 正常拿起不误触发；摇动加精力及加油提示 | 通过 |
| 持续摇动不重复加分；停稳后可再次触发 | 通过 |
| B 键休息、长按切换演示／日常模式 | 通过 |
| 休息充电、充满提示及演示模式耗尽表现 | 通过 |
| 耗尽和充满各轻响一次、停留不重复鸣叫 | 通过 |

验收依据为用户逐项操作反馈，结构化记录见 `output/worker_powerbank/hardware-status.json`，烧录证据见 `output/worker_powerbank/upload.log`。一分钟讲解与操作稿见 [DEMO.md](DEMO.md)。

## 怎么玩

| 操作 / 状态 | 效果 |
| --- | --- |
| 开机 | 演示模式，待机，精力 100，按 A 开始 |
| 待机、休息或耗尽时短按 A | 新一轮专注，精力重置 100 |
| 专注时短按 A | 暂停，精力保持不变；再短按 A 继续 |
| 专注明显来回摇动 | 精力 +20，上限 100，振作动画与「还能再撑一下！」持续 1.5 秒 |
| 短按 B | 进入休息，保留当前精力，逐渐恢复 |
| 精力耗尽 | 小人趴下，显示「人类需要充电」，轻响一次，等待 A 或 B |
| 休息充满 | 显示「满血复活」，轻响一次，停留在休息画面 |
| 长按 B 约 1.2 秒 | 切换演示／日常模式，回到待机和精力 100；松手不会误触发休息 |

| 模式 | 100 → 0 的工作时间 | 0 → 100 的休息时间 |
| --- | --- | --- |
| 演示（开机默认） | 20 秒 | 10 秒 |
| 日常 | 25 分钟 | 5 分钟 |

摇动增加的精力会继续消耗：演示模式一次完整 +20 可多工作 4 秒，日常可多工作 5 分钟。接近 100 时按上限计算实际增加量。模式不写入闪存，重新开机仍为演示模式。

## 一分钟现场展示

开始前置于「演示模式」待机、精力 100。以下时间为近似值，以屏幕状态为准；详细台词见 [DEMO.md](DEMO.md)。

1. **0–5 秒**：介绍「精力」是游戏数值，5 秒时按 A 开始。
2. **5–12 秒**：展示掉精力，10 秒时按 A 暂停，12 秒时再按 A 继续。
3. **12–16 秒**：精力低于 80 后来回摇两三下，触发一次加油提示，随后停止摇动。
4. **16–33 秒**：展示表情逐渐疲惫，约 31 秒耗尽趴下，听一次提示音。
5. **33–47 秒**：按 B 休息，约 43 秒满血复活并轻响；再按 A 展示新一轮精力 100。
6. **47–60 秒**：长按 B 切到日常，按 A 展示 25:00；再长按 B 回演示待机，收尾。

## 当前开发环境

本机已安装并复用：Arduino CLI 1.5.1、Espressif ESP32 core 3.3.10-cn、M5Unified 0.2.27、M5GFX 0.2.34。版本与实际构建记录见 `output/worker_powerbank/environment.txt`。

使用已安装的 **ESP32S3 Dev Module** 板型，参数保存在 `worker_powerbank/sketch.yaml`：

```text
esp32:esp32:esp32s3:FlashMode=qio,FlashSize=8M,PartitionScheme=default_8MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,UploadMode=default,UploadSpeed=921600
```

对应 8MB Flash、QIO 80MHz、8MB Octal PSRAM、Hardware CDC/JTAG、USB CDC On Boot。此 core 的 `FlashMode=qio` 已包含 80MHz，没有独立 `FlashFreq` 菜单。官方资料确认芯片是 ESP32-S3-PICO-1-N8R8；并非 M5StickC 或 M5StickC Plus。官方 Arduino 教程也可使用 M5Stack 板卡包的 M5StickS3，本项目复用参数等价的通用 S3 板型，无需再安装另一套核心。

屏幕、IMU、按钮、电源与音频由 M5Unified/M5GFX 的 StickS3 实现初始化，没有手动套用其他板子的引脚。官方对应关系：

| 硬件 | 参数 |
| --- | --- |
| LCD | ST7789P3，135×240，程序横屏 240×135；MOSI39 / SCK40 / DC45 / CS41 / RST21 / BL38 |
| A / B | GPIO11 / GPIO12 |
| IMU | BMI270，SDA47 / SCL48，地址 0x68 |
| 音频 | ES8311 + AW8737，MCLK18 / BCLK17 / LRCK15 / DATA14，由库处理功放 |

参考：[M5Stack StickS3 官方硬件与引脚资料](https://docs.m5stack.com/en/core/StickS3)、[官方 Arduino 指南](https://docs.m5stack.com/en/arduino/m5sticks3/program)。

## 在电脑上重新验证

在本项目目录执行：

```bash
bash tools/build-worker.sh
```

它先跑电脑端状态机和摇动测试（含 AddressSanitizer / UndefinedBehaviorSanitizer），然后编译完整 ESP32-S3 固件。结果分别保存在：

- `output/worker_powerbank/tests.log`：逻辑测试结果。
- `output/worker_powerbank/fonts.log`：真实字库字形覆盖与文字宽度检查结果。
- `output/worker_powerbank/compile.log`：完整编译结果与 Flash/RAM 占用。
- `output/worker_powerbank/verification.json`：构建时间、编译参数与源码／固件 SHA-256。
- `output/worker_powerbank/firmware/`：应用、bootloader、分区、merged 固件。
- `output/worker_powerbank/ports.json`：构建时的串口检测结果。

仅运行电脑端逻辑测试可执行 `bash tools/test-worker.sh`。

## 再次连接与上传

使用**支持数据传输**的 USB-C 线和转接头接到电脑。上传前先检测本次实际串口，再执行上传脚本。

手动操作方式：

```bash
arduino-cli board list
bash tools/upload-worker.sh /dev/cu.usbmodem实际端口
```

把示例端口替换为刚检测到的开发板 USB 串口；不要选蓝牙或 `debug-console`。脚本会重新编译，再上传当前源码对应的固件，并保留 `upload.log`。没有参数时只列端口并给出用法，不上传。

若识别不到或无法进入下载模式，按官方操作：连接 USB 后，长按**侧面的 Reset 按钮**约 2 秒，内部绿灯闪烁后松开，再重新检测端口。这个按钮与作品里的 B 键不是同一个操作。上传后如果停留在下载模式，短按 Reset 重启。

真机验收顺序：

1. 画面为横屏，中文完整，有小人、精力条、状态和模式；启动不依赖电脑串口连接。
2. A 开始／暂停／继续；B 休息；B 长按只切换一次模式，松开仍是待机。
3. 正常拿起或旋转不加精力；明显来回摇两三下加一次；持续摇不连续加分。静置约 1 秒且距上次触发满 1.8 秒后，可再次摇动。
4. 耗尽和充满时各轻响约 85ms 一次；停留时不反复响。当前音量为 40/255，真机上确认听感。
5. 从休息／耗尽按 A 都回到精力 100；日常模式显示 25:00，暂停后倒计时保持。

## 实现与验证边界

- `Game.h`：以剩余工作毫秒保存精力，避免逐帧取整误差，处理 `millis()` 回绕。
- `ShakeDetector.h`：约 100Hz 采样，去除缓慢重力变化；700ms 内至少三个反向冲击才触发，之后同时满足 1.8 秒冷却和 600ms 连续静止才重新启用。
- `Ui.h`：程序绘制像素小人、笔记本、咖啡杯和精力条；使用库内 `efontCN_16` 与 `efontCN_12` 中文字库，无外部图片／字体下载。
- 主循环每轮处理按钮、计时和传感器，20fps 缓冲绘图，提示音异步；没有动画等待循环。

本次基础功能已通过用户实际操作验收。演示模式充放电、屏幕、按钮、摇动和声音均正常；日常模式完整 25 分钟／5 分钟计时由电脑逻辑测试验证，未进行整轮实物计时。
