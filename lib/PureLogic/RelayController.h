#ifndef RELAYCONTROLLER_H
#define RELAYCONTROLLER_H

// 继电器/LED 输出控制器：把"什么时候该翻转"的决策与"如何写引脚"的副作用分开。
//
// 设计约束（刻意为之，请勿破坏）：
//   - 不 include <Arduino.h>。写引脚通过构造时注入的回调完成，
//     因此本类可用 PC 上的 gcc 直接编译并单元测试（注入假回调即可）。
//   - update() 只做时间戳比较，不调用 delay()，不阻塞。

#include <stddef.h>

class RelayController {
public:
    // 写引脚的回调：on 为 true 表示输出高电平。
    typedef void (*WritePinFn)(int pin, bool on);

    // pin          —— 要控制的 GPIO
    // blinkPeriodMs—— 心跳翻转周期（毫秒）；传 0 表示不启用闪烁
    // writePin     —— 注入的写引脚实现；传 nullptr 时内部置为无操作（便于纯逻辑测试）
    RelayController(int pin, unsigned long blinkPeriodMs, WritePinFn writePin);

    // 启用/关闭心跳闪烁。关闭时会立即把输出拉到 on 参数指定的状态。
    void setBlinkEnabled(bool enabled);
    bool blinkEnabled() const { return blinkEnabled_; }

    // 由 MQTT 命令驱动的状态设置（LED 的实际控制入口）。
    void setDesiredState(bool on);

    // 手动立即翻转一次（供测试或按键使用）。
    void toggle();

    // 由 loop() 非阻塞调用：nowMs 为 millis() 的当前值。
    // 启用闪烁时，每满一个 blinkPeriodMs 翻转一次。
    void update(unsigned long nowMs);

    // 当前逻辑电平。
    bool state() const { return state_; }

    int pin() const { return pin_; }

private:
    void applyState(bool on);

    int pin_;
    unsigned long blinkPeriodMs_;
    WritePinFn writePin_;
    bool state_;
    bool blinkEnabled_;
    unsigned long lastToggleMs_;
    bool hasTicked_; // 首次 update 用于对齐时间基准，避免开机瞬间误翻转
};

#endif // RELAYCONTROLLER_H
