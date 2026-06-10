#include <cstdio>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <xgdb/xgdb.h>

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

#define ASSERT_EQ(a, b) do { \
    const auto _a = (a); \
    const auto _b = (b); \
    if (!(_a == _b)) { \
        std::cerr << "  FAIL: " << #a << " == " << #b \
                  << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        throw std::runtime_error("assertion failed"); \
    } \
} while(0)

TEST(z80_disassembler_sdcc_output) {
    const std::vector<uint8_t> bytes = {
        0x3E, 0x42,
        0xDD, 0x7E, 0x05,
        0xDD, 0x77, 0xFE
    };

    xgdb::vector_memory_reader memory(bytes);
    auto disassembler = xgdb::make_z80_disassembler();
    auto formatter = xgdb::make_sdcc_z80_formatter();

    auto inst0 = disassembler->disassemble_one(0x0000, memory);
    ASSERT_EQ(formatter->format(inst0), "ld\ta, #0x42");

    auto inst1 = disassembler->disassemble_one(0x0002, memory);
    ASSERT_EQ(formatter->format(inst1), "ld\ta, 5(ix)");

    auto inst2 = disassembler->disassemble_one(0x0005, memory);
    ASSERT_EQ(formatter->format(inst2), "ld\t-2(ix), a");
}

TEST(z80_disassembler_gnu_output) {
    const std::vector<uint8_t> bytes = {
        0x3E, 0x42,
        0xDD, 0x7E, 0x05,
        0xDD, 0x77, 0xFE
    };

    xgdb::vector_memory_reader memory(bytes);
    auto disassembler = xgdb::make_z80_disassembler();
    auto formatter = xgdb::make_gnu_z80_formatter();

    auto inst0 = disassembler->disassemble_one(0x0000, memory);
    ASSERT_EQ(formatter->format(inst0), "ld\ta, 0x42");

    auto inst1 = disassembler->disassemble_one(0x0002, memory);
    ASSERT_EQ(formatter->format(inst1), "ld\ta, (ix+5)");

    auto inst2 = disassembler->disassemble_one(0x0005, memory);
    ASSERT_EQ(formatter->format(inst2), "ld\t(ix-2), a");
}

int main() {
    int passed = 0;
    int failed = 0;

    for (const auto& test : test_registry()) {
        std::cout << "  " << test.name << "... ";
        try {
            test.func();
            std::cout << "OK\n";
            ++passed;
        } catch (const std::exception&) {
            std::cout << "FAILED\n";
            ++failed;
        }
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
