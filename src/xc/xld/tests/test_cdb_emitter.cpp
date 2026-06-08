//
// cdb_emitter unit tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <filesystem>
#include <fstream>

#include <xld/binary_emitter.h>
#include <xld/cdb_emitter.h>
#include <xld/linker.h>

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
        "F:G$main$0$0({2}DF,SI:S),C,0,0,0,0,0,ABI=sdcccall(1)\n"
        "S:G$main$0$0({2}DF,SI:S),C,0,0\n"
        "S:G$flags$0$0({1}SC:U),E,0,0\n");

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

    xld::cdb_emitter emitter;
    emitter.emit(cdb_path, bin_path, ctx);

    const auto text = read_cdb_text(cdb_path);

    ASSERT(text.find("M:xlink_emit_main\n") != std::string::npos);
    ASSERT(text.find("F:G$main$0$0({2}DF,SI:S),C,0,0,0,0,0,ABI=sdcccall(1)\n") != std::string::npos);
    ASSERT(text.find("S:G$flags$0$0({1}SC:U),E,0,0\n") != std::string::npos);
    ASSERT(text.find("L:G$main$0$0:104\n") != std::string::npos);
    ASSERT(text.find("L:XG$main$0$0:107\n") != std::string::npos);
    ASSERT(text.find("L:G$flags$0$0:107\n") != std::string::npos);
    ASSERT(text.find("L:C$xlink_emit_main.c$3$0_0$0:104\n") != std::string::npos);
    ASSERT(text.find("L:C$xlink_emit_main.c$5$0_0$0:106\n") != std::string::npos);
    ASSERT(text.find("L:A$xlink_emit_startup$5:100\n") != std::string::npos);
}

TEST(cdb_emitter_supports_xcc_adb_and_source_label_fallback) {
    cdb_temp_dir temp;

    auto asm_rel = temp.path / "xlink_emit_startup.rel";
    auto asm_src = temp.path / "xlink_emit_startup.s";
    auto c_rel = temp.path / "xlink_emit_main.rel";
    auto c_adb = temp.path / "xlink_emit_main.adb";
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
        "S halt_loop Def00000003\n"
        "T 00 00 00 00 CD 00 00 C9\n"
        "R 00 00 00 00 02 05 00 00\n");

    write_cdb_text(asm_src,
        "        .module xlink_emit_startup\n"
        "        .globl _entry\n"
        "        .globl _main\n"
        "        .area _CODE\n"
        "_entry::\n"
        "        call _main\n"
        "halt_loop:\n"
        "        ret\n");

    write_cdb_text(c_rel,
        "XL4\n"
        "H 1 areas 4 global symbols\n"
        "M xlink_emit_main\n"
        "S .__.ABS. Def00000000\n"
        "A _CODE size 0006 flags 0 addr 0\n"
        "S _sieve Def00000000\n"
        "S _main Def00000003\n"
        "T 00 00 00 00 3E 01 C9 AF C9\n"
        "R 00 00 00 00\n");

    write_cdb_text(c_adb,
        "[sieve.c]\n"
        "F_sieve:12:0:0:0\n"
        "F_main:39:0:0:0\n");

    xld::cli_options opts;
    opts.input_files = {asm_rel, c_rel};
    opts.output_file = bin_path;
    opts.entry_symbol = "_entry";
    opts.area_bases["_CODE"] = 0x0100;
    opts.format = xld::output_format::bin;
    opts.output_range = xld::address_range{0x0100, 0x010A};

    xld::link_context ctx;
    ctx.entry_name = opts.entry_symbol;
    ctx.area_bases = opts.area_bases;
    ctx.output_range = opts.output_range;
    ctx.format = opts.format;

    xld::linker::link(ctx, opts);
    xld::binary_emitter::emit(bin_path, ctx);

    xld::cdb_emitter emitter;
    emitter.emit(cdb_path, bin_path, ctx);

    const auto text = read_cdb_text(cdb_path);

    ASSERT(text.find("M:xlink_emit_main\n") != std::string::npos);
    ASSERT(text.find("F:G$sieve$0$0({2}DF,SI:S),C,0,0,0,0,0\n") != std::string::npos);
    ASSERT(text.find("L:G$sieve$0$0:104\n") != std::string::npos);
    ASSERT(text.find("L:XG$sieve$0$0:107\n") != std::string::npos);
    ASSERT(text.find("L:C$sieve.c$12$0$0:104\n") != std::string::npos);
    ASSERT(text.find("L:C$sieve.c$39$0$0:107\n") != std::string::npos);
    ASSERT(text.find("L:A$xlink_emit_startup$5:100\n") != std::string::npos);
    ASSERT(text.find("L:A$xlink_emit_startup$7:103\n") != std::string::npos);
}

TEST(cdb_emitter_normalizes_native_sdcc_function_keys_and_preserves_c_lines) {
    cdb_temp_dir temp;

    auto asm_rel = temp.path / "xlink_emit_startup.rel";
    auto asm_src = temp.path / "xlink_emit_startup.s";
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

    write_cdb_text(asm_src,
        "        .module xlink_emit_startup\n"
        "        .globl _entry\n"
        "        .globl _main\n"
        "        .area _CODE\n"
        "_entry::\n"
        "        call _main\n"
        "        ret\n");

    write_cdb_text(c_rel,
        "XL4\n"
        "H 2 areas 8 global symbols\n"
        "M xlink_emit_main\n"
        "S .__.ABS. Def00000000\n"
        "A _CODE size 0003 flags 0 addr 0\n"
        "S _main Def00000000\n"
        "S G$main$0$0 Def00000000\n"
        "S XG$main$0$0 Def00000003\n"
        "S C$xlink_emit_main.c$3$1_0$7 Def00000000\n"
        "S C$xlink_emit_main.c$5$1_0$7 Def00000002\n"
        "T 00 00 00 00 3E 01 C9\n"
        "R 00 00 00 00\n"
        "A _DATA size 0001 flags 0 addr 0\n"
        "S G$flags$0_0$0 Def00000000\n");

    write_cdb_text(c_cdb,
        "M:xlink_emit_main\n"
        "F:G$main$0_0$0({2}DF,SI:S),C,0,0,0,0,0\n"
        "S:G$main$0_0$0({2}DF,SI:S),C,0,0\n"
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

    xld::cdb_emitter emitter;
    emitter.emit(cdb_path, bin_path, ctx);

    const auto text = read_cdb_text(cdb_path);

    ASSERT(text.find("F:G$main$0_0$0({2}DF,SI:S),C,0,0,0,0,0\n")
           != std::string::npos);
    ASSERT(text.find("L:G$main$0$0:104\n") != std::string::npos);
    ASSERT(text.find("L:XG$main$0$0:107\n") != std::string::npos);
    ASSERT(text.find("L:G$flags$0_0$0:107\n") != std::string::npos);
    ASSERT(text.find("L:C$xlink_emit_main.c$3$1_0$7:104\n")
           != std::string::npos);
    ASSERT(text.find("L:C$xlink_emit_main.c$5$1_0$7:106\n")
           != std::string::npos);
    ASSERT(text.find("L:C$xlink_emit_main.c$3$1$0:104\n")
           == std::string::npos);
}

TEST(cdb_emitter_tracks_symbol_shifts_after_reserved_hole_branch_promotion) {
    cdb_temp_dir temp;

    auto rel_path = temp.path / "branch_promote.rel";
    auto cdb_sidecar = temp.path / "branch_promote.cdb";
    auto bin_path = temp.path / "prog.bin";
    auto cdb_path = temp.path / "prog.cdb";

    write_cdb_text(cdb_sidecar,
        "M:branch_promote\n"
        "F:G$main$0$0({2}DF,SI:S),C,0,0,0,0,0\n"
        "S:G$main$0$0({2}DF,SI:S),C,0,0\n");

    auto mod = std::make_shared<xld::module>("branch_promote", rel_path);
    mod->areas().emplace_back("_A", 3, xld::area_flags::none, 0);
    mod->areas().emplace_back("_B", 1, xld::area_flags::none, 1);
    mod->symbols().emplace_back("_main", xld::symbol_type::def, 0, 0, 0);
    mod->symbols().emplace_back("_target", xld::symbol_type::def, 0, 1, 1);
    mod->symbols().emplace_back("C$branch.c$3$0_0$0",
                                xld::symbol_type::def, 0, 2, 0);
    mod->symbols().emplace_back("C$branch.c$4$0_0$0",
                                xld::symbol_type::def, 2, 3, 0);
    mod->symbols().emplace_back("G$main$0$0",
                                xld::symbol_type::def, 0, 4, 0);
    mod->symbols().emplace_back("XG$main$0$0",
                                xld::symbol_type::def, 3, 5, 0);

    xld::text_record source;
    source.area_index = 0;
    source.offset = 0;
    source.data = {0x18, 0x00, 0xC9}; // jr _target ; ret
    xld::reloc_entry re;
    re.mode = xld::reloc_mode::pc_rel | xld::reloc_mode::sym;
    re.offset_in_t = 1;
    re.ref_index = 1;
    source.relocs.push_back(re);
    mod->texts().push_back(source);

    xld::text_record target;
    target.area_index = 1;
    target.offset = 0;
    target.data = {0xC9};
    mod->texts().push_back(target);

    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);
    xld::binary_emitter::emit(bin_path, ctx);

    xld::cdb_emitter emitter;
    emitter.emit(cdb_path, bin_path, ctx);

    const auto text = read_cdb_text(cdb_path);
    ASSERT(text.find("L:G$main$0$0:0\n") != std::string::npos);
    ASSERT(text.find("L:XG$main$0$0:4\n") != std::string::npos);
    ASSERT(text.find("L:C$branch.c$3$0_0$0:0\n") != std::string::npos);
    ASSERT(text.find("L:C$branch.c$4$0_0$0:3\n") != std::string::npos);
}
