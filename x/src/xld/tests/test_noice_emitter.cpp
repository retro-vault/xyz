//
// noice_emitter unit tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <filesystem>
#include <fstream>

#include <xld/binary_emitter.h>
#include <xld/linker.h>
#include <xld/noice_emitter.h>

namespace {

    struct noice_temp_dir {
        std::filesystem::path path;

        noice_temp_dir() {
            path = std::filesystem::temp_directory_path()
                 / ("xlink_noice_test_" + std::to_string(std::rand()));
            std::filesystem::create_directories(path);
        }

        ~noice_temp_dir() {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    static void write_noice_text(const std::filesystem::path& path,
                                 const std::string& text)
    {
        std::ofstream out(path);
        out << text;
    }

    static std::string read_noice_text(const std::filesystem::path& path) {
        std::ifstream in(path);
        return std::string((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());
    }

} // namespace

TEST(noice_emitter_emits_symbol_and_source_commands) {
    noice_temp_dir temp;

    auto asm_source = temp.path / "xlink_emit_startup.s";
    auto asm_rel = temp.path / "xlink_emit_startup.rel";
    auto asm_lst = temp.path / "xlink_emit_startup.lst";
    auto c_source = temp.path / "xlink_emit_main.c";
    auto c_rel = temp.path / "xlink_emit_main.rel";
    auto c_adb = temp.path / "xlink_emit_main.adb";
    auto bin_path = temp.path / "prog.bin";
    auto noi_path = temp.path / "prog.noi";

    write_noice_text(asm_source,
        "        .module xlink_emit_startup\n"
        "        .globl _entry\n"
        "        .globl _main\n"
        "        .area _CODE\n"
        "_entry::\n"
        "        call _main\n"
        "        ret\n");

    write_noice_text(asm_rel,
        "XL4\n"
        "H 1 areas 3 global symbols\n"
        "M xlink_emit_startup\n"
        "S _main Ref00000000\n"
        "S .__.ABS. Def00000000\n"
        "A _CODE size 0004 flags 0 addr 0\n"
        "S _entry Def00000000\n"
        "T 00 00 00 00 CD 00 00 C9\n"
        "R 00 00 00 00 02 05 00 00\n");

    write_noice_text(asm_lst,
        "                                      1         .module xlink_emit_startup\n"
        "                                      2         .globl _entry\n"
        "                                      3         .globl _main\n"
        "                                      4         .area _CODE\n"
        "    00000000                         5 _entry::\n"
        "    00000000 CD 00 00         [17]   6         call _main\n"
        "    00000003 C9               [10]   7         ret\n");

    write_noice_text(c_source,
        "unsigned char flags;\n"
        "\n"
        "int main(void) {\n"
        "    unsigned char counter;\n"
        "    counter = 1;\n"
        "    return counter;\n"
        "}\n");

    write_noice_text(c_rel,
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

    write_noice_text(c_adb,
        "M:xlink_emit_main\n"
        "F:G$main$0_0$0({2}DF,SI:S),C,0,0,0,0,0\n"
        "S:Lxlink_emit_main.main$counter$1_0$4({1}SC:U),R,0,0,[c,b]\n"
        "S:G$flags$0_0$0({1}SC:U),E,0,0\n");

    xld::cli_options opts;
    opts.input_files = {asm_rel, c_rel};
    opts.output_file = bin_path;
    opts.entry_symbol = "_entry";
    opts.area_bases["_CODE"] = 0x0100;
    opts.format = xld::output_format::bin;
    opts.output_range = xld::address_range{0x0100, 0x0107};

    xld::link_context ctx;
    ctx.entry_name = opts.entry_symbol;
    ctx.area_bases = opts.area_bases;
    ctx.output_range = opts.output_range;
    ctx.format = opts.format;

    xld::linker::link(ctx, opts);
    xld::binary_emitter::emit(bin_path, ctx);

    xld::noice_emitter emitter;
    emitter.emit(noi_path, bin_path, ctx);

    const auto text = read_noice_text(noi_path);

    ASSERT(text.find("LASTFILELOADED\n") != std::string::npos);
    ASSERT(text.find("CLEARLINEINFO Y\n") != std::string::npos);
    ASSERT(text.find("DEF _entry 0x0100\n") != std::string::npos);
    ASSERT(text.find("DEF _flags 0x0107\n") != std::string::npos);
    ASSERT(text.find("FUNCTION _main 0x0104\n") != std::string::npos);
    ASSERT(text.find("DEFSCOPE counter &BC %U8\n") != std::string::npos);
    ASSERT(text.find("LINE 3 0x0000\n") != std::string::npos);
    ASSERT(text.find("LINE 5 0x0002\n") != std::string::npos);
    ASSERT(text.find("ENDFUNCTION 0x0106\n") != std::string::npos);
}
