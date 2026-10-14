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
platformio.ini          # 构建配置（含 esp32 与 native 两个环境，见 default_envs 说明）
src/                    # 依赖 Arduino 的固件代码
  main.cpp              # setup()/loop()、WiFi 与 MQTT 逻辑、回调派发
  ChipInfo.h/.cpp       # printChipInfo()：输出芯片型号、版本、核心数、无线能力、Flash 大小
lib/PureLogic/          # 纯逻辑库：不依赖 Arduino，可在 PC 上编译与测试
  CommandParser.h/.cpp  #   payload -> Command
  RelayController.h/.cpp#   输出状态机 + 时间驱动翻转（写引脚可注入）
test/                   # PC 上运行的单元测试（21 个用例）
  test_setup_teardown.cpp   # Unity 运行器要求的 setUp/tearDown 空实现
  test_command_parser/      # 11 个用例
  test_relay_controller/    # 10 个用例
include/                # 预留的头文件目录（当前仅有 PlatformIO 默认 README，未使用）
.pio/                   # 构建产物与 libdeps 缓存 —— 已被 .gitignore 忽略，不要提交或手工编辑
```

说明：`ChipInfo.h` 与实现一起放在 `src/`，PlatformIO 会自动把 `src/` 加入头文件搜索路径，因此 `#include "ChipInfo.h"` 可直接工作；`include/` 目录尚未启用，不要无谓地搬动文件。**而纯逻辑模块必须放 `lib/PureLogic/`**，原因见「单元测试」一节。

## 代码约定

- 沿用现有的 C++/Arduino 风格：4 空格缩进、`#ifndef` 头文件保护（见 `ChipInfo.h`）。
- 每个功能模块成对提供 `.h` + `.cpp`，头文件中写声明、`.cpp` 中写实现。
- 引脚与常量定义在文件顶部，宏用全大写（`LED2`），运行期常量用 `const`（`relayPin`）。
- 面向用户的串口输出可用中文；`Serial.printf` 的格式串使用 `%s` / `%d` / `%u`，注意 `uint32_t` 应使用 `%u`。
- 函数与文件补充简洁的中文注释说明用途，与现有风格一致。
- **新增纯逻辑时放到 `lib/PureLogic/` 并配套测试**，不要在 `src/` 里写业务逻辑。`lib/PureLogic/` 内**禁止** `#include <Arduino.h>`。
- 修改 `lib/PureLogic/` 的行为后，必须同步更新 `test/` 下对应用例并实际运行 `pio test -e native`。

## 硬件与运行时行为（修改前务必了解）

- 代码中的引脚定义：板载 LED `LED2 = GPIO2`，继电器 `relayPin = GPIO13`。
- `loop()` 现在是**纯非阻塞**的：调用 `led.update(millis())`（时间驱动）、MQTT 重连检查、`client.loop()`。早先那对 `delay(1000)` 已删除。
- 继电器默认**不会**自动动作（`RELAY_BLINK_PERIOD_MS = 0`）；如需心跳指示可把该常量改为非 0，翻转由 `RelayController::update()` 按时间戳驱动，不阻塞。
- MQTT 回调约定：负载为 `onled2` 点亮 GPIO2，为 `off` 熄灭，其他内容静默忽略（不再回显）。注意两者命名不对称（`onled2` 而非 `on`），修改时保持向后兼容或同步更新设备端与 broker 端。
- 解析规则由 `lib/PureLogic/CommandParser.cpp` 定义，**区分大小写**、要求长度完全相等；有 11 个用例覆盖边界情况，改规则请同步改测试。
- 客户端 ID 由 `esp32-` 前缀加 `WiFi.macAddress()` 生成，用于避免多设备抢占同一会话。
- `connectToMqttBroker()` 是**阻塞重试循环**（失败后 `delay(2000)` 重试），在 `setup()` 和 `loop()` 中都会被调用。
- **GPIO2 是 ESP32 的 strapping 引脚**（ESP-IDF GPIO 文档明确标注：GPIO0、GPIO2、GPIO5、GPIO12、GPIO15 为 strapping 引脚），启动时被采样以决定启动模式，复位时默认内部下拉。把它当普通输出在启动**之后**使用通常没问题（官方 blink 示例即如此），但若外部电路在复位瞬间把它**拉高**，可能干扰启动——加继电器/LED 外围电路时需留意。
- `GPIO13` 在通用 ESP32 上是普通可用引脚（标注为 JTAG 用途，非 strapping），但**在真实 WROVER-KIT 上被 JTAG 与 microSD 共用**（见上节）。

## 已知问题清单（审查结论，修复前请先确认预期）

以下问题按严重程度排列，均可在源码中直接定位。**不要把它们当作"设计如此"而忽略**，但修复前应先与维护者确认预期行为，因为其中几条涉及产品意图：

1. **凭据明文硬编码**（`src/main.cpp`）：WiFi 与 MQTT 的账号密码直接写在源码里。**这是当前最优先、且尚未处理的问题**，详见下一节。
2. ~~继电器无法通过 MQTT 控制~~ —— **已修复**：删除了 `loop()` 中的无条件翻转。
3. ~~`loop()` 阻塞~~ —— **已修复**：已移除两个 `delay(1000)`，改为 `millis()` 时间驱动。
4. **MQTT 无重连退避**：`setup()` 中若 broker 不可达会**永久卡在 `connectToMqttBroker()`**，设备表现为"死机"且不重试 WiFi。`loop()` 中的重连也无指数退避，会每 2 秒无限重试。**尚未处理。**
5. **WiFi 连接无超时**：`while (WiFi.status() != WL_CONNECTED)` 无超时、无失败提示，WiFi 不可用时会永久卡住。**尚未处理。**
6. ~~回调无边界校验~~ —— **已修复**：`parseCommand()` 按长度解析并校验 `nullptr`，不再逐字节拼 `String`；新增 `test_command_parser` 覆盖该风险。
7. **无遗嘱/状态上报**：固件不发布任何主题，设备离线或执行结果对 broker 侧不可见。**尚未处理。**
8. ~~`client_id` 前缀型号错误~~ —— **已修复**：`esp32s2-` 已改为 `esp32-`。
9. ~~串口措辞与功能不一致~~ —— **已修复**：日志中的 "public mqtt broker" 已改为 "mqtt broker"。

## 敏感信息与配置

`src/main.cpp` 中的 WiFi SSID / 密码、MQTT broker 地址、用户名与密码目前是硬编码的明文占位值（如 `ssid = "kkk"`、`password = "11223344"`、`mqtt_broker = "192.168.139.206"`）。

- **不要**把真实的生产凭据提交到仓库。
- 需要提供真实配置时，优先改造为从独立的、被 git 忽略的头文件（例如 `src/secrets.h` 并加入 `.gitignore`）读取，或使用 `build_flags` / 环境变量注入。

## 验证方式

**改动的验证分两层，两层都要做：**

**第一层：PC 上的自动化测试（快，秒级，无需开发板）**

```bash
pio test -e native
```

改动 `lib/PureLogic/` 后**必须**跑这一步，当前为 21 个用例全部通过。若新增了纯逻辑而未加测试，视为改动未完成。

**第二层：真机人工验证（涉及硬件/网络，无法自动化）**

1. `pio run` 确认编译通过（无新增警告）。
2. `pio run -t upload` 烧录，`pio device monitor` 观察串口是否打印芯片信息、WiFi 与 MQTT 连接日志。**首次连接新板子时，先看 `Chip model:` 这一行确认实际芯片型号。**
3. 向 `TOPIC1` 发布 `onled2` / `off`，确认 GPIO2 与串口输出符合预期。
4. 若改动了继电器相关逻辑，确认 MQTT 命令的响应延迟仍在可接受范围。

**注意：** 第二层无法自动化，因为仓库使用的是 `native` PC 测试而非上板测试。因此**不要声称"已在真机验证"**，除非确实烧录并观察过串口；同理，`pio test -e native` 通过**不等于**固件在硬件上工作正常。

### 单元测试的运行前提与配置约束

**工具链前提（本机已装好）：** PlatformIO 的 native 平台不自带工具链，依赖系统 `gcc`。本机原先没有 gcc，已安装 **MinGW-w64 GCC 14.2.0** 到：

```
%LOCALAPPDATA%\mingw64\mingw64\bin
```

并已加入**用户级 PATH**（新开的终端自动生效）。若在别的机器上克隆本仓库，需先自行安装 gcc，否则 `pio test -e native` 会报 `'gcc' is not recognized`；注意**这不会影响 `pio run`**（见下）。

**两个关键的配置约束，改动时务必保留：**

1. **`default_envs = esp-wrover-kit` 必须保留。** 否则 `pio run`（不带 `-e`）会连 native 环境一起构建，在没有 gcc 的机器上**直接失败**——即"加测试反而弄坏了构建"。已实测：移除 gcc 后 `pio run` 仍能正常成功。
2. **纯逻辑模块必须放在 `lib/PureLogic/`，不要放回 `src/`。** PlatformIO 只对 `lib/` 下的库做自动编译与包含，且 `src/main.cpp`、`src/ChipInfo.cpp` 依赖 `WiFi.h` / `Arduino.h`，native 环境无法编译它们。若把这些模块移回 `src/` 并打开 `test_build_src`，`pio test -e native` 会因找不到 `Arduino.h` / `WiFi.h` 而失败（已实际踩到）。

**测试目录结构：**

```
lib/PureLogic/            # 纯逻辑库（不依赖 Arduino，可在 PC 编译）
  CommandParser.h/.cpp    #   payload -> Command
  RelayController.h/.cpp  #   时间驱动 + 注入式写引脚
test/
  test_setup_teardown.cpp # PlatformIO Unity 运行器要求的 setUp/tearDown 空实现
  test_command_parser/    # 11 个用例
  test_relay_controller/  # 10 个用例
```

关于"上板测试"（`pio test -e esp-wrover-kit`）：PlatformIO 的测试构建会**替换掉 `src/main.cpp`**，Arduino 的 `main.cpp` 因而找不到 `setup()`/`loop()`，报 `undefined reference to 'setup()'`。本工程当前**只支持 PC 上的 native 测试**；若将来要做上板测试，需要在测试中自行提供 `setup()`/`loop()` 桩函数。

## 模块化结构（已实施）

原先所有逻辑挤在 `main.cpp` 里，任何一部分都无法在不碰硬件的情况下测试。现已完成解耦：

| 模块 | 位置 | 职责 | 可测性 |
| --- | --- | --- | --- |
| `CommandParser` | `lib/PureLogic/` | payload（字节 + 长度）→ `Command` 枚举 | **纯函数，11 个用例** |
| `RelayController` | `lib/PureLogic/` | 输出状态 + 时间驱动翻转，写引脚可注入 | **纯逻辑，10 个用例**（假时间戳，不真的等待） |
| `main.cpp` | `src/` | WiFi/MQTT 连接、回调派发、`loop()` | 需硬件 |
| `ChipInfo` | `src/` | 芯片信息打印 | 无需测试 |

**分层原则（新增代码请遵循）：**

- **纯逻辑（无 Arduino 依赖、无动态分配、无阻塞）一律放 `lib/PureLogic/`**，这样它天然可在 PC 上被测试。判断标准很简单：能不能只 `#include <stddef.h>` 就写完？能，就放 `lib/`。
- **副作用集中在 `src/`**：`digitalWrite`、`Serial`、`WiFi`、`client.loop()` 只出现在 `src/` 与注入的回调里。
- 需要与硬件交互的纯逻辑，用**注入回调**（见 `RelayController` 的 `WritePinFn`）而不是直接调用 `digitalWrite`。

### 本次重构改动的行为（重要）

1. **删除了 `loop()` 中无条件的继电器闪烁。** 经确认该逻辑属于调试遗留：它既用 `delay(1000)` 阻塞主循环（使 MQTT 下行延迟可达约 2 秒），又让继电器完全无法被 MQTT 控制。现在继电器引脚上电后保持 LOW，`loop()` 为纯非阻塞。
   - 如将来确实需要心跳指示，把 `RELAY_BLINK_PERIOD_MS` 改成非 0 即可（已保留该能力，且它是非阻塞的、会被 MQTT 命令覆盖）。
2. **回调不再把 payload 逐字节拼进 `String`。** 改为 `parseCommand()` 按长度解析，因此顺带修掉了原先"无长度上限 / 无 `nullptr` 校验 / 堆分配"的风险（原问题清单第 6 条）。
3. **未知命令不再回显 payload 内容**，只静默忽略，避免把任意内容写进串口。

## 注意事项

- 只提交 `src/`、`lib/`、`test/`、`platformio.ini` 等源码与配置；`.pio/` 属于构建缓存，始终保持在 `.gitignore` 中。
- 修改 `platformio.ini` 中的 board 或 platform 时，需同步检查引脚定义（GPIO2 / GPIO13 仅对当前假定板卡成立，且该假定本身存疑，见上）。
- 该工程有 `.idea/` 目录（CLion/IntelliJ 工程文件，已包含 PlatformIO 插件配置），属于本地 IDE 配置，一般不需要改动。
- `lib/PureLogic/` 是**唯一**允许存放跨平台纯逻辑的地方，不要把 Arduino 依赖引入其中，否则 PC 测试会立刻失效。
