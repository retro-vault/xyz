#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <xdbg/xdbg.hpp>

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

static xdbg::document sample_document() {
    xdbg::document doc;
    doc.version = 1;
    doc.image_path = "yos.rom";
    doc.entry_address = 0x0100;

    doc.files.push_back({1, "src/yos/kernel/syscall.c", xdbg::language_kind::c});
    doc.files.push_back({2, "src/yos/startup/crt0rom.s", xdbg::language_kind::assembly});

    xdbg::symbol main_symbol;
    main_symbol.name = "_main";
    main_symbol.kind = xdbg::symbol_kind::function;
    main_symbol.address = 0x0100;
    main_symbol.size = 0x20;
    main_symbol.file_id = 1;
    main_symbol.line = 12;
    main_symbol.column = 1;
    main_symbol.type_name = "int(void)";
    main_symbol.language = xdbg::language_kind::c;
    doc.symbols.push_back(main_symbol);

    xdbg::symbol asm_label;
    asm_label.name = "gsinit";
    asm_label.kind = xdbg::symbol_kind::label;
    asm_label.address = 0x0200;
    asm_label.parent_name = "crt0";
    asm_label.file_id = 2;
    asm_label.line = 44;
    asm_label.language = xdbg::language_kind::assembly;
    doc.symbols.push_back(asm_label);

    xdbg::function dispatch;
    dispatch.name = "_syscall_dispatch";
    dispatch.start_address = 0x1432;
    dispatch.end_address = 0x1490;
    dispatch.file_id = 1;
    dispatch.line = 42;
    dispatch.column = 1;
    dispatch.return_type = "uint16_t";
    dispatch.language = xdbg::language_kind::c;
    doc.functions.push_back(dispatch);

    doc.lines.push_back({0x1432, 1, 42, 1});
    doc.lines.push_back({0x1435, 1, 43, 1});

    xdbg::variable argument;
    argument.name = "call_number";
    argument.kind = xdbg::symbol_kind::parameter;
    argument.parent_name = "_syscall_dispatch";
    argument.storage = xdbg::storage_kind::register_pair;
    argument.register_name = "hl";
    argument.type_name = "uint16_t";
    argument.start_address = 0x1432;
    argument.end_address = 0x1490;
    argument.file_id = 1;
    argument.line = 42;
    argument.column = 24;
    argument.language = xdbg::language_kind::c;
    doc.variables.push_back(argument);

    xdbg::variable local;
    local.name = "tmp";
    local.kind = xdbg::symbol_kind::local;
    local.parent_name = "_syscall_dispatch";
    local.storage = xdbg::storage_kind::stack;
    local.offset = -2;
    local.type_name = "uint8_t";
    local.start_address = 0x1440;
    local.end_address = 0x1488;
    local.file_id = 1;
    local.line = 45;
    local.column = 9;
    local.language = xdbg::language_kind::c;
    doc.variables.push_back(local);

    return doc;
}

TEST(write_and_read_round_trip) {
    const xdbg::document original = sample_document();

    std::stringstream buffer;
    xdbg::write(buffer, original);
    const xdbg::document parsed = xdbg::read(buffer);

    ASSERT_EQ(parsed.version, 1u);
    ASSERT(parsed.image_path.has_value());
    ASSERT_EQ(parsed.image_path.value(), "yos.rom");
    ASSERT(parsed.entry_address.has_value());
    ASSERT_EQ(parsed.entry_address.value(), 0x0100u);

    ASSERT_EQ(parsed.files.size(), 2u);
    ASSERT_EQ(parsed.files[1].path, "src/yos/startup/crt0rom.s");
    ASSERT_EQ(parsed.symbols.size(), 2u);
    ASSERT_EQ(parsed.symbols[0].name, "_main");
    ASSERT(parsed.symbols[0].size.has_value());
    ASSERT_EQ(parsed.symbols[0].size.value(), 0x20u);
    ASSERT(parsed.symbols[1].parent_name.has_value());
    ASSERT_EQ(parsed.symbols[1].parent_name.value(), "crt0");

    ASSERT_EQ(parsed.functions.size(), 1u);
    ASSERT_EQ(parsed.functions[0].start_address, 0x1432u);
    ASSERT(parsed.functions[0].return_type.has_value());
    ASSERT_EQ(parsed.functions[0].return_type.value(), "uint16_t");

    ASSERT_EQ(parsed.lines.size(), 2u);
    ASSERT_EQ(parsed.lines[1].line, 43u);

    ASSERT_EQ(parsed.variables.size(), 2u);
    ASSERT(parsed.variables[0].register_name.has_value());
    ASSERT_EQ(parsed.variables[0].register_name.value(), "hl");
    ASSERT(parsed.variables[1].offset.has_value());
    ASSERT_EQ(parsed.variables[1].offset.value(), -2);
}

TEST(read_file_and_write_file) {
    const xdbg::document original = sample_document();
    const auto path = std::filesystem::temp_directory_path() / "xdbg_test_file.xdbg";

    xdbg::write_file(path, original);
    const xdbg::document parsed = xdbg::read_file(path);

    ASSERT_EQ(parsed.symbols.size(), original.symbols.size());
    ASSERT_EQ(parsed.variables.size(), original.variables.size());

    std::filesystem::remove(path);
}

TEST(parse_comments_and_manual_records) {
    std::stringstream input;
    input
        << "# comment\n"
        << "xdbg 1\n"
        << "image path=\"prog.bin\"\n"
        << "entry address=0x200\n"
        << "file id=7 path=\"asm/file.s\" language=\"assembly\"\n"
        << "symbol name=\"label_1\" kind=\"label\" address=0x200 language=\"assembly\" file=7 line=10\n"
        << "function name=\"boot\" start=0x200 end=0x210 language=\"assembly\" file=7 line=1\n"
        << "line address=0x200 file=7 line=1 column=1\n"
        << "variable name=\"saved_a\" kind=\"local\" storage=\"stack\" offset=-1 language=\"assembly\" parent=\"boot\"\n";

    const xdbg::document parsed = xdbg::read(input);

    ASSERT(parsed.image_path.has_value());
    ASSERT_EQ(parsed.image_path.value(), "prog.bin");
    ASSERT_EQ(parsed.files[0].language, xdbg::language_kind::assembly);
    ASSERT_EQ(parsed.symbols[0].kind, xdbg::symbol_kind::label);
    ASSERT_EQ(parsed.functions[0].name, "boot");
    ASSERT(parsed.variables[0].offset.has_value());
    ASSERT_EQ(parsed.variables[0].offset.value(), -1);
}

TEST(z80_disassembler_sdcc_output) {
    const std::vector<uint8_t> bytes = {
        0x3E, 0x42,
        0xDD, 0x7E, 0x05,
        0xDD, 0x77, 0xFE
    };

    xdbg::vector_memory_reader memory(bytes);
    auto disassembler = xdbg::make_z80_disassembler();
    auto formatter = xdbg::make_sdcc_z80_formatter();

    auto inst0 = disassembler->disassemble_one(0x0000, memory);
    ASSERT_EQ(formatter->format(inst0), "ld\ta, #0x42");

    auto inst1 = disassembler->disassemble_one(0x0002, memory);
    ASSERT_EQ(formatter->format(inst1), "ld\ta, 5(ix)");

    auto inst2 = disassembler->disassemble_one(0x0005, memory);
    ASSERT_EQ(formatter->format(inst2), "ld\t-2(ix), a");
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
