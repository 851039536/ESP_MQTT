// CommandParser 单元测试（在 PC 上运行，无需开发板）。
//
// 运行：pio test -e native

#include <unity.h>

#include "CommandParser.h"

// ---------- 正常路径 ----------

void test_parses_onled2(void) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::LedOn),
                          static_cast<int>(parseCommand("onled2", 6)));
}

void test_parses_off(void) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::LedOff),
                          static_cast<int>(parseCommand("off", 3)));
}

// ---------- 大小写敏感（原实现用 String::equals，同样区分大小写，行为保持一致）----------

void test_is_case_sensitive(void) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("ONLED2", 6)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("Off", 3)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("OFF", 3)));
}

// ---------- 边界与畸形输入（这是本次重构要修掉的风险点）----------

void test_null_payload_is_unknown(void) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand(nullptr, 0)));
    // 即便声称有长度，nullptr 也必须安全返回而不是解引用
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand(nullptr, 6)));
}

void test_empty_payload_is_unknown(void) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("", 0)));
}

void test_prefix_is_not_a_match(void) {
    // 前缀/截断不得误判
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("onled", 5)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("of", 2)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("o", 1)));
}

void test_longer_payload_is_not_a_match(void) {
    // 超长输入不得误判为有效命令（顺带覆盖越界读取风险）
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("onled2x", 7)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("offf", 4)));
}

void test_unterminated_buffer_is_safe(void) {
    // MQTT 给的是"长度 + 字节"，payload 不保证以 '\0' 结尾。
    // 这里刻意用一个非终止的缓冲区，验证实现没有把它当 C 字符串处理。
    char buf[6] = {'o', 'n', 'l', 'e', 'd', '2'};
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::LedOn),
                          static_cast<int>(parseCommand(buf, 6)));

    // 同样非终止，但内容不匹配
    char bad[3] = {'x', 'y', 'z'};
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand(bad, 3)));
}

void test_embedded_nul_is_not_a_match(void) {
    // 长度内出现 '\0' 时不能提前判定相等
    char buf[6] = {'o', 'n', 'l', 'e', 'd', '\0'};
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand(buf, 6)));
}

void test_whitespace_is_not_trimmed(void) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand(" onled2", 7)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Unknown),
                          static_cast<int>(parseCommand("onled2\n", 7)));
}

// ---------- commandName ----------

void test_command_names(void) {
    TEST_ASSERT_EQUAL_STRING("LED_ON", commandName(Command::LedOn));
    TEST_ASSERT_EQUAL_STRING("LED_OFF", commandName(Command::LedOff));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", commandName(Command::Unknown));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_parses_onled2);
    RUN_TEST(test_parses_off);
    RUN_TEST(test_is_case_sensitive);
    RUN_TEST(test_null_payload_is_unknown);
    RUN_TEST(test_empty_payload_is_unknown);
    RUN_TEST(test_prefix_is_not_a_match);
    RUN_TEST(test_longer_payload_is_not_a_match);
    RUN_TEST(test_unterminated_buffer_is_safe);
    RUN_TEST(test_embedded_nul_is_not_a_match);
    RUN_TEST(test_whitespace_is_not_trimmed);
    RUN_TEST(test_command_names);
    return UNITY_END();
}
