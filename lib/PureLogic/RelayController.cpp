#include "RelayController.h"

RelayController::RelayController(int pin, unsigned long blinkPeriodMs, WritePinFn writePin)
    : pin_(pin),
      blinkPeriodMs_(blinkPeriodMs),
      writePin_(writePin),
      state_(false),
      blinkEnabled_(false),
      lastToggleMs_(0),
      hasTicked_(false) {
    applyState(false);
}

void RelayController::applyState(bool on) {
    state_ = on;
    if (writePin_ != nullptr) {
        writePin_(pin_, on);
    }
}

void RelayController::setBlinkEnabled(bool enabled) {
    blinkEnabled_ = enabled;
    hasTicked_ = false; // 重新对齐时间基准
    if (!enabled) {
        applyState(false); // 停用闪烁时保持关闭，避免停在不确定状态
    }
}

void RelayController::setDesiredState(bool on) {
    // 被 MQTT 显式控制时，心跳闪烁让位给显式命令。
    blinkEnabled_ = false;
    applyState(on);
}

void RelayController::toggle() {
    applyState(!state_);
}

void RelayController::update(unsigned long nowMs) {
    if (!blinkEnabled_ || blinkPeriodMs_ == 0) {
        return;
    }

    if (!hasTicked_) {
        // 第一次调用只记录基准时间，不翻转，避免开机瞬间立刻动作。
        lastToggleMs_ = nowMs;
        hasTicked_ = true;
        return;
    }

    if (nowMs - lastToggleMs_ >= blinkPeriodMs_) {
        lastToggleMs_ = nowMs;
        applyState(!state_);
    }
}
