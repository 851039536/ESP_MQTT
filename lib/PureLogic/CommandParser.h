#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

// 纯逻辑模块：把 MQTT payload 解析为命令。
//
// 设计约束（刻意为之，请勿破坏）：
//   - 不 include <Arduino.h>，不使用 String / Serial 等 Arduino 专有设施，
//     只依赖 <stddef.h>，因此可用 PC 上的 gcc 直接编译并单元测试。
//   - 无动态内存分配，无全局状态，可重入。
//   - payload 可能不是以 '\0' 结尾的（MQTT 给的是长度 + 字节），
//     因此必须按长度比较，不能假设它是个 C 字符串。

#include <stddef.h>

enum class Command {
    LedOn,    // payload 为 "onled2"
    LedOff,   // payload 为 "off"
    Unknown,  // 其他任何内容（含空、超长、大小写不符）
};

// 把 payload[0..length) 解析为 Command。
// payload 为 nullptr 或 length 为 0 时返回 Command::Unknown。
// 比较区分大小写；payload 必须与命令字面量等长且完全一致才算匹配。
Command parseCommand(const char *payload, size_t length);

// 命令的可读名称，用于日志（"LED_ON" / "LED_OFF" / "UNKNOWN"）。
// 返回的是静态字符串常量，调用方不需要释放。
const char *commandName(Command command);

#endif // COMMANDPARSER_H
