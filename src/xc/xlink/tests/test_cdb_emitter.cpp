//
// cdb_emitter unit tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <filesystem>
#include <fstream>

#include <xlink/binary_emitter.hpp>
#include <xlink/cdb_emitter.hpp>
#include <xlink/linker.hpp>

namespace {

    struct cdb_temp_dir {
        std::filesystem::path path;

        cdb_temp_dir() {
            path = std::filesystem::temp_directory_path()
                 / ("xlink_cdb_test_" + std::to_string(std::rand()));
            std::filesystem::create_directories(path);
        }

        ~cdb_temp_dir() {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    static void write_cdb_text(const std::filesystem::path& path,
                               const std::string& text)
    {
        std::ofstream out(path);
        out << text;
    }

    static std::string read_cdb_text(const std::filesystem::path& path) {
        std::ifstream in(path);
        return std::string((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());
    }

} // namespace

TEST(cdb_emitter_merges_compiler_records_with_link_records) {
    cdb_temp_dir temp;

    auto asm_rel = temp.path / "xlink_emit_startup.rel";
    auto asm_lst = temp.path / "xlink_emit_startup.lst";
    auto c_rel = temp.path / "xlink_emit_main.rel";
    auto c_cdb = temp.path / "xlink_emit_main.cdb";
    auto bin_path = temp.path / "prog.bin";
    auto cdb_path = temp.path / "prog.cdb";

    write_cdb_text(asm_rel,
        "XL4\n"
        "H 1 areas 3 global symbols\n"
        "M xlink_emit_startup\n"
        "S _main Ref00000000\n"
        "S .__.ABS. Def00000000\n"
        "A _CODE size 0004 flags 0 addr 0\n"
        "S _entry Def00000000\n"
        "T 00 00 00 00 CD 00 00 C9\n"
        "R 00 00 00 00 02 05 00 00\n");

    write_cdb_text(asm_lst,
        "                                      1         .module xlink_emit_startup\n"
        "                                      2         .globl _entry\n"
        "                                      3         .globl _main\n"
        "                                      4         .area _CODE\n"
        "    00000000                         5 _entry::\n"
        "    00000000 CD 00 00         [17]   6         call _main\n"
        "    00000003 C9               [10]   7         ret\n");

    write_cdb_text(c_rel,
        "XL4\n"
        "H 2 areas 6 global symbols\n"
        "M xlink_emit_main\n"
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
        "S G$flags$0$0 Def00000000\n");

    write_cdb_text(c_cdb,
        "M:xlink_emit_main\n"
        "F:G$main$0$0({2}DF,SI:S),C,0,0,0,0,0\n"
        "S:G$main$0$0({2}DF,SI:S),C,0,0\n"
        "S:G$flags$0$0({1}SC:U),E,0,0\n");

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

    xlink::cdb_emitter emitter;
    emitter.emit(cdb_path, bin_path, ctx);

    const auto text = read_cdb_text(cdb_path);

    ASSERT(text.find("M:xlink_emit_main\n") != std::string::npos);
    ASSERT(text.find("F:G$main$0$0({2}DF,SI:S),C,0,0,0,0,0\n") != std::string::npos);
    ASSERT(text.find("S:G$flags$0$0({1}SC:U),E,0,0\n") != std::string::npos);
    ASSERT(text.find("L:G$main$0$0:104\n") != std::string::npos);
    ASSERT(text.find("L:XG$main$0$0:107\n") != std::string::npos);
    ASSERT(text.find("L:G$flags$0$0:107\n") != std::string::npos);
    ASSERT(text.find("L:C$xlink_emit_main.c$3$0$0:104\n") != std::string::npos);
    ASSERT(text.find("L:C$xlink_emit_main.c$5$0$0:106\n") != std::string::npos);
    ASSERT(text.find("L:A$xlink_emit_startup$5:100\n") != std::string::npos);
}
