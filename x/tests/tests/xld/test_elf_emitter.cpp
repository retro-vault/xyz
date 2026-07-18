//
// primary ELF emitter unit tests
//
// MIT License (see: LICENSE)
//
#include <algorithm>
#include <filesystem>

#include <xld/elf_emitter.h>
#include <xld/linker.h>
#include <xbfd/xbfd.h>

TEST(elf_emitter_writes_linked_executable_with_sections_entry_and_symbols) {
    auto dir = make_linker_temp_dir("/tmp/xld-elf-output-XXXXXX");
    auto obj_path = dir / "main.o";
    auto elf_path = dir / "prog.elf";

    write_multi_section_elf_object(obj_path, "main", "_start",
                                   {0x00, 0xC9}, {0x12, 0x34}, {0x56});

    xld::cli_options opts;
    opts.mode = xld::link_mode::gnu;
    opts.input_files = {obj_path};
    opts.entry_symbol = "_start";
    opts.format = xld::output_format::elf;
    opts.area_bases[".text"] = 0x0100;
    opts.area_bases[".data"] = 0x2000;

    xld::link_context ctx;
    ctx.entry_name = opts.entry_symbol;
    ctx.area_bases = opts.area_bases;
    ctx.format = opts.format;

    xld::linker::link(ctx, opts);
    xld::elf_emitter::emit(elf_path, ctx, false);

    auto obj = bfd::bfd::open_r(elf_path);
    ASSERT(obj->check_format(bfd::format::executable));
    ASSERT_EQ(static_cast<int>(obj->get_flavour()),
              static_cast<int>(bfd::flavour::elf));
    ASSERT_EQ(obj->object().entry, 0x0100u);

    auto* text = obj->find_section(".text");
    auto* rodata = obj->find_section(".rodata");
    auto* data = obj->find_section(".data");
    ASSERT(text != nullptr);
    ASSERT(rodata != nullptr);
    ASSERT(data != nullptr);
    ASSERT_EQ(text->vma, 0x0100u);
    ASSERT_EQ(text->size, 2u);
    ASSERT_EQ(text->data[0], 0x00);
    ASSERT_EQ(text->data[1], 0xC9);
    ASSERT_EQ(rodata->vma, 0x0102u);
    ASSERT_EQ(rodata->size, 2u);
    ASSERT_EQ(data->vma, 0x2000u);
    ASSERT_EQ(data->size, 1u);

    bool saw_start = false;
    for (const auto& sym : obj->symbols()) {
        if (sym.name != "_start")
            continue;
        saw_start = true;
        ASSERT(sym.is_global());
        ASSERT(!sym.is_absolute());
        ASSERT_EQ(sym.value, 0x0100u);
        ASSERT_EQ(sym.size, 0u);
    }
    ASSERT(saw_start);

    std::filesystem::remove_all(dir);
}

TEST(elf_emitter_exports_selected_strong_symbol_over_weak_fallback) {
    auto dir = make_linker_temp_dir("/tmp/xld-elf-weak-output-XXXXXX");
    auto elf_path = dir / "prog.elf";

    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::elf;

    auto weak_mod = std::make_shared<xld::module>("weak", "weak.o");
    weak_mod->areas().emplace_back(".text", 1, xld::area_flags::none, 0);
    weak_mod->texts().push_back({0, 0, {0xC9}, {}});
    weak_mod->symbols().emplace_back("_hook", xld::symbol_type::def, 0, 0, 0,
                                     false, xld::symbol_kind::function, 1,
                                     true, true);
    ctx.modules.push_back(weak_mod);

    auto strong_mod = std::make_shared<xld::module>("strong", "strong.o");
    strong_mod->areas().emplace_back(".text", 1, xld::area_flags::none, 0);
    strong_mod->texts().push_back({0, 0, {0xC9}, {}});
    strong_mod->symbols().emplace_back("_main", xld::symbol_type::def, 0, 0, 0,
                                       false, xld::symbol_kind::function, 1,
                                       true);
    strong_mod->symbols().emplace_back("_hook", xld::symbol_type::def, 0, 1, 0,
                                       false, xld::symbol_kind::function, 1,
                                       true);
    ctx.modules.push_back(strong_mod);

    xld::cli_options opts;
    opts.format = xld::output_format::elf;
    xld::linker::link(ctx, opts);
    xld::elf_emitter::emit(elf_path, ctx, false);

    auto obj = bfd::bfd::open_r(elf_path);
    bool saw_hook = false;
    for (const auto& sym : obj->symbols()) {
        if (sym.name != "_hook")
            continue;
        saw_hook = true;
        ASSERT(sym.is_global());
        ASSERT(!sym.is_weak());
        ASSERT_EQ(sym.value, 0x0001u);
    }
    ASSERT(saw_hook);

    std::filesystem::remove_all(dir);
}
