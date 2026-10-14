// PlatformIO 的 Unity 测试运行器要求每个测试可执行文件都提供 setUp/tearDown。
// 这两个钩子在本项目中暂时不需要做任何事，留空即可。
//
// 注意：本文件位于 test/ 根目录，会被所有测试套件共享。

#include <unity.h>

void setUp(void) {
    // 每个测试用例执行前调用；本工程各测试自管理状态，故留空。
}

void tearDown(void) {
    // 每个测试用例执行后调用；本工程各测试自管理状态，故留空。
}
