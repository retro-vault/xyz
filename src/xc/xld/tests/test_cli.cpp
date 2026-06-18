//
// cli option parsing tests
//
// MIT License (see: LICENSE)
//
#include <xld/cli.h>
#include <xbfd/lscript.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

static std::filesystem::path cli_fixture_path(const std::string& name) {
    std::filesystem::path p = "tests/fixtures/" + name;
    if (std::filesystem::exists(p)) return p;
    p = std::filesystem::path(__FILE__).parent_path() / "fixtures" / name;
    return p;
}

TEST(cli_gnu_mode_defaults_entry_and_text_aliases) {
    std::vector<std::string> args = {
        "xld",
        "--mode=gnu",
        "-Ttext=0100",
        "-Tdata=2000",
        "-Tbss=3000",
        "main.o"
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    ASSERT(opts.mode == xld::link_mode::gnu);
    ASSERT_EQ(opts.entry_symbol, std::string("_start"));
    ASSERT_EQ(opts.area_bases[".text"], 0x0100);
    ASSERT_EQ(opts.area_bases[".data"], 0x2000);
    ASSERT_EQ(opts.area_bases[".bss"], 0x3000);
}

TEST(cli_gnu_linker_script_applies_defaults) {
    auto script = cli_fixture_path("script_gnu.ld");
    std::vector<std::string> args = {
        "xld",
        "--mode=gnu",
        "-T", script.string(),
        "main.o"
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    ASSERT(opts.script_file.has_value());
    ASSERT_EQ(opts.entry_symbol, std::string("_gnu_entry"));
    ASSERT_EQ(opts.format, xld::output_format::bin);
    ASSERT(opts.output_range.has_value());
    ASSERT_EQ(opts.output_range->start, 0x0000);
    ASSERT_EQ(opts.output_range->end, 0x3FFF);
    ASSERT_EQ(opts.area_bases[".text"], 0x0200);
    ASSERT_EQ(opts.area_bases[".data"], 0x4100);
    ASSERT_EQ(opts.area_bases[".bss"], 0x4000);
    ASSERT_EQ(static_cast<int>(opts.reserved_ranges.size()), 1);
    ASSERT_EQ(opts.reserved_ranges[0].start, 0x0100);
    ASSERT_EQ(opts.reserved_ranges[0].end, 0x010F);
}

TEST(cli_gnu_realistic_rom_script_extracts_order_and_regions) {
    auto script = cli_fixture_path("script_gnu_rom.ld");
    std::vector<std::string> args = {
        "xld",
        "--mode=gnu",
        "-T", script.string(),
        "main.o"
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    ASSERT(opts.script_file.has_value());
    ASSERT_EQ(opts.entry_symbol, std::string("_start"));
    ASSERT_EQ(opts.format, xld::output_format::bin);
    ASSERT(opts.output_range.has_value());
    ASSERT_EQ(opts.output_range->start, 0x0000);
    ASSERT_EQ(opts.output_range->end, 0x3FFF);
    ASSERT_EQ(opts.area_bases[".text"], 0x0000);
    ASSERT_EQ(opts.area_bases[".data"], 0x8000);
    ASSERT(opts.area_bases.find(".bss") == opts.area_bases.end());
    ASSERT_EQ(static_cast<int>(opts.area_order.size()), 5);
    ASSERT_EQ(opts.area_order[0], std::string(".text"));
    ASSERT_EQ(opts.area_order[1], std::string(".rodata"));
    ASSERT_EQ(opts.area_order[2], std::string(".vectors"));
    ASSERT_EQ(opts.area_order[3], std::string(".data"));
    ASSERT_EQ(opts.area_order[4], std::string(".bss"));
}

TEST(cli_sdcc_linker_script_applies_defaults) {
    auto script = cli_fixture_path("script_sdcc.lk");
    std::vector<std::string> args = {
        "xld",
        "-T", script.string(),
        "main.rel"
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    ASSERT(opts.script_file.has_value());
    ASSERT_EQ(opts.entry_symbol, std::string("_script_main"));
    ASSERT_EQ(opts.format, xld::output_format::bin);
    ASSERT(opts.output_range.has_value());
    ASSERT_EQ(opts.output_range->start, 0x0000);
    ASSERT_EQ(opts.output_range->end, 0x7FFF);
    ASSERT_EQ(opts.area_bases["_CODE"], 0x0200);
    ASSERT_EQ(opts.area_bases["_DATA"], 0x4000);
    ASSERT_EQ(opts.area_bases["_BSS"], 0x4100);
    ASSERT_EQ(static_cast<int>(opts.reserved_ranges.size()), 1);
    ASSERT_EQ(opts.reserved_ranges[0].start, 0x0100);
    ASSERT_EQ(opts.reserved_ranges[0].end, 0x010F);
}

TEST(cli_sdcc_command_file_style_script_is_accepted) {
    auto script = cli_fixture_path("script_sdcc_cmd.lk");
    auto parsed = xbfd::lscript::open(script, xbfd::lscript_mode::sdcc);

    ASSERT(parsed->output_format().has_value());
    ASSERT_EQ(*parsed->output_format(), xbfd::lscript_output_format::ihx);
    ASSERT_EQ(parsed->area_bases().at("_CODE"), 0x0000);
    ASSERT_EQ(parsed->area_bases().at("_DATA"), 0x4000);
    ASSERT_EQ(parsed->area_bases().at("_BSS"), 0x8000);
    ASSERT_EQ(parsed->area_bases().at("_RST8"), 0x0008);
    ASSERT_EQ(parsed->area_bases().at("_NMI"), 0x0066);
}

TEST(cli_linker_script_cli_options_override_script) {
    auto script = cli_fixture_path("script_gnu.ld");
    std::vector<std::string> args = {
        "xld",
        "--mode=gnu",
        "--script", script.string(),
        "-e", "_cli_entry",
        "-Ttext=0300",
        "-f", "xl",
        "main.o"
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    ASSERT_EQ(opts.entry_symbol, std::string("_cli_entry"));
    ASSERT_EQ(opts.area_bases[".text"], 0x0300);
    ASSERT_EQ(opts.format, xld::output_format::xl);
    ASSERT_EQ(opts.area_bases[".data"], 0x4100);
    ASSERT_EQ(static_cast<int>(opts.reserved_ranges.size()), 1);
}

TEST(cli_supports_ihx_output_and_map_file) {
    std::vector<std::string> args = {
        "xld",
        "--mode=sdcc",
        "--oformat=ihx",
        "-Map=out.map",
        "main.rel"
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());

    ASSERT_EQ(opts.format, xld::output_format::ihx);
    ASSERT(opts.map_file.has_value());
    ASSERT_EQ(opts.map_file->string(), std::string("out.map"));
}

TEST(cli_resolves_dash_l_against_dash_L_paths) {
    auto dir = std::filesystem::temp_directory_path() / "xld-cli-dash-l-test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    auto lib = dir / "libfixed.a";
    std::ofstream(lib) << "!<arch>\n";

    std::vector<std::string> args = {
        "xld",
        "-L" + dir.string(),
        "-lfixed",
        "main.rel"
    };

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args)
        argv.push_back(arg.data());

    auto opts = xld::cli::parse(static_cast<int>(argv.size()), argv.data());
    xld::cli::resolve_libraries(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 2);
    ASSERT_EQ(opts.input_files[0].string(), std::string("main.rel"));
    ASSERT_EQ(opts.input_files[1], std::filesystem::weakly_canonical(lib));

    std::filesystem::remove_all(dir);
}
