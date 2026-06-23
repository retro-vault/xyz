//
// test_main.cpp
//
// lightweight test runner
//
// MIT License (see: LICENSE)
//
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

struct test_case {
    std::string name;
    std::function<void()> func;
};

static std::vector<test_case>& test_registry() {
    static std::vector<test_case> tests;
    return tests;
}

struct test_registrar {
    test_registrar(const std::string& name, std::function<void()> func) {
        test_registry().push_back({name, func});
    }
};

#define TEST(name) \
    static void test_##name(); \
    static test_registrar reg_##name(#name, test_##name); \
    static void test_##name()

#define ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "  FAIL: " << #cond \
                  << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        throw std::runtime_error("assertion failed"); \
    } \
} while(0)

template<typename T>
std::string printable(const T& v) {
    std::ostringstream oss;
    if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>)
        oss << static_cast<int>(v);
    else
        oss << v;
    return oss.str();
}

#define ASSERT_EQ(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (_a != _b) { \
        std::cerr << "  FAIL: " << #a << " == " << #b \
                  << " (got " << printable(_a) \
                  << " vs " << printable(_b) << ")" \
                  << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        throw std::runtime_error("assertion failed"); \
    } \
} while(0)

#define ASSERT_THROWS(expr, exc_type) do { \
    bool caught = false; \
    try { expr; } catch (const exc_type&) { caught = true; } \
    if (!caught) { \
        std::cerr << "  FAIL: expected " << #exc_type \
                  << " from " << #expr \
                  << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        throw std::runtime_error("assertion failed"); \
    } \
} while(0)

#include "test_cli.cpp"
#include "test_operations.cpp"

int main() {
    int passed = 0;
    int failed = 0;
    for (auto& tc : test_registry()) {
        std::cout << "  " << tc.name << "... ";
        try {
            tc.func();
            std::cout << "OK\n";
            passed++;
        } catch (const std::exception&) {
            std::cout << "FAILED\n";
            failed++;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
