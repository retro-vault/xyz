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

static std::filesystem::path fixture_path(const std::string& name) {
    std::filesystem::path p = "tests/fixtures/" + name;
    if (std::filesystem::exists(p)) return p;
    p = std::filesystem::path(__FILE__).parent_path() / "fixtures" / name;
    return p;
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

    in.close();
    std::filesystem::remove(out);
}
