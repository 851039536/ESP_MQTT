// RelayController 单元测试（在 PC 上运行，无需开发板）。
//
// 关键点：写引脚通过注入的假回调捕获，时间通过假时间戳驱动，
// 因此既不碰真实 GPIO，也不真的 sleep。

#include <unity.h>

#include "RelayController.h"

namespace {

// 假引脚：记录每一次写操作
struct FakePin {
    int lastPin = -1;
    bool lastValue = false;
    int writeCount = 0;
};

FakePin g_pin;

void fakeWritePin(int pin, bool on) {
    g_pin.lastPin = pin;
    g_pin.lastValue = on;
    g_pin.writeCount++;
}

void resetFakePin() {
    g_pin = FakePin();
}

} // namespace

// ---------- 基础行为 ----------

void test_initial_state_is_off(void) {
    resetFakePin();
    RelayController relay(13, 0, fakeWritePin);
    TEST_ASSERT_FALSE(relay.state());
    TEST_ASSERT_EQUAL_INT(13, g_pin.lastPin);
}

void test_set_desired_state_drives_pin(void) {
    resetFakePin();
    RelayController relay(13, 0, fakeWritePin);

    relay.setDesiredState(true);
    TEST_ASSERT_TRUE(relay.state());
    TEST_ASSERT_TRUE(g_pin.lastValue);
    TEST_ASSERT_EQUAL_INT(13, g_pin.lastPin);

    relay.setDesiredState(false);
    TEST_ASSERT_FALSE(relay.state());
    TEST_ASSERT_FALSE(g_pin.lastValue);
}

void test_toggle_flips(void) {
    resetFakePin();
    RelayController relay(13, 0, fakeWritePin);

    relay.toggle();
    TEST_ASSERT_TRUE(relay.state());
    relay.toggle();
    TEST_ASSERT_FALSE(relay.state());
}

void test_null_write_pin_does_not_crash(void) {
    // 允许注入 nullptr（纯逻辑测试用），内部应安全地跳过写操作
    resetFakePin();
    RelayController relay(13, 0, nullptr);
    relay.setDesiredState(true);
    TEST_ASSERT_TRUE(relay.state()); // 逻辑状态仍然更新
    relay.update(1000);
    TEST_ASSERT_TRUE(relay.state());
}

// ---------- 心跳闪烁：用假时间戳验证，不真的等待 ----------

void test_blink_disabled_never_toggles(void) {
    resetFakePin();
    RelayController relay(13, 1000, fakeWritePin);
    relay.setBlinkEnabled(false);

    for (unsigned long t = 0; t <= 10000; t += 500) {
        relay.update(t);
    }
    TEST_ASSERT_FALSE(relay.state());
}

void test_blink_first_tick_does_not_toggle(void) {
    resetFakePin();
    RelayController relay(13, 1000, fakeWritePin);
    relay.setBlinkEnabled(true);

    relay.update(0); // 仅对齐时间基准
    TEST_ASSERT_FALSE(relay.state());
}

void test_blink_toggles_at_period(void) {
    resetFakePin();
    RelayController relay(13, 1000, fakeWritePin);
    relay.setBlinkEnabled(true);

    relay.update(0);      // 基准
    relay.update(999);    // 未到周期
    TEST_ASSERT_FALSE(relay.state());

    relay.update(1000);   // 到周期 -> 翻转
    TEST_ASSERT_TRUE(relay.state());

    relay.update(1500);   // 周期内
    TEST_ASSERT_TRUE(relay.state());

    relay.update(2000);   // 再翻转
    TEST_ASSERT_FALSE(relay.state());
}

void test_blink_handles_long_gap_without_burst(void) {
    // 若两次 update 间隔远超周期，也只翻转一次（不做补偿式连翻），
    // 否则一次卡顿后会瞬间翻转多次。
    resetFakePin();
    RelayController relay(13, 1000, fakeWritePin);
    relay.setBlinkEnabled(true);

    relay.update(0);
    relay.update(100000); // 巨大的时间跳跃
    TEST_ASSERT_TRUE(relay.state());

    relay.update(100001); // 刚翻转完，不应再次翻转
    TEST_ASSERT_TRUE(relay.state());
}

void test_mqtt_command_overrides_blink(void) {
    // 心跳开着时收到显式命令，应以显式命令为准并停止闪烁
    resetFakePin();
    RelayController relay(13, 1000, fakeWritePin);
    relay.setBlinkEnabled(true);
    relay.update(0);

    relay.setDesiredState(true);
    TEST_ASSERT_TRUE(relay.state());
    TEST_ASSERT_FALSE(relay.blinkEnabled());

    // 停止闪烁后，时间继续走也不应再翻转
    relay.update(5000);
    TEST_ASSERT_TRUE(relay.state());
}

void test_zero_period_blink_is_noop(void) {
    resetFakePin();
    RelayController relay(13, 0, fakeWritePin);
    relay.setBlinkEnabled(true);

    for (unsigned long t = 0; t <= 5000; t += 100) {
        relay.update(t);
    }
    TEST_ASSERT_FALSE(relay.state());
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_initial_state_is_off);
    RUN_TEST(test_set_desired_state_drives_pin);
    RUN_TEST(test_toggle_flips);
    RUN_TEST(test_null_write_pin_does_not_crash);
    RUN_TEST(test_blink_disabled_never_toggles);
    RUN_TEST(test_blink_first_tick_does_not_toggle);
    RUN_TEST(test_blink_toggles_at_period);
    RUN_TEST(test_blink_handles_long_gap_without_burst);
    RUN_TEST(test_mqtt_command_overrides_blink);
    RUN_TEST(test_zero_period_blink_is_noop);
    return UNITY_END();
}
