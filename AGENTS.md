# AGENTS.md

本文件为在此仓库中工作的 AI 编码代理提供指引。内容描述仓库的**实际现状**，修改代码前请先阅读。

## 项目概况

基于 PlatformIO + Arduino 框架的 ESP32 固件工程，实现通过 MQTT 远程控制继电器 / LED，并在启动时打印芯片信息。注释与日志以中文为主，代码标识符使用英文。

## 技术栈与构建

- 构建系统：PlatformIO（`platformio.ini`），非 CMake / Makefile。
- 环境：`env:esp-wrover-kit`，唯一环境；`platform = espressif32`、`board = esp-wrover-kit`、`framework = arduino`。
- 依赖：`knolleary/PubSubClient@^2.8`（`lib_deps`）。新增第三方库必须写入 `lib_deps`，不要手动 vendor 到 `src/`。
- 串口：`monitor_speed = 115200`（与 `Serial.begin(115200)` 一致）、`upload_speed = 921600`。

常用命令（需本机已安装 PlatformIO Core，`pio` 或 `platformio`）：

```bash
pio run                 # 编译
pio run -t upload       # 编译并烧录
pio device monitor      # 打开串口监视器（115200）
pio run -t clean        # 清理构建产物
```

## 目录结构

```
platformio.ini      # 唯一的构建配置
src/                # 全部固件源码（PlatformIO 只编译此目录）
  main.cpp          # setup()/loop()、WiFi 与 MQTT 逻辑
  ChipInfo.h/.cpp   # printChipInfo()：输出芯片型号、版本、核心数、无线能力、Flash 大小
include/            # 预留的头文件目录（当前仅有 PlatformIO 默认 README，未使用）
lib/                # 预留的私有库目录（当前为空）
test/               # 预留的单元测试目录（当前为空）
.pio/               # 构建产物与 libdeps 缓存 —— 已被 .gitignore 忽略，不要提交或手工编辑
```

说明：头文件目前与实现一起放在 `src/`（如 `ChipInfo.h`）。PlatformIO 会自动把 `src/` 加入头文件搜索路径，因此 `#include "ChipInfo.h"` 可直接工作；`include/` 目录尚未启用，不要无谓地搬动文件。

## 代码约定

- 沿用现有的 C++/Arduino 风格：4 空格缩进、`#ifndef` 头文件保护（见 `ChipInfo.h`）。
- 每个功能模块成对提供 `.h` + `.cpp`，头文件中写声明、`.cpp` 中写实现，与 `ChipInfo` 保持一致。
- 引脚与常量定义在文件顶部，宏用全大写（`LED2`），运行期常量用 `const`（`relayPin`）。
- 面向用户的串口输出可用中文；`Serial.printf` 的格式串使用 `%s` / `%d` / `%u`，注意 `uint32_t` 应使用 `%u`。
- 函数与文件补充简洁的中文注释说明用途，与现有风格一致。

## 硬件与运行时行为（修改前务必了解）

- 出厂引脚定义：板载 LED `LED2 = GPIO2`，继电器 `relayPin = GPIO13`。
- `loop()` 当前以 1 秒为周期翻转 GPIO13（继电器），并同时打印 `13 ON` / `13 OFF`。**该逻辑是阻塞式的**，会拖慢 MQTT 收发；如需改成非阻塞，请用 `millis()` 时间戳替换 `delay()`。
- MQTT 回调约定：负载为 `onled2` 时点亮 GPIO2，为 `off` 时熄灭，其他内容仅打印。注意两者命名不对称（`onled2` 而非 `on`），修改时保持向后兼容或同步更新设备端与 broker 端的发布内容。
- 客户端 ID 由 `esp32s2-` 前缀加 `WiFi.macAddress()` 生成，用于避免多设备抢占同一会话。
- `connectToMqttBroker()` 是**阻塞重试循环**（失败后 `delay(2000)` 重试），在 `setup()` 和 `loop()` 中都会被调用。

## 敏感信息与配置

`src/main.cpp` 中的 WiFi SSID / 密码、MQTT broker 地址、用户名与密码目前是硬编码的明文占位值（如 `ssid = "kkk"`、`password = "11223344"`、`mqtt_broker = "192.168.139.206"`）。

- **不要**把真实的生产凭据提交到仓库。
- 需要提供真实配置时，优先改造为从独立的、被 git 忽略的头文件（例如 `src/secrets.h` 并加入 `.gitignore`）读取，或使用 `build_flags` / 环境变量注入。

## 验证方式

仓库当前**没有自动化测试**（`test/` 为空，未配置测试框架），因此改动后必须人工验证：

1. `pio run` 确认编译通过（无新增警告）。
2. `pio run -t upload` 烧录，`pio device monitor` 观察串口是否打印芯片信息、WiFi 与 MQTT 连接日志。
3. 向 `TOPIC1` 发布 `onled2` / `off`，确认 GPIO2 与串口输出符合预期。

请不要声称"已通过测试"，除非确实执行了上述步骤。

## 注意事项

- 只提交 `src/`、`platformio.ini` 等源码与配置；`.pio/` 属于构建缓存，始终保持在 `.gitignore` 中。
- 修改 `platformio.ini` 中的 board 或 platform 时，需同步检查引脚定义（GPIO2 / GPIO13 仅对当前板卡成立）。
- 该工程有 `.idea/` 目录（CLion/IntelliJ 工程文件），属于本地 IDE 配置，一般不需要改动。
