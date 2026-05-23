//
// xdbg_emitter unit tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <filesystem>
#include <fstream>

#include <xdbg/io.hpp>

#include <xlink/binary_emitter.hpp>
#include <xlink/linker.hpp>
#include <xlink/xdbg_emitter.hpp>

namespace {

    struct scoped_temp_dir {
        std::filesystem::path path;

        scoped_temp_dir() {
            path = std::filesystem::temp_directory_path()
                 / ("xlink_xdbg_test_" + std::to_string(std::rand()));
            std::filesystem::create_directories(path);
        }

        ~scoped_temp_dir() {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    static void write_text(const std::filesystem::path& path,
                           const std::string& text)
    {
        std::ofstream out(path);
        out << text;
    }

} // namespace

TEST(xdbg_emitter_emits_c_and_assembly_debug_info) {
    scoped_temp_dir temp;

    auto asm_source = temp.path / "xlink_emit_startup.s";
    auto asm_rel = temp.path / "xlink_emit_startup.rel";
    auto asm_lst = temp.path / "xlink_emit_startup.lst";
    auto c_source = temp.path / "xlink_emit_main.c";
    auto c_rel = temp.path / "xlink_emit_main.rel";
    auto c_adb = temp.path / "xlink_emit_main.adb";
    auto bin_path = temp.path / "prog.bin";
    auto xdbg_path = temp.path / "prog.xdbg";

    write_text(asm_source,
        "        .module xlink_emit_startup\n"
        "        .globl _entry\n"
        "        .globl _main\n"
        "        .area _CODE\n"
        "_entry::\n"
        "        call _main\n"
        "        ret\n");

    write_text(asm_rel,
        "XL4\n"
        "H 1 areas 3 global symbols\n"
        "M xlink_emit_startup\n"
        "S _main Ref00000000\n"
        "S .__.ABS. Def00000000\n"
        "A _CODE size 0004 flags 0 addr 0\n"
        "S _entry Def00000000\n"
        "T 00 00 00 00 CD 00 00 C9\n"
        "R 00 00 00 00 02 05 00 00\n");

    write_text(asm_lst,
        "                                      1         .module xlink_emit_startup\n"
        "                                      2         .globl _entry\n"
        "                                      3         .globl _main\n"
        "                                      4         .area _CODE\n"
        "    00000000                         5 _entry::\n"
        "    00000000 CD 00 00         [17]   6         call _main\n"
        "    00000003 C9               [10]   7         ret\n");

    write_text(c_source,
        "unsigned char flags;\n"
        "\n"
        "int main(void) {\n"
        "    unsigned char counter;\n"
        "    counter = 1;\n"
        "    return counter;\n"
        "}\n");

    write_text(c_rel,
        ";!FILE xlink_emit_main.asm\n"
        "XL4\n"
        "H 2 areas 6 global symbols\n"
        "M xlink_emit_main\n"
        "O -mz80 sdcccall(1)\n"
        "S .__.ABS. Def00000000\n"
        "A _CODE size 0003 flags 0 addr 0\n"
        "S _main Def00000000\n"
        "S C$xlink_emit_main.c$3$0_0$0 Def00000000\n"
        "S C$xlink_emit_main.c$5$0_0$0 Def00000002\n"
        "S G$main$0$0 Def00000000\n"
        "S XG$main$0$0 Def00000003\n"
        "T 00 00 00 00 3E 01 C9\n"
        "R 00 00 00 00\n"
        "A _DATA size 0001 flags 0 addr 0\n"
        "S _flags Def00000000\n");

    write_text(c_adb,
        "M:xlink_emit_main\n"
        "F:G$main$0_0$0({2}DF,SI:S),C,0,0,0,0,0\n"
        "S:Lxlink_emit_main.main$counter$1_0$4({1}SC:U),R,0,0,[c,b]\n"
        "S:G$flags$0_0$0({1}SC:U),E,0,0\n");

    xlink::cli_options opts;
    opts.input_files = {asm_rel, c_rel};
    opts.output_file = bin_path;
    opts.entry_symbol = "_entry";
    opts.area_bases["_CODE"] = 0x0100;
    opts.format = xlink::output_format::bin;
    opts.output_range = xlink::address_range{0x0100, 0x0107};

    xlink::link_context ctx;
    ctx.entry_name = opts.entry_symbol;
    ctx.area_bases = opts.area_bases;
    ctx.output_range = opts.output_range;
    ctx.format = opts.format;

    xlink::linker::link(ctx, opts);
    xlink::binary_emitter::emit(bin_path, ctx);
    xlink::xdbg_emitter emitter;
    emitter.emit(xdbg_path, bin_path, ctx);

    auto doc = xdbg::read_file(xdbg_path);

    ASSERT(doc.image_path.has_value());
    ASSERT_EQ(doc.image_path.value(), std::filesystem::absolute(bin_path).lexically_normal().string());
    ASSERT(doc.entry_address.has_value());
    ASSERT_EQ(doc.entry_address.value(), 0x0100u);
    ASSERT_EQ(doc.files.size(), 2u);
    ASSERT_EQ(doc.functions.size(), 2u);

    bool saw_entry = false;
    bool saw_main = false;
    bool saw_flags = false;
    bool saw_counter = false;
    bool saw_asm_line = false;
    bool saw_c_line = false;

    for (const auto& function : doc.functions) {
        if (function.name == "_entry") {
            saw_entry = true;
            ASSERT_EQ(function.start_address, 0x0100u);
            ASSERT(function.language == xdbg::language_kind::assembly);
        } else if (function.name == "_main") {
            saw_main = true;
            ASSERT_EQ(function.start_address, 0x0104u);
            ASSERT(function.return_type.has_value());
            ASSERT_EQ(function.return_type.value(), "int");
            ASSERT(function.language == xdbg::language_kind::c);
        }
    }

    for (const auto& symbol : doc.symbols) {
        if (symbol.name == "_entry") {
            ASSERT_EQ(symbol.address, 0x0100u);
        }
        if (symbol.name == "_flags") {
            saw_flags = true;
            ASSERT_EQ(symbol.address, 0x0107u);
            ASSERT(symbol.size.has_value());
            ASSERT_EQ(symbol.size.value(), 1u);
        }
    }

    for (const auto& variable : doc.variables) {
        if (variable.name == "counter") {
            saw_counter = true;
            ASSERT(variable.parent_name.has_value());
            ASSERT_EQ(variable.parent_name.value(), "_main");
            ASSERT(variable.storage == xdbg::storage_kind::register_pair);
            ASSERT(variable.register_name.has_value());
            ASSERT_EQ(variable.register_name.value(), "bc");
        }
    }

    for (const auto& line : doc.lines) {
        if (line.address == 0x0100u && line.line == 5u)
            saw_asm_line = true;
        if (line.address == 0x0104u && line.line == 3u)
            saw_c_line = true;
    }

    ASSERT(saw_entry);
    ASSERT(saw_main);
    ASSERT(saw_flags);
    ASSERT(saw_counter);
    ASSERT(saw_asm_line);
    ASSERT(saw_c_line);
}
