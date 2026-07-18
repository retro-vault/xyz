#include <cstdint>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>
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

TEST(machine_binds_platform_emu_stdio_defaults) {
    xemu::machine emu;
    std::istringstream input("Q");
    std::ostringstream output;
    const std::vector<uint8_t> program = {
        0xDB, 0xE2, // in a, (0xe2) status
        0xB7,       // or a
        0x28, 0xFB, // jr z, back to start
        0xDB, 0xE3, // in a, (0xe3) data
        0xD3, 0xE1, // out (0xe1), a
        0x76        // halt
    };

    emu.load_bytes(0x0000, program);
    emu.bind_emu_stdio(input, output);
    emu.set_pc(0x0000);

    const auto stop = emu.continue_execution();
    ASSERT_EQ(stop.reason, xemu::stop_reason::halted);
    ASSERT_EQ(output.str(), std::string("Q"));
}

TEST(machine_loads_sparse_ihx_image) {
    const auto path = std::filesystem::temp_directory_path() / "xemu-load-ihx-test.ihx";
    {
        std::ofstream out(path);
        ASSERT(out.is_open());
        out << ":030100003E427606\n";
        out << ":01FF0000768A\n";
        out << ":00000001FF\n";
    }

    xemu::machine emu;
    emu.load_ihx(path);

    ASSERT_EQ(emu.read_byte(0x0100), static_cast<uint8_t>(0x3E));
    ASSERT_EQ(emu.read_byte(0x0101), static_cast<uint8_t>(0x42));
    ASSERT_EQ(emu.read_byte(0x0102), static_cast<uint8_t>(0x76));
    ASSERT_EQ(emu.read_byte(0x00FF), static_cast<uint8_t>(0x00));
    ASSERT_EQ(emu.read_byte(0xFF00), static_cast<uint8_t>(0x76));

    std::filesystem::remove(path);
}

TEST(machine_loads_elf_sections_and_returns_entry) {
    const auto path = std::filesystem::temp_directory_path() / "xemu-load-elf-test.elf";

    xbfd::object obj;
    obj.module_name = "xemu_load_elf";
    obj.format = xbfd::obj_format::executable;
    obj.flavour = xbfd::obj_flavour::elf;
    obj.entry = 0x0100;

    xbfd::section text;
    text.name = ".text";
    text.flags = xbfd::section_flags::alloc
               | xbfd::section_flags::load
               | xbfd::section_flags::code;
    text.vma = 0x0100;
    text.size = 1;
    text.data = {0x76};
    obj.sections.push_back(text);

    xbfd::section debug;
    debug.name = ".debug_info";
    debug.flags = xbfd::section_flags::debugging;
    debug.size = 2;
    debug.data = {1, 2};
    obj.sections.push_back(debug);

    xbfd::elf_writer writer;
    writer.write(path.string(), obj);

    xemu::machine emu;
    const auto entry = emu.load_elf(path);

    ASSERT_EQ(entry, static_cast<uint16_t>(0x0100));
    ASSERT_EQ(emu.read_byte(0x0100), static_cast<uint8_t>(0x76));
    ASSERT_EQ(emu.read_byte(0x0000), static_cast<uint8_t>(0x00));

    std::filesystem::remove(path);
}

TEST(machine_loads_readonly_store_image) {
    xemu::machine emu;
    xemu::memory_map_config map;
    map.stores.push_back({"rom", 1, 0x4000u, false});
    map.stores.push_back({"ram", 1, 0xC000u, true});
    map.windows.push_back({0x0000, 0x3FFF, "rom", 0, std::nullopt, 0});
    map.windows.push_back({0x4000, 0xFFFF, "ram", 0, std::nullopt, 0});

    emu.configure_memory_map(map);
    const std::array<uint8_t, 3> image = {0x3E, 0x42, 0x76};
    emu.load_bytes(0x0000, image);

    ASSERT_EQ(emu.read_byte(0x0000), static_cast<uint8_t>(0x3E));
    ASSERT_EQ(emu.read_byte(0x0001), static_cast<uint8_t>(0x42));
    ASSERT_EQ(emu.read_byte(0x0002), static_cast<uint8_t>(0x76));

    emu.write_byte(0x0001, 0x99);
    ASSERT_EQ(emu.read_byte(0x0001), static_cast<uint8_t>(0x42));
}

TEST(machine_switches_selector_driven_window_via_port) {
    xemu::machine emu;
    std::ostringstream output;
    const std::vector<uint8_t> program = {
        0x3E, 0x41,       // ld a, 'A'
        0x32, 0x00, 0x80, // ld (0x8000), a   bank 0
        0x3E, 0x01,       // ld a, 1
        0xD3, 0x10,       // out (0x10), a    switch to bank 1
        0x3E, 0x42,       // ld a, 'B'
        0x32, 0x00, 0x80, // ld (0x8000), a   bank 1
        0xAF,             // xor a
        0xD3, 0x10,       // out (0x10), a    switch to bank 0
        0x3A, 0x00, 0x80, // ld a, (0x8000)
        0xD3, 0x01,       // out (1), a
        0x3E, 0x01,       // ld a, 1
        0xD3, 0x10,       // out (0x10), a    switch to bank 1
        0x3A, 0x00, 0x80, // ld a, (0x8000)
        0xD3, 0x01,       // out (1), a
        0x76              // halt
    };

    xemu::memory_map_config map;
    map.stores.push_back({"low", 1, 0x8000u, true});
    map.stores.push_back({"banked", 2, 0x4000u, true});
    map.stores.push_back({"high", 1, 0x4000u, true});
    map.selectors.push_back({"bank", 0});
    map.windows.push_back({0x0000, 0x7FFF, "low", 0, std::nullopt, 0});
    map.windows.push_back({0x8000, 0xBFFF, "banked", std::nullopt, std::string("bank"), 0});
    map.windows.push_back({0xC000, 0xFFFF, "high", 0, std::nullopt, 0});
    map.port_rules.push_back({0x0010, 0xFFFF, "bank", 0x00FF, 0});

    emu.configure_memory_map(map);
    emu.load_bytes(0x0000, program);
    emu.bind_stdout(0x0001, output);
    emu.set_pc(0x0000);

    const auto stop = emu.continue_execution();
    ASSERT_EQ(stop.reason, xemu::stop_reason::halted);
    ASSERT_EQ(output.str(), std::string("AB"));
    ASSERT_EQ(emu.read_byte(0x0000), static_cast<uint8_t>(0x3E));
}

TEST(machine_updates_multiple_selectors_from_one_port) {
    xemu::machine emu;
    std::ostringstream output;
    const std::vector<uint8_t> program = {
        0x01, 0xFD, 0x7F, // ld bc, 0x7ffd
        0xAF,             // xor a
        0xED, 0x79,       // out (c), a      low=0 top=0
        0x3E, 0x4C,       // ld a, 'L'
        0x32, 0x00, 0x00, // ld (0x0000), a  low bank 0
        0x3E, 0x30,       // ld a, '0'
        0x32, 0x00, 0xC0, // ld (0xc000), a  top bank 0
        0x3E, 0x11,       // ld a, 0x11
        0xED, 0x79,       // out (c), a      low=1 top=1
        0x3E, 0x6C,       // ld a, 'l'
        0x32, 0x00, 0x00, // ld (0x0000), a  low bank 1
        0x3E, 0x31,       // ld a, '1'
        0x32, 0x00, 0xC0, // ld (0xc000), a  top bank 1
        0xAF,             // xor a
        0xED, 0x79,       // out (c), a      low=0 top=0
        0x3A, 0x00, 0x00, // ld a, (0x0000)
        0xD3, 0x01,       // out (1), a
        0x3A, 0x00, 0xC0, // ld a, (0xc000)
        0xD3, 0x01,       // out (1), a
        0x3E, 0x11,       // ld a, 0x11
        0xED, 0x79,       // out (c), a      low=1 top=1
        0x3A, 0x00, 0x00, // ld a, (0x0000)
        0xD3, 0x01,       // out (1), a
        0x3A, 0x00, 0xC0, // ld a, (0xc000)
        0xD3, 0x01,       // out (1), a
        0x76              // halt
    };

    xemu::memory_map_config map;
    map.stores.push_back({"low", 2, 0x4000u, true});
    map.stores.push_back({"mid", 1, 0x8000u, true});
    map.stores.push_back({"top", 8, 0x4000u, true});
    map.selectors.push_back({"low_bank", 0});
    map.selectors.push_back({"top_bank", 0});
    map.windows.push_back({0x0000, 0x3FFF, "low", std::nullopt, std::string("low_bank"), 0});
    map.windows.push_back({0x4000, 0xBFFF, "mid", 0, std::nullopt, 0});
    map.windows.push_back({0xC000, 0xFFFF, "top", std::nullopt, std::string("top_bank"), 0});
    map.port_rules.push_back({0x00FD, 0x00FF, "low_bank", 0x0010, 4});
    map.port_rules.push_back({0x00FD, 0x00FF, "top_bank", 0x0007, 0});

    emu.configure_memory_map(map);
    emu.load_bytes(0x4000, program);
    emu.bind_stdout(0x0001, output);
    emu.set_pc(0x4000);

    const auto stop = emu.continue_execution();
    ASSERT_EQ(stop.reason, xemu::stop_reason::halted);
    ASSERT_EQ(output.str(), std::string("L0l1"));
    ASSERT_EQ(emu.read_byte(0x0000), static_cast<uint8_t>('l'));
    ASSERT_EQ(emu.read_byte(0xC000), static_cast<uint8_t>('1'));
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
