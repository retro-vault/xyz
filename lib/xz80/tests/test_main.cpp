// test_main.cpp — xz80 test runner (include-based, mirrors xld/xbfd pattern).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace test_fw {

struct test_case {
    std::string              name;
    std::function<void()>    run;
    bool                     passed = true;
    std::string              failure;
};

static std::vector<test_case>& suite()
{
    static std::vector<test_case> s;
    return s;
}

static test_case* g_current = nullptr;

static void register_test(const char* name, std::function<void()> fn)
{
    suite().push_back({ name, std::move(fn) });
}

static void require(bool cond, const char* expr, const char* file, int line)
{
    if (!cond && g_current) {
        g_current->passed = false;
        char buf[256];
        std::snprintf(buf, sizeof buf, "%s:%d: REQUIRE(%s) failed", file, line, expr);
        g_current->failure = buf;
    }
}

} // namespace test_fw

#define TEST(name) \
    static void _test_##name(); \
    static int  _reg_##name = (test_fw::register_test(#name, _test_##name), 0); \
    static void _test_##name()

#define REQUIRE(expr) \
    test_fw::require(!!(expr), #expr, __FILE__, __LINE__)

#define REQUIRE_EQ(a, b) \
    do { auto _a = (a); auto _b = (b); \
         if (!(_a == _b) && test_fw::g_current) { \
             test_fw::g_current->passed = false; \
             test_fw::g_current->failure = std::string(__FILE__) + ":" \
                 + std::to_string(__LINE__) + ": REQUIRE_EQ(" #a ", " #b ") failed"; \
         } } while(0)

// Include all test files (each registers its tests via static initializers).
#include "test_disassembler.cpp"
#include "test_cpu.cpp"

int main()
{
    int pass = 0, fail = 0;
    for (auto& tc : test_fw::suite()) {
        test_fw::g_current = &tc;
        tc.run();
        test_fw::g_current = nullptr;
        if (tc.passed) {
            std::printf("  %s... OK\n", tc.name.c_str());
            ++pass;
        } else {
            std::printf("  %s... FAIL\n    %s\n",
                tc.name.c_str(), tc.failure.c_str());
            ++fail;
        }
    }
    std::printf("\n%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
