// test_rel_parser.cpp
//
// rel_parser unit tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <fstream>
#include <xlink/rel_parser.hpp>
#include <xlink/errors.hpp>

static std::filesystem::path fixture(const std::string& name) {
    // Try relative to cwd first, then relative to executable.
    std::filesystem::path p = "tests/fixtures/" + name;
    if (std::filesystem::exists(p)) return p;
    // Try from src/xc/xlink directory.
    p = std::filesystem::path(__FILE__).parent_path() / "fixtures" / name;
    return p;
}

TEST(rel_parser_simple) {
    auto mod = xlink::rel_parser::parse(fixture("simple.rel"));
    ASSERT(mod != nullptr);
    ASSERT_EQ(mod->name(), "simple");
    ASSERT(mod->endian() == xlink::byte_order::little_endian);
    ASSERT_EQ(static_cast<int>(mod->areas().size()), 1);
    ASSERT_EQ(mod->areas()[0].name(), "_CODE");
    ASSERT_EQ(mod->areas()[0].size(), 6);
    ASSERT_EQ(static_cast<int>(mod->symbols().size()), 2);
    ASSERT_EQ(mod->symbols()[0].name(), "_main");
    ASSERT(mod->symbols()[0].is_def());
    ASSERT_EQ(mod->symbols()[1].name(), "_helper");
    ASSERT(mod->symbols()[1].is_ref());
}

TEST(rel_parser_multi_area) {
    auto mod = xlink::rel_parser::parse(fixture("multi_area.rel"));
    ASSERT(mod != nullptr);
    ASSERT_EQ(mod->name(), "multi_area");
    ASSERT_EQ(static_cast<int>(mod->areas().size()), 3);
    ASSERT_EQ(mod->areas()[0].name(), "_CODE");
    ASSERT_EQ(mod->areas()[0].size(), 4);
    ASSERT_EQ(mod->areas()[1].name(), "_DATA");
    ASSERT_EQ(mod->areas()[1].size(), 2);
    ASSERT_EQ(mod->areas()[2].name(), "_BSS");
    ASSERT_EQ(mod->areas()[2].size(), 16);
}

TEST(rel_parser_text_records) {
    auto mod = xlink::rel_parser::parse(fixture("simple.rel"));
    ASSERT(!mod->texts().empty());
    auto& tr = mod->texts()[0];
    ASSERT_EQ(tr.offset, 0);
    ASSERT_EQ(static_cast<int>(tr.data.size()), 4);
    // Data: CD 00 00 C9
    ASSERT_EQ(tr.data[0], 0xCD);
    ASSERT_EQ(tr.data[3], 0xC9);
}

TEST(rel_parser_scan_defs) {
    auto defs = xlink::rel_parser::scan_defs(fixture("lib_mod1.rel"));
    ASSERT_EQ(static_cast<int>(defs.size()), 1);
    ASSERT_EQ(defs[0], "_helper");
}

TEST(rel_parser_nonexistent_file) {
    ASSERT_THROWS(
        xlink::rel_parser::parse("nonexistent.rel"),
        xlink::parse_error);
}

TEST(rel_parser_xl2_reloc_entry_layout) {
    auto dir = std::filesystem::temp_directory_path();
    auto path = dir / "xlink_test_xl2.rel";

    {
        std::ofstream out(path);
        out << "XL2\n"
            << "H 1 areas 3 global symbols\n"
            << "M xl2\n"
            << "S .__.ABS. Def0000\n"
            << "S _ext Ref0000\n"
            << "A _CODE size 0003 flags 0 addr 0\n"
            << "S _main Def0000\n"
            << "T 00 00 C3 00 00\n"
            << "R 00 00 00 00 02 01 01 00\n";
    }

    auto mod = xlink::rel_parser::parse(path);
    ASSERT_EQ(static_cast<int>(mod->texts().size()), 1);
    ASSERT_EQ(static_cast<int>(mod->texts()[0].relocs.size()), 1);
    ASSERT_EQ(mod->texts()[0].relocs[0].offset_in_t, 1);
    ASSERT_EQ(mod->texts()[0].relocs[0].ref_index, 1);

    std::filesystem::remove(path);
}
