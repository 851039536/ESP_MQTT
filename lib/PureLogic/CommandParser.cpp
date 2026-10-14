#include "CommandParser.h"

namespace {

// 命令字面量集中在此，便于与测试用例对照。
const char kLedOn[] = "onled2";
const char kLedOff[] = "off";

// 按长度比较，不要求 payload 以 '\0' 结尾。
// 长度不等直接判否，因此不会越界读取。
bool matches(const char *payload, size_t length, const char *literal, size_t literalLength) {
    if (length != literalLength) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        if (payload[i] != literal[i]) {
            return false;
        }
    }
    return true;
}

} // namespace

Command parseCommand(const char *payload, size_t length) {
    if (payload == nullptr || length == 0) {
        return Command::Unknown;
    }

    if (matches(payload, length, kLedOn, sizeof(kLedOn) - 1)) {
        return Command::LedOn;
    }
    if (matches(payload, length, kLedOff, sizeof(kLedOff) - 1)) {
        return Command::LedOff;
    }
    return Command::Unknown;
}

const char *commandName(Command command) {
    switch (command) {
        case Command::LedOn:
            return "LED_ON";
        case Command::LedOff:
            return "LED_OFF";
        case Command::Unknown:
        default:
            return "UNKNOWN";
    }
}
