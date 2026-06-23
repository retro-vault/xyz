//
// ELF debug emitter unit tests
//
// MIT License (see: LICENSE)
//
#include <filesystem>
#include <fstream>
#include <algorithm>

#include <xld/binary_emitter.h>
#include <xld/elf_debug_emitter.h>
#include <xld/linker.h>
#include <xbfd/xbfd.h>

namespace {

    struct elf_debug_temp_dir {
        std::filesystem::path path;

        elf_debug_temp_dir() {
            path = std::filesystem::temp_directory_path()
                 / ("xlink_elf_debug_test_" + std::to_string(std::rand()));
            std::filesystem::create_directories(path);
        }

        ~elf_debug_temp_dir() {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    static void write_elf_debug_text(const std::filesystem::path& path,
                                     const std::string& text)
    {
        std::ofstream out(path);
        out << text;
    }

} // namespace

TEST(elf_debug_emitter_emits_elf_with_dwarf2_sections) {
    elf_debug_temp_dir temp;

    auto asm_rel = temp.path / "emit_startup.rel";
    auto asm_lst = temp.path / "emit_startup.lst";
    auto c_rel = temp.path / "emit_main.rel";
    auto c_cdb = temp.path / "emit_main.cdb";
    auto bin_path = temp.path / "prog.bin";
    auto elf_path = temp.path / "prog.elf";

    write_elf_debug_text(asm_rel,
        "XL4\n"
        "H 1 areas 3 global symbols\n"
        "M emit_startup\n"
        "S _main Ref00000000\n"
        "S .__.ABS. Def00000000\n"
        "A _CODE size 0004 flags 0 addr 0\n"
        "S _entry Def00000000\n"
        "T 00 00 00 00 CD 00 00 C9\n"
        "R 00 00 00 00 02 05 00 00\n");

    write_elf_debug_text(asm_lst,
        "                                      1         .module emit_startup\n"
        "                                      2         .globl _entry\n"
        "                                      3         .globl _main\n"
        "                                      4         .area _CODE\n"
        "    00000000                         5 _entry::\n"
        "    00000000 CD 00 00         [17]   6         call _main\n"
        "    00000003 C9               [10]   7         ret\n");

    write_elf_debug_text(c_rel,
        "XL4\n"
        "H 2 areas 6 global symbols\n"
        "M emit_main\n"
        "S .__.ABS. Def00000000\n"
        "A _CODE size 0003 flags 0 addr 0\n"
        "S _main Def00000000\n"
        "S C$emit_main.c$3$0_0$0 Def00000000\n"
        "S C$emit_main.c$5$0_0$0 Def00000002\n"
        "S G$main$0$0 Def00000000\n"
        "S XG$main$0$0 Def00000003\n"
        "T 00 00 00 00 3E 01 C9\n"
        "R 00 00 00 00\n"
        "A _DATA size 0001 flags 0 addr 0\n"
        "S G$flags$0$0 Def00000000\n");

    write_elf_debug_text(c_cdb,
        "M:emit_main\n"
        "F:G$main$0$0({2}DF,SI:S),C,0,0,0,0,0,ABI=sdcccall(0)\n"
        "S:G$main$0$0({2}DF,SI:S),C,0,0\n"
        "S:G$flags$0$0({1}SC:U),E,0,0\n");

    xld::cli_options opts;
    opts.mode = xld::link_mode::gnu;
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

    xld::elf_debug_emitter emitter;
    emitter.emit(elf_path, bin_path, ctx);

    auto obj = bfd::bfd::open_r(elf_path);
    ASSERT(obj->check_format(bfd::format::object));
    ASSERT_EQ(static_cast<int>(obj->get_flavour()),
              static_cast<int>(bfd::flavour::elf));

    auto* text = obj->find_section(".text");
    auto* abbrev = obj->find_section(".debug_abbrev");
    auto* info = obj->find_section(".debug_info");
    auto* line = obj->find_section(".debug_line");
    ASSERT(text != nullptr);
    ASSERT(abbrev != nullptr);
    ASSERT(info != nullptr);
    ASSERT(line != nullptr);
    ASSERT_EQ(text->vma, 0x0100);
    ASSERT_EQ(text->size, 8u);
    ASSERT(!abbrev->data.empty());
    ASSERT(!info->data.empty());
    ASSERT(!line->data.empty());
    ASSERT(std::find(abbrev->data.begin(), abbrev->data.end(), 0x36u)
           != abbrev->data.end());

    bool saw_entry = false;
    bool saw_main = false;
    for (const auto& sym : obj->symbols()) {
        if (sym.name == "_entry") {
            saw_entry = true;
            ASSERT(sym.is_global());
            ASSERT(sym.is_absolute());
            ASSERT_EQ(sym.value, 0x0100u);
        } else if (sym.name == "_main") {
            saw_main = true;
            ASSERT(sym.is_global());
            ASSERT(sym.is_absolute());
            ASSERT_EQ(sym.value, 0x0104u);
        }
    }
    ASSERT(saw_entry);
    ASSERT(saw_main);

    bool saw_debug_main = false;
    for (const auto& fn : obj->object().debug.functions) {
        if (fn.name != "_main")
            continue;
        saw_debug_main = true;
        ASSERT_EQ(fn.convention, xbfd::calling_convention::xcc_sdcccall0);
        ASSERT_EQ(fn.start, 0x0104u);
        ASSERT_EQ(fn.end, 0x0107u);
    }
    ASSERT(saw_debug_main);
}
