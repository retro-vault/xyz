// test_main.cpp — xcc runtime test runner.
//
// Loads the pre-assembled runtime binary, initialises the global Z80
// emulation machine, then runs all registered test cases.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>

// ---------------------------------------------------------------------------
// Minimal test framework (mirrors xz80/xlink pattern)
// ---------------------------------------------------------------------------

namespace test_fw {

struct test_case {
    std::string           name;
    std::function<void()> run;
    bool                  passed = true;
    std::string           failure;
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
        g_current->passed  = false;
        char buf[256];
        std::snprintf(buf, sizeof buf, "%s:%d: REQUIRE(%s) failed",
                      file, line, expr);
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
             char _buf[256]; \
             std::snprintf(_buf, sizeof _buf, \
                 "%s:%d: REQUIRE_EQ(" #a ", " #b ") failed", \
                 __FILE__, __LINE__); \
             test_fw::g_current->failure = _buf; \
         } } while(0)

// ---------------------------------------------------------------------------
// Global runtime machine forward declaration (runtime_machine.hpp provides
// the struct; test files reference g_rt which is initialised in main).
// ---------------------------------------------------------------------------
#include "runtime_machine.hpp"

// Include all test modules (static-initialiser registration pattern).
#include "test_int16.cpp"
#include "test_int16_edge.cpp"
#include "test_shifts.cpp"
#include "test_int32.cpp"
#include "test_int32_edge.cpp"
#include "test_float_arith.cpp"
#include "test_float_conv.cpp"
#include "test_float_edge.cpp"
// long long and double are now implemented: activate their tests.
#define PENDING_TEST TEST
#include "test_ll.cpp"
#include "test_double.cpp"

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

static std::vector<uint8_t> load_file(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "error: cannot open %s\n", path);
        return {};
    }
    return { std::istreambuf_iterator<char>(f), {} };
}

int main(int argc, char* argv[])
{
    const char* bin_path = "build/runtime.bin";
    if (argc > 1) bin_path = argv[1];

    auto code = load_file(bin_path);
    if (code.empty()) {
        std::fprintf(stderr, "fatal: runtime binary not found: %s\n", bin_path);
        return 1;
    }

    runtime_machine rt(std::span<const uint8_t>(code.data(), code.size()));
    g_rt = &rt;

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
