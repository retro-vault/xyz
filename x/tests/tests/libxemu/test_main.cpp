#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <xemu/xemu.h>

struct test_case {
    std::string name;
    void (*run)();
};

static std::vector<test_case>& registry() {
    static std::vector<test_case> tests;
    return tests;
}

struct registrar {
    registrar(const std::string& name, void (*run)()) {
        registry().push_back({name, run});
    }
};

#define TEST(name) \
    static void test_##name(); \
    static registrar reg_##name(#name, test_##name); \
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

TEST(machine_runs_until_halt_and_writes_stdout) {
    xemu::machine emu;
    std::ostringstream output;
    const std::vector<uint8_t> program = {
        0x3E, 0x4F, // ld a, 'O'
        0xD3, 0x01, // out (1), a
        0x3E, 0x4B, // ld a, 'K'
        0xD3, 0x01, // out (1), a
        0x76        // halt
    };

    emu.load_bytes(0x0000, program);
    emu.bind_stdout(0x0001, output);
    emu.set_pc(0x0000);

    const auto stop = emu.continue_execution();
    ASSERT_EQ(stop.reason, xemu::stop_reason::halted);
    ASSERT_EQ(output.str(), std::string("OK"));
    ASSERT(emu.halted());
}

TEST(machine_reads_stdin_port) {
    xemu::machine emu;
    std::istringstream input("Z");
    std::ostringstream output;
    const std::vector<uint8_t> program = {
        0xDB, 0x00, // in a, (0)
        0xD3, 0x01, // out (1), a
        0x76        // halt
    };

    emu.load_bytes(0x0000, program);
    emu.bind_stdin(0x0000, input);
    emu.bind_stdout(0x0001, output);
    emu.set_pc(0x0000);

    const auto stop = emu.continue_execution();
    ASSERT_EQ(stop.reason, xemu::stop_reason::halted);
    ASSERT_EQ(output.str(), std::string("Z"));
}

TEST(machine_reads_split_stdin_status_and_data_ports) {
    xemu::machine emu;
    std::istringstream input("Z");
    std::ostringstream output;
    const std::vector<uint8_t> program = {
        0xDB, 0x10, // in a, (0x10) status
        0xD3, 0x01, // out (1), a
        0xDB, 0x11, // in a, (0x11) data
        0xD3, 0x01, // out (1), a
        0xDB, 0x10, // in a, (0x10) status after consume
        0xD3, 0x01, // out (1), a
        0x76        // halt
    };

    emu.load_bytes(0x0000, program);
    emu.bind_stdin_status_data(0x0010, 0x0011, input);
    emu.bind_stdout(0x0001, output);
    emu.set_pc(0x0000);

    const auto stop = emu.continue_execution();
    const auto bytes = output.str();
    ASSERT_EQ(stop.reason, xemu::stop_reason::halted);
    ASSERT_EQ(bytes.size(), static_cast<std::size_t>(3));
    ASSERT_EQ(static_cast<unsigned char>(bytes[0]), 0x01);
    ASSERT_EQ(bytes[1], 'Z');
    ASSERT_EQ(static_cast<unsigned char>(bytes[2]), 0x00);
}

TEST(machine_breakpoint_stops_before_instruction) {
    xemu::machine emu;
    const std::vector<uint8_t> program = {
        0x00, // nop
        0x00, // nop
        0x76  // halt
    };

    emu.load_bytes(0x0000, program);
    emu.set_pc(0x0000);
    emu.insert_breakpoint(0x0001);

    auto stop = emu.continue_execution();
    ASSERT_EQ(stop.reason, xemu::stop_reason::breakpoint);
    ASSERT_EQ(stop.pc, static_cast<uint16_t>(0x0001));

    stop = emu.step_instruction();
    ASSERT_EQ(stop.reason, xemu::stop_reason::stepped);
    ASSERT_EQ(stop.pc, static_cast<uint16_t>(0x0002));

    stop = emu.continue_execution();
    ASSERT_EQ(stop.reason, xemu::stop_reason::halted);
}

TEST(machine_ignores_redundant_ix_prefix_on_nop) {
    xemu::machine emu;
    const std::vector<uint8_t> program = {
        0xDD, 0x00, // redundant IX prefix before NOP
        0x76        // halt
    };

    emu.load_bytes(0x0000, program);
    emu.set_pc(0x0000);

    const auto stop = emu.continue_execution();
    ASSERT_EQ(stop.reason, xemu::stop_reason::halted);
    ASSERT_EQ(stop.pc, static_cast<uint16_t>(0x0003));
}

int main() {
    int passed = 0;
    int failed = 0;

    for (const auto& test : registry()) {
        std::cout << "  " << test.name << "... ";
        try {
            test.run();
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
