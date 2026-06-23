// test_linker.cpp
//
// linker integration tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <xld/linker.h>
#include <xld/lib_parser.h>
#include <xld/binary_emitter.h>
#include <xld/errors.h>
#include <xbfd/xbfd.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <unistd.h>

static std::filesystem::path fixture_path(const std::string& name) {
    std::filesystem::path p = "tests/fixtures/" + name;
    if (std::filesystem::exists(p)) return p;
    p = std::filesystem::path(__FILE__).parent_path() / "fixtures" / name;
    return p;
}

static std::filesystem::path make_linker_temp_dir(const char* pattern) {
    char dir_template[64];
    std::snprintf(dir_template, sizeof(dir_template), "%s", pattern);
    char* dir = mkdtemp(dir_template);
    ASSERT(dir != nullptr);
    return std::filesystem::path(dir);
}

static std::string read_file_bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    ASSERT(input.is_open());
    return std::string((std::istreambuf_iterator<char>(input)), {});
}

static std::string linker_fixed_width_field(const std::string& value,
                                            std::size_t width) {
    ASSERT(value.size() <= width);
    return value + std::string(width - value.size(), ' ');
}

static void write_linker_ar_member(std::ofstream& out,
                                   const std::string& raw_name,
                                   const std::string& data)
{
    std::string header;
    header += linker_fixed_width_field(raw_name, 16);
    header += linker_fixed_width_field("0", 12);
    header += linker_fixed_width_field("0", 6);
    header += linker_fixed_width_field("0", 6);
    header += linker_fixed_width_field("100644", 8);
    header += linker_fixed_width_field(std::to_string(data.size()), 10);
    header += "`\n";
    ASSERT_EQ(header.size(), 60);
    out.write(header.data(), header.size());
    out.write(data.data(), data.size());
    if ((data.size() & 1U) != 0)
        out.put('\n');
}

static void write_simple_elf_object(const std::filesystem::path& path,
                                    const std::string& module_name,
                                    const std::string& defined_symbol,
                                    const std::vector<uint8_t>& text_bytes,
                                    const std::vector<std::string>& refs = {})
{
    auto obj = bfd::bfd::open_w(path, bfd::flavour::elf);
    obj->set_module_name(module_name);
    auto& text = obj->add_section(
        ".text",
        bfd::section_flags::alloc
            | bfd::section_flags::load
            | bfd::section_flags::code,
        0);
    text.data = text_bytes;
    text.size = text_bytes.size();
    obj->add_symbol(defined_symbol,
                    bfd::symbol_flags::global | bfd::symbol_flags::function,
                    0,
                    ".text");
    for (const auto& ref : refs) {
        obj->add_symbol(ref,
                        bfd::symbol_flags::global
                            | bfd::symbol_flags::undefined,
                        0,
                        "");
    }
    obj->close();
}

static void write_multi_section_elf_object(const std::filesystem::path& path,
                                           const std::string& module_name,
                                           const std::string& defined_symbol,
                                           const std::vector<uint8_t>& text_bytes,
                                           const std::vector<uint8_t>& rodata_bytes,
                                           const std::vector<uint8_t>& data_bytes = {})
{
    auto obj = bfd::bfd::open_w(path, bfd::flavour::elf);
    obj->set_module_name(module_name);

    auto& text = obj->add_section(
        ".text",
        bfd::section_flags::alloc
            | bfd::section_flags::load
            | bfd::section_flags::code,
        0);
    text.data = text_bytes;
    text.size = text_bytes.size();

    auto& rodata = obj->add_section(
        ".rodata",
        bfd::section_flags::alloc | bfd::section_flags::load,
        0);
    rodata.data = rodata_bytes;
    rodata.size = rodata_bytes.size();

    if (!data_bytes.empty()) {
        auto& data = obj->add_section(
            ".data",
            bfd::section_flags::alloc
                | bfd::section_flags::load
                | bfd::section_flags::data,
            0);
        data.data = data_bytes;
        data.size = data_bytes.size();
    }

    obj->add_symbol(defined_symbol,
                    bfd::symbol_flags::global | bfd::symbol_flags::function,
                    0,
                    ".text");
    obj->close();
}

TEST(linker_duplicate_symbol_error) {
    xld::link_context ctx;
    ctx.entry_name = "_main";

    // Create two modules both defining _main.
    auto mod1 = std::make_shared<xld::module>("mod1", "mod1.rel");
    mod1->areas().emplace_back("_CODE", 1, xld::area_flags::none, 0);
    mod1->symbols().emplace_back("_main", xld::symbol_type::def, 0, 0);
    ctx.modules.push_back(mod1);

    auto mod2 = std::make_shared<xld::module>("mod2", "mod2.rel");
    mod2->areas().emplace_back("_CODE", 1, xld::area_flags::none, 0);
    mod2->symbols().emplace_back("_main", xld::symbol_type::def, 0, 0);
    ctx.modules.push_back(mod2);

    xld::cli_options opts;
    ASSERT_THROWS(xld::linker::link(ctx, opts), xld::symbol_error);
}

TEST(linker_unresolved_symbol_error) {
    xld::link_context ctx;
    ctx.entry_name = "_main";

    auto mod = std::make_shared<xld::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 1, xld::area_flags::none, 0);
    mod->symbols().emplace_back("_main", xld::symbol_type::def, 0, 0);
    mod->symbols().emplace_back("_missing", xld::symbol_type::ref, 0, 1);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    ASSERT_THROWS(xld::linker::link(ctx, opts), xld::symbol_error);
}

TEST(linker_library_selective_inclusion) {
    auto lib_members = xld::lib_parser::parse(fixture_path("library.lib"));
    // Library should list 2 modules.
    ASSERT_EQ(static_cast<int>(lib_members.size()), 2);
    ASSERT_EQ(lib_members[0].path.filename(),
              std::filesystem::path("lib_mod1.rel"));
    ASSERT(!lib_members[0].contents.has_value());

    // Scan defs from lib_mod1.
    auto defs = xld::rel_parser::scan_defs(fixture_path("lib_mod1.rel"));
    ASSERT_EQ(static_cast<int>(defs.size()), 1);
    ASSERT_EQ(defs[0], "_helper");
}

TEST(linker_simple_link) {
    // Test: link simple.rel + lib_mod1.rel (provides _helper).
    xld::link_context ctx;
    ctx.entry_name = "_main";

    auto mod1 = xld::rel_parser::parse(fixture_path("simple.rel"));
    auto mod2 = xld::rel_parser::parse(fixture_path("lib_mod1.rel"));
    ctx.modules.push_back(mod1);
    ctx.modules.push_back(mod2);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT(ctx.code_size > 0);
    ASSERT(!ctx.code_buffer.empty());
    ASSERT(ctx.linker_symbols.find("s__CODE") != ctx.linker_symbols.end());
    ASSERT(ctx.linker_symbols.find("l__CODE") != ctx.linker_symbols.end());
}

TEST(linker_uses_sdcc_linker_script_for_area_bases_and_holes) {
    auto script = fixture_path("script_sdcc_place.lk");
    std::vector<std::string> args = {
        "xld",
        "-T", script.string(),
        fixture_path("simple.rel").string(),
        fixture_path("lib_mod1.rel").string()
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    xld::link_context ctx;
    ctx.entry_name = opts.entry_symbol;
    ctx.holes = opts.reserved_ranges;
    ctx.area_bases = opts.area_bases;
    ctx.output_range = opts.output_range;
    ctx.format = opts.format;

    xld::linker::link(ctx, opts);

    ASSERT_EQ(ctx.linker_symbols["s__CODE"], 0x0110);
    ASSERT_EQ(ctx.modules[0]->areas()[0].placed_addr().value(), 0x0110);
}

TEST(linker_binary_output) {
    xld::link_context ctx;
    ctx.code_size = 4;
    ctx.code_buffer = {0xC9, 0x00, 0x00, 0xC9};
    ctx.entry_point = 0;

    xld::output_reloc r;
    r.offset = 1;
    r.size = 2;
    r.pad = 0;
    ctx.reloc_table.push_back(r);

    std::filesystem::path out = "/tmp/xlink_test_output.bin";
    xld::binary_emitter::emit(out, ctx);

    // Read back and verify header.
    std::ifstream in(out, std::ios::binary);
    ASSERT(in.is_open());

    uint8_t buf[12];
    in.read(reinterpret_cast<char*>(buf), 12);

    // Magic: 'X', 'L'
    ASSERT_EQ(buf[0], 0x58);
    ASSERT_EQ(buf[1], 0x4C);
    // Version.
    ASSERT_EQ(buf[2], 0x01);
    // Entry point: 0x0000.
    ASSERT_EQ(buf[4], 0x00);
    ASSERT_EQ(buf[5], 0x00);
    // Code size: 0x0004.
    ASSERT_EQ(buf[6], 0x04);
    ASSERT_EQ(buf[7], 0x00);
    // Reloc count: 1.
    ASSERT_EQ(buf[8], 0x01);
    ASSERT_EQ(buf[9], 0x00);

    // Read reloc entry (4 bytes).
    uint8_t reloc_buf[4];
    in.read(reinterpret_cast<char*>(reloc_buf), 4);
    // Offset: 1.
    ASSERT_EQ(reloc_buf[0], 0x01);
    ASSERT_EQ(reloc_buf[1], 0x00);
    // Size: 2.
    ASSERT_EQ(reloc_buf[2], 0x02);
    ASSERT_EQ(reloc_buf[3], 0x00);

    in.close();
    std::filesystem::remove(out);
}

TEST(linker_binary_output_preserves_byte_reloc_flags) {
    xld::link_context ctx;
    ctx.code_size = 1;
    ctx.code_buffer = {0x00};
    ctx.entry_point = 0;

    xld::output_reloc r;
    r.offset = 0;
    r.size = 1;
    r.pad = 0x01;
    ctx.reloc_table.push_back(r);

    std::filesystem::path out = "/tmp/xlink_test_output_msb.xl";
    xld::binary_emitter::emit(out, ctx);

    std::ifstream in(out, std::ios::binary);
    ASSERT(in.is_open());
    in.seekg(12);

    uint8_t reloc_buf[4];
    in.read(reinterpret_cast<char*>(reloc_buf), 4);
    ASSERT_EQ(reloc_buf[0], 0x00);
    ASSERT_EQ(reloc_buf[1], 0x00);
    ASSERT_EQ(reloc_buf[2], 0x01);
    ASSERT_EQ(reloc_buf[3], 0x01);

    in.close();
    std::filesystem::remove(out);
}

TEST(linker_relaxes_local_jp_to_jr) {
    xld::link_context ctx;
    ctx.entry_name = "_main";

    auto mod = std::make_shared<xld::module>("relax", "relax.rel");
    mod->areas().emplace_back("_CODE", 4, xld::area_flags::none, 0);
    mod->symbols().emplace_back("_main", xld::symbol_type::def, 0, 0, 0);
    mod->symbols().emplace_back("L1", xld::symbol_type::def, 3, 1, 0);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0xC3, 0x00, 0x00, 0xC9}; // jp L1 ; ret

    xld::reloc_entry re;
    re.mode = xld::reloc_mode::word | xld::reloc_mode::sym;
    re.offset_in_t = 1;
    re.ref_index = 1;
    tr.relocs.push_back(re);
    mod->texts().push_back(tr);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(ctx.code_size, 3);
    ASSERT_EQ(static_cast<int>(ctx.code_buffer.size()), 3);
    ASSERT_EQ(ctx.code_buffer[0], 0x18);
    ASSERT_EQ(ctx.code_buffer[1], 0x00);
    ASSERT_EQ(ctx.code_buffer[2], 0xC9);
    ASSERT_EQ(static_cast<int>(ctx.reloc_table.size()), 0);
}

TEST(linker_promotes_forward_short_branch_over_reserved_hole) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = std::make_shared<xld::module>("fwd", "fwd.rel");
    mod->areas().emplace_back("_A", 3, xld::area_flags::none, 0);
    mod->areas().emplace_back("_B", 1, xld::area_flags::none, 1);
    mod->symbols().emplace_back("_main", xld::symbol_type::def, 0, 0, 0);
    mod->symbols().emplace_back("_target", xld::symbol_type::def, 0, 1, 1);

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

    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(mod->area_by_index(0).size(), 4);
    ASSERT_EQ(ctx.code_buffer[0x0000], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x80);
    ASSERT_EQ(ctx.code_buffer[0x0002], 0x01);
    ASSERT_EQ(ctx.code_buffer[0x0003], 0xC9);
    ASSERT_EQ(ctx.code_buffer[0x0180], 0xC9);
}

TEST(linker_promotes_backward_short_branch_over_reserved_hole) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = std::make_shared<xld::module>("back", "back.rel");
    mod->areas().emplace_back("_A", 1, xld::area_flags::none, 0);
    mod->areas().emplace_back("_B", 3, xld::area_flags::none, 1);
    mod->symbols().emplace_back("_target", xld::symbol_type::def, 0, 0, 0);
    mod->symbols().emplace_back("_main", xld::symbol_type::def, 0, 1, 1);

    xld::text_record target;
    target.area_index = 0;
    target.offset = 0;
    target.data = {0xC9};
    mod->texts().push_back(target);

    xld::text_record source;
    source.area_index = 1;
    source.offset = 0;
    source.data = {0x18, 0x00, 0xC9}; // jr _target ; ret
    xld::reloc_entry re;
    re.mode = xld::reloc_mode::pc_rel | xld::reloc_mode::sym;
    re.offset_in_t = 1;
    re.ref_index = 0;
    source.relocs.push_back(re);
    mod->texts().push_back(source);

    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(mod->area_by_index(1).size(), 4);
    ASSERT_EQ(ctx.code_buffer[0x0180], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0181], 0x00);
    ASSERT_EQ(ctx.code_buffer[0x0182], 0x00);
    ASSERT_EQ(ctx.code_buffer[0x0183], 0xC9);
    ASSERT_EQ(ctx.code_buffer[0x0000], 0xC9);
}

TEST(linker_links_gnu_elf_object) {
    auto dir = make_linker_temp_dir("/tmp/xld-gnu-obj-XXXXXX");
    auto obj_path = dir / "main.o";
    write_simple_elf_object(obj_path, "main", "_start", {0xC9});

    xld::link_context ctx;
    xld::cli_options opts;
    opts.mode = xld::link_mode::gnu;
    opts.input_files = {obj_path};
    ctx.entry_name = "_start";

    xld::linker::link(ctx, opts);

    ASSERT_EQ(static_cast<int>(ctx.modules.size()), 1);
    ASSERT(ctx.global_symbols.find("_start") != ctx.global_symbols.end());
    ASSERT_EQ(ctx.code_size, 1);
    ASSERT_EQ(static_cast<int>(ctx.code_buffer.size()), 1);
    ASSERT_EQ(ctx.code_buffer[0], 0xC9);

    std::filesystem::remove_all(dir);
}

TEST(linker_uses_gnu_linker_script_for_section_bases) {
    auto dir = make_linker_temp_dir("/tmp/xld-gnu-script-XXXXXX");
    auto obj_path = dir / "main.o";
    write_simple_elf_object(obj_path, "main", "_gnu_entry", {0xC9});

    auto script = fixture_path("script_gnu.ld");
    std::vector<std::string> args = {
        "xld",
        "--mode=gnu",
        "-T", script.string(),
        obj_path.string()
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    xld::link_context ctx;
    ctx.entry_name = opts.entry_symbol;
    ctx.holes = opts.reserved_ranges;
    ctx.area_bases = opts.area_bases;
    ctx.output_range = opts.output_range;
    ctx.format = opts.format;

    xld::linker::link(ctx, opts);

    ASSERT_EQ(static_cast<int>(ctx.modules.size()), 1);
    ASSERT_EQ(ctx.modules[0]->areas()[0].placed_addr().value(), 0x0200);
    ASSERT_EQ(ctx.entry_point, 0x0200);

    std::filesystem::remove_all(dir);
}

TEST(linker_uses_gnu_rom_script_section_order) {
    auto dir = make_linker_temp_dir("/tmp/xld-gnu-rom-XXXXXX");
    auto obj_path = dir / "rommain.o";
    write_multi_section_elf_object(obj_path, "rommain", "_start",
                                   {0x00, 0xC9}, {0x11, 0x22}, {0x33});

    auto script = fixture_path("script_gnu_rom.ld");
    std::vector<std::string> args = {
        "xld",
        "--mode=gnu",
        "-T", script.string(),
        obj_path.string()
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    xld::link_context ctx;
    ctx.entry_name = opts.entry_symbol;
    ctx.holes = opts.reserved_ranges;
    ctx.area_bases = opts.area_bases;
    ctx.area_order = opts.area_order;
    ctx.output_range = opts.output_range;
    ctx.format = opts.format;

    xld::linker::link(ctx, opts);

    ASSERT_EQ(static_cast<int>(ctx.modules.size()), 1);
    ASSERT_EQ(ctx.entry_point, 0x0000);

    std::optional<uint16_t> text_addr;
    std::optional<uint16_t> rodata_addr;
    std::optional<uint16_t> data_addr;

    for (const auto& area : ctx.modules[0]->areas()) {
        if (area.name() == ".text")
            text_addr = area.placed_addr();
        else if (area.name() == ".rodata")
            rodata_addr = area.placed_addr();
        else if (area.name() == ".data")
            data_addr = area.placed_addr();
    }

    ASSERT(text_addr.has_value());
    ASSERT(rodata_addr.has_value());
    ASSERT(data_addr.has_value());
    ASSERT_EQ(*text_addr, 0x0000);
    ASSERT_EQ(*rodata_addr, 0x0002);
    ASSERT_EQ(*data_addr, 0x8000);

    std::filesystem::remove_all(dir);
}

TEST(linker_resolves_gnu_archive_members) {
    auto dir = make_linker_temp_dir("/tmp/xld-gnu-archive-XXXXXX");
    auto main_obj = dir / "main.o";
    auto helper_obj = dir / "helper.o";
    auto archive_path = dir / "libhelpers.a";

    write_simple_elf_object(main_obj, "main", "_start", {0xC9}, {"_helper"});
    write_simple_elf_object(helper_obj, "helper", "_helper", {0xC9});

    {
        std::ofstream out(archive_path, std::ios::binary);
        out << "!<arch>\n";
        write_linker_ar_member(out, "helper.o/", read_file_bytes(helper_obj));
    }

    auto members = xld::lib_parser::parse(archive_path);
    ASSERT_EQ(static_cast<int>(members.size()), 1);
    ASSERT(members[0].contents.has_value());
    {
        std::istringstream defs_input(members[0].contents.value());
        auto defs = xld::rel_parser::scan_defs(members[0].path.string(),
                                               defs_input);
        ASSERT_EQ(static_cast<int>(defs.size()), 1);
        ASSERT_EQ(defs[0], std::string("_helper"));
    }

    xld::link_context ctx;
    xld::cli_options opts;
    opts.mode = xld::link_mode::gnu;
    opts.input_files = {main_obj, archive_path};
    ctx.entry_name = "_start";

    xld::linker::link(ctx, opts);

    ASSERT_EQ(static_cast<int>(ctx.modules.size()), 2);
    ASSERT(ctx.global_symbols.find("_helper") != ctx.global_symbols.end());
    ASSERT(ctx.global_symbols.find("_start") != ctx.global_symbols.end());
    ASSERT(ctx.code_size >= 2);

    std::filesystem::remove_all(dir);
}
