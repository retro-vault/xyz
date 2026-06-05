//
// lib_parser and library_reader tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

#include <xld/lib_parser.h>
#include <xld/rel_parser.h>

static std::string fixed_width(const std::string& value, std::size_t width) {
    ASSERT(value.size() <= width);
    return value + std::string(width - value.size(), ' ');
}

static void write_ar_member(std::ofstream& out,
                            const std::string& raw_name,
                            const std::string& data)
{
    std::string header;
    header += fixed_width(raw_name, 16);
    header += fixed_width("0", 12);
    header += fixed_width("0", 6);
    header += fixed_width("0", 6);
    header += fixed_width("100644", 8);
    header += fixed_width(std::to_string(data.size()), 10);
    header += "`\n";
    ASSERT_EQ(header.size(), 60);
    out.write(header.data(), header.size());
    out.write(data.data(), data.size());
    if ((data.size() & 1U) != 0)
        out.put('\n');
}

TEST(lib_parser_text_index_comments_and_blank_lines) {
    char tmp_template[] = "/tmp/xld-lib-index-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path lib_path = tmp_template;

    {
        std::ofstream out(lib_path);
        out << "# comment\n\n"
            << "mod1.rel\n"
            << "  mod2.rel  \n";
    }

    auto members = xld::lib_parser::parse(lib_path);
    ASSERT_EQ(static_cast<int>(members.size()), 2);
    ASSERT_EQ(members[0].path.filename(), std::filesystem::path("mod1.rel"));
    ASSERT_EQ(members[1].path.filename(), std::filesystem::path("mod2.rel"));
    ASSERT(!members[0].contents.has_value());

    std::filesystem::remove(lib_path);
}

TEST(lib_parser_ar_archive_long_names) {
    char tmp_template[] = "/tmp/xld-lib-archive-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path lib_path = tmp_template;

    const std::string long_names = "verylonghelper.rel/\n";
    const std::string rel_body =
        "XL\n"
        "H 1 areas 1 global symbols\n"
        "M helper\n"
        "A _CODE size 0001 flags 0\n"
        "S _helper Def0000\n"
        "T 00 00 C9\n"
        "R 00 00 00 00\n";

    {
        std::ofstream out(lib_path, std::ios::binary);
        out << "!<arch>\n";
        write_ar_member(out, "//", long_names);
        write_ar_member(out, "/0", rel_body);
    }

    auto members = xld::lib_parser::parse(lib_path);
    ASSERT_EQ(static_cast<int>(members.size()), 1);
    ASSERT(members[0].contents.has_value());
    ASSERT_EQ(members[0].path.string(),
              lib_path.string() + "[verylonghelper.rel]");

    std::istringstream defs_input(members[0].contents.value());
    auto defs = xld::rel_parser::scan_defs(members[0].path.string(),
                                             defs_input);
    ASSERT_EQ(static_cast<int>(defs.size()), 1);
    ASSERT_EQ(defs[0], "_helper");

    std::istringstream parse_input(members[0].contents.value());
    auto mod = xld::rel_parser::parse(members[0].path.string(), parse_input);
    ASSERT_EQ(mod->name(), "helper");

    std::filesystem::remove(lib_path);
}
