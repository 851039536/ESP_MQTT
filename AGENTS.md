# AGENTS.md

本文件为在此仓库中工作的 AI 编码代理提供指引。内容描述仓库的**实际现状**，修改代码前请先阅读。

## 项目概况

基于 PlatformIO + Arduino 框架的 ESP32 固件工程，实现通过 MQTT 远程控制继电器 / LED，并在启动时打印芯片信息。注释与日志以中文为主，代码标识符使用英文。

### 这个项目实际是做什么的

一句话：**一块 ESP32 通过 MQTT 订阅命令，按命令开关一个继电器和一颗 LED；同时它自己还会每秒无条件地"咔哒"一次继电器。**

从代码（`src/main.cpp`，105 行）看，运行时行为有三条主线：

1. **启动流程**（`setup()`）：初始化 GPIO → 打开串口 → 连 WiFi（阻塞等待直到连上）→ 打印芯片信息 → 连接 MQTT 并订阅 `TOPIC1`。
2. **MQTT 下行控制**（`callback()`）：收到 `onled2` 点亮 GPIO2，收到 `off` 熄灭 GPIO2，其他内容只打印。**全部通过串口日志反馈，固件没有任何状态回传/上报主题。**
3. **无条件的继电器闪烁**（`loop()`）：每 1 秒翻转一次 GPIO13，与 MQTT 无关、无法通过 MQTT 停止。

典型的"MQTT 点灯 + 继电器"入门/练习工程：`ChipInfo` 用于验证开发板是否正常工作，`onled2` 用于验证 MQTT 下行链路是否打通。

**关于项目成熟度的判断：** 这是一份可运行的原型 / 教学代码，不是可交付的产品固件。判断依据：凭据明文硬编码、`loop()` 里用阻塞 `delay()`、MQTT 无重连退避、继电器被无条件循环翻转、`client_id` 前缀写错芯片型号（见下）、没有任何测试。审查这类代码时应按"原型"而非"生产"标准判断意图，但**不要**因此忽略其中真实存在的缺陷。

### 已确认的硬件目标与代码中的不一致

这点容易误判，已通过构建产物核实（证据在下）：

- **实际编译目标是经典 ESP32（双核，支持 WiFi + 蓝牙），不是 ESP32-S2。** 证据链：
  - 板级清单 `platformio/.../boards/esp-wrover-kit.json`：`"mcu": "esp32"`、`"core": "esp32"`、`extra_flags: -DARDUINO_ESP32_DEV`、`flash_size: 4MB`。
  - 工具链 SDK 配置 `framework-arduinoespressif32/tools/sdk/esp32/sdkconfig`：`CONFIG_IDF_TARGET="esp32"`、`CONFIG_IDF_TARGET_ESP32=y`、`CONFIG_IDF_TARGET_ARCH_XTENSA=y`、`CONFIG_BT_ENABLED=y`、`CONFIG_ESP32_SPIRAM_SUPPORT=y`。
  - 已编译的 `firmware.map` 引用 `framework-arduinoespressif32\tools\sdk\esp32\lib\...`。
  - Arduino-ESP32 内核版本：`3.20017.241212+sha.dcc1105b`（即 **2.0.17**，2.x 系列末版）。
  - ⇒ `ChipInfo.cpp` 中 `Serial.printf("Chip model: %s\n", CONFIG_IDF_TARGET)` 实际会打印 **`Chip model: esp32`**。
- **但 `src/main.cpp:77` 的 `String client_id = "esp32s2-";` 暗示作者以为自己在用 ESP32-S2。** 注释与文档中不要沿用这个错误型号；它只是 MQTT 客户端 ID 的一段前缀。
  - 来源已查明：Arduino-ESP32 的默认**主机名**正是按 `CONFIG_IDF_TARGET + "-" + MAC 后三字节` 拼接（`esp32-XXXXXX` / `esp32s2-XXXXXX`），作者多半是照抄了这个模式，却手写成了 `esp32s2-`。注意 `WiFi.macAddress()` 本身返回的是 `D8:A0:1D:0B:88:4C` 这种 6 字节大写冒号分隔串，**不含** `esp32s2-` 前缀，两者是不同的字段。
  - ESP32-S2 是**单核**、**完全没有蓝牙（BT Classic 与 BLE 都没有）**、仅 WiFi 的芯片，与上面 SDK 配置中 `CONFIG_BT_ENABLED=y` 直接矛盾，因此可确认当前固件**不是**为 ESP32-S2 编译的。
- **`board = esp-wrover-kit` 与代码描述的实物板卡不符。** 证据与推论：
  - 已验证 PlatformIO 的 `esp-wrover-kit.json` 清单内容：`"mcu": "esp32"`、`"core": "esp32"`、`extra_flags: -DARDUINO_ESP32_DEV`、`flash_size: 4MB`、`f_cpu: 240MHz`、`flash_mode: dio`。
  - 关键点：该清单与 `esp32dev.json`、`nodemcu-32s.json` **在编译参数上完全等价**（mcu / flash / 频率 / 宏 / variant 均相同，仅调试探针、hwids、openocd 配置和名称不同）。⇒ **`board` 选项本身不构成实物板卡的证据**，改成 `esp32dev` 也不会改变编译出的 MCU 与 Flash 规格。
  - 但**按真实 ESP-WROVER-KIT 的接线，代码写的引脚是错的**：官方用户指南中板载 LED 是**三引脚 RGB LED（非可寻址）**，`GPIO0`=红、**`GPIO2`=绿**、`GPIO4`=蓝 —— 即 GPIO2 在 WROVER-KIT 上是**绿色**通道，不是"蓝色 LED"。同时 `GPIO13` 在 WROVER-KIT 上被 JTAG 的 `MTCK/TCK` 与 microSD 的 `CD/DATA3` 共用。
  - 而"GPIO2 上的单色蓝灯 + GPIO13 上的继电器"是**廉价 ESP32-WROOM-32 DevKit / NodeMCU-32S** 的典型特征。
  - ⇒ 最可能的真相：**实物是 WROOM-32 DevKit / NodeMCU-32S 一类的通用板，只是 `platformio.ini` 里填了 `esp-wrover-kit`**。因为编译参数等价，固件仍能正常运行，所以这个错误一直没有暴露。
  - **动手改引脚前，请先确认实物的真实型号**；`printChipInfo()` 的串口输出是最快的确认手段（但它只能区分芯片、区分不了板卡）。若确认是 DevKit 类板卡，建议把 `board` 改为 `esp32dev` 或 `nodemcu-32s` 以免误导。
  - 补充：ESP-WROVER-KIT 在 Espressif 官方文档中已标记为 **EOL（停产）**。

> 说明：以上"GPIO2 = 绿色、GPIO4 = 蓝色"来自 Espressif 官方 WROVER-KIT 用户指南的 RGB LED 表与引脚分配表；`esp-wrover-kit.json` 与 `esp32dev.json` 的等价性来自对两个清单的直接比对。"廉价 DevKit 用 GPIO2 做板载蓝灯"属于广泛的社区/厂商惯例，**并非 Espressif 官方文档保证**，不同板子可能不同，请以实物为准。

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

- 代码中的引脚定义：板载 LED `LED2 = GPIO2`，继电器 `relayPin = GPIO13`。
- `loop()` 当前以 1 秒为周期**无条件**翻转 GPIO13（继电器）并打印 `13 ON` / `13 OFF`。**该逻辑是阻塞式的**，会拖慢 MQTT 收发；如需改成非阻塞，请用 `millis()` 时间戳替换 `delay()`。
- MQTT 回调约定：负载为 `onled2` 时点亮 GPIO2，为 `off` 时熄灭，其他内容仅打印。注意两者命名不对称（`onled2` 而非 `on`），修改时保持向后兼容或同步更新设备端与 broker 端的发布内容。
- 客户端 ID 由 `esp32s2-` 前缀加 `WiFi.macAddress()` 生成，用于避免多设备抢占同一会话（该前缀的型号含义有误，见上节）。
- `connectToMqttBroker()` 是**阻塞重试循环**（失败后 `delay(2000)` 重试），在 `setup()` 和 `loop()` 中都会被调用。
- **GPIO2 是 ESP32 的 strapping 引脚**（ESP-IDF GPIO 文档明确标注：GPIO0、GPIO2、GPIO5、GPIO12、GPIO15 为 strapping 引脚），启动时被采样以决定启动模式，复位时默认内部下拉。把它当普通输出在启动**之后**使用通常没问题（官方 blink 示例即如此），但若外部电路在复位瞬间把它**拉高**，可能干扰启动——加继电器/LED 外围电路时需留意。
- `GPIO13` 在通用 ESP32 上是普通可用引脚（标注为 JTAG 用途，非 strapping），但**在真实 WROVER-KIT 上被 JTAG 与 microSD 共用**（见上节）。

## 已知问题清单（审查结论，修复前请先确认预期）

以下问题按严重程度排列，均可在源码中直接定位。**不要把它们当作"设计如此"而忽略**，但修复前应先与维护者确认预期行为，因为其中几条涉及产品意图：

1. **凭据明文硬编码**（`src/main.cpp:14-22`）：WiFi 与 MQTT 的账号密码直接写在源码里。这是最优先处理的问题，详见下一节。
2. **继电器无法通过 MQTT 控制**（`src/main.cpp:93-98`）：`loop()` 无条件翻转继电器，MQTT 只能控制 LED。若要远程控制继电器，这条无条件循环必须去掉或改为状态驱动。
3. **`loop()` 阻塞**（`src/main.cpp:95,98`）：两个 `delay(1000)` 让 `client.loop()` 每秒最多只跑一次，下行命令延迟可达约 2 秒；MQTT keepalive 也可能因此受影响。
4. **MQTT 无重连退避**（`src/main.cpp:75-89`）：`setup()` 中若 broker 不可达会**永久卡在 `connectToMqttBroker()`**，设备表现为"死机"且不重试 WiFi。`loop()` 中的重连也无指数退避，会每 2 秒无限重试。
5. **WiFi 连接无超时**（`src/main.cpp:41-44`）：`while (WiFi.status() != WL_CONNECTED)` 无超时、无失败提示，WiFi 不可用时会永久卡住。
6. **回调无边界校验**（`src/main.cpp:58-73`）：payload 未做长度上限与 `nullptr` 校验，逐字节拼进 `String` 可能触发堆碎片或内存耗尽；PubSubClient 默认缓冲仅 256 字节。
7. **无遗嘱/状态上报**：固件不发布任何主题，设备离线或执行结果对 broker 侧不可见。
8. **`client_id` 前缀型号错误**（`src/main.cpp:77`）：`esp32s2-` 与实际的经典 ESP32 不符，仅影响可读性。
9. **`serial` 输出与功能不一致的细节**：`setup()` 打印的 "The client %s connects to the public mqtt broker" 措辞暗示是公共 broker，但实际连的是局域网私有地址 `192.168.139.206`。

## 敏感信息与配置

`src/main.cpp` 中的 WiFi SSID / 密码、MQTT broker 地址、用户名与密码目前是硬编码的明文占位值（如 `ssid = "kkk"`、`password = "11223344"`、`mqtt_broker = "192.168.139.206"`）。

- **不要**把真实的生产凭据提交到仓库。
- 需要提供真实配置时，优先改造为从独立的、被 git 忽略的头文件（例如 `src/secrets.h` 并加入 `.gitignore`）读取，或使用 `build_flags` / 环境变量注入。

## 验证方式

仓库当前**没有自动化测试**（`test/` 为空，未配置测试框架），因此改动后必须人工验证：

1. `pio run` 确认编译通过（无新增警告）。
2. `pio run -t upload` 烧录，`pio device monitor` 观察串口是否打印芯片信息、WiFi 与 MQTT 连接日志。**首次连接新板子时，先看 `Chip model:` 这一行确认实际芯片型号。**
3. 向 `TOPIC1` 发布 `onled2` / `off`，确认 GPIO2 与串口输出符合预期。
4. 若改动了 `loop()` 中的继电器逻辑，确认 MQTT 命令的响应延迟仍在可接受范围。

请不要声称"已通过测试"，除非确实执行了上述步骤。

## 注意事项

- 只提交 `src/`、`platformio.ini` 等源码与配置；`.pio/` 属于构建缓存，始终保持在 `.gitignore` 中。
- 修改 `platformio.ini` 中的 board 或 platform 时，需同步检查引脚定义（GPIO2 / GPIO13 仅对当前假定板卡成立，且该假定本身存疑，见上）。
- 该工程有 `.idea/` 目录（CLion/IntelliJ 工程文件，已包含 PlatformIO 插件配置），属于本地 IDE 配置，一般不需要改动。
