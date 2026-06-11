// cli.cpp
//
// command-line argument parsing
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <xld/cli.h>
#include <xld/errors.h>
#include <xbfd/lscript.h>

namespace xld {

    static uint16_t parse_hex16(const std::string& s) {
        return static_cast<uint16_t>(std::stoul(s, nullptr, 16));
    }

    static address_range parse_range_arg(const std::string& arg_name,
                                         const std::string& range_str)
    {
        auto dash = range_str.find('-');
        if (dash == std::string::npos) {
            throw xld_error(arg_name
                              + " format: start-end (hex), e.g. 4000-7FFF");
        }

        address_range r;
        r.start = parse_hex16(range_str.substr(0, dash));
        r.end = parse_hex16(range_str.substr(dash + 1));
        return r;
    }

    static std::pair<std::string, uint16_t> parse_area_base_arg(
        const std::string& arg_name,
        const std::string& value)
    {
        auto eq = value.find('=');
        if (eq == std::string::npos || eq == 0 || eq == value.size() - 1) {
            throw xld_error(arg_name
                              + " format: AREA=ADDR (hex), e.g. _CODE=0100");
        }

        return {value.substr(0, eq), parse_hex16(value.substr(eq + 1))};
    }

    static std::string require_arg(int argc, char* argv[], int& i,
                                   const std::string& arg_name)
    {
        if (++i >= argc)
            throw xld_error(arg_name + " requires an argument");
        return argv[i];
    }

    static xld::output_format to_xld_output_format(
        xbfd::lscript_output_format fmt,
        const std::filesystem::path& script_path)
    {
        switch (fmt) {
        case xbfd::lscript_output_format::xl:
            return xld::output_format::xl;
        case xbfd::lscript_output_format::bin:
            return xld::output_format::bin;
        case xbfd::lscript_output_format::ihx:
            return xld::output_format::ihx;
        case xbfd::lscript_output_format::elf:
            throw xld_error("linker script '" + script_path.string()
                            + "' requests an output format not implemented by xld");
        default:
            return xld::output_format::xl;
        }
    }

    static void apply_script_defaults(cli_options& opts,
                                      const xbfd::lscript& script,
                                      const std::filesystem::path& script_path)
    {
        if (script.entry_symbol().has_value())
            opts.entry_symbol = *script.entry_symbol();
        if (script.output_format().has_value())
            opts.format = to_xld_output_format(*script.output_format(),
                                               script_path);
        if (script.output_range().has_value()) {
            opts.output_range = xld::address_range{
                script.output_range()->start,
                script.output_range()->end
            };
        }
        for (const auto& [area_name, base] : script.area_bases())
            opts.area_bases[area_name] = base;
        for (const auto& area_name : script.area_order())
            opts.area_order.push_back(area_name);
        for (const auto& range : script.reserved_ranges()) {
            opts.reserved_ranges.push_back(xld::address_range{
                range.start,
                range.end
            });
        }
    }

    static bool is_script_switch(const std::string& arg) {
        return arg == "-T"
            || arg == "--script"
            || arg.rfind("--script=", 0) == 0
            || (arg.rfind("-T", 0) == 0
                && arg.rfind("-Ttext=", 0) != 0
                && arg.rfind("-Tdata=", 0) != 0
                && arg.rfind("-Tbss=", 0) != 0);
    }

    cli_options cli::parse(int argc, char* argv[]) {
        cli_options opts;
        bool entry_explicit = false;
        bool entry_from_script = false;
        std::optional<uint16_t> text_base_alias;
        std::optional<uint16_t> data_base_alias;
        std::optional<uint16_t> bss_base_alias;
        std::optional<std::filesystem::path> script_path;
        auto detected_mode = link_mode::sdcc;

        if (argc < 2) {
            opts.show_help = true;
            return opts;
        }

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                opts.show_help = true;
                return opts;
            }
            if (arg == "--version") {
                opts.show_version = true;
                return opts;
            }
            if (arg == "--mode=sdcc") {
                detected_mode = link_mode::sdcc;
                continue;
            }
            if (arg == "--mode=gnu") {
                detected_mode = link_mode::gnu;
                continue;
            }
            if (!is_script_switch(arg))
                continue;

            std::string value;
            if (arg == "-T" || arg == "--script") {
                value = require_arg(argc, argv, i, arg);
            } else if (arg.rfind("--script=", 0) == 0) {
                value = arg.substr(std::string("--script=").size());
            } else {
                value = arg.substr(2);
            }
            if (script_path.has_value()) {
                throw xld_error("multiple linker scripts are not supported");
            }
            script_path = std::filesystem::path(value);
        }

        opts.mode = detected_mode;
        opts.script_file = script_path;

        if (script_path.has_value()) {
            try {
                auto script = xbfd::lscript::open(
                    *script_path,
                    detected_mode == link_mode::gnu
                        ? xbfd::lscript_mode::gnu
                        : xbfd::lscript_mode::sdcc);
                apply_script_defaults(opts, *script, *script_path);
                entry_from_script = script->entry_symbol().has_value();
            } catch (const xbfd::lscript_error& e) {
                throw xld_error(e.what());
            }
        }

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                opts.show_help = true;
                return opts;
            } else if (arg == "--version") {
                opts.show_version = true;
                return opts;
            } else if (arg == "--mode=sdcc") {
                opts.mode = link_mode::sdcc;
            } else if (arg == "--mode=gnu") {
                opts.mode = link_mode::gnu;
            } else if (arg == "-g") {
                opts.debug_info = true;
            } else if (arg == "-c" || arg == "-n" || arg == "--cdb" || arg == "--noi") {
                throw xld_error(
                    "separate debug output switches are not supported; use -g");
            } else if (arg == "--sdcc-runtime" || arg == "-B") {
                opts.sdcc_runtime_dir = std::filesystem::path(
                    require_arg(argc, argv, i, arg));
            } else if (arg.rfind("-B", 0) == 0 && arg.size() > 2) {
                opts.sdcc_runtime_dir = std::filesystem::path(arg.substr(2));
            } else if (arg == "-nostartfiles") {
                opts.no_startfiles = true;
            } else if (arg == "-nostdlib") {
                opts.no_stdlib = true;
            } else if (arg == "-L" || arg.rfind("-L", 0) == 0) {
                throw xld_error("-L is not implemented yet");
            } else if (arg == "-l" || arg.rfind("-l", 0) == 0) {
                throw xld_error("-l is not implemented yet");
            } else if (arg == "-T" || arg == "--script") {
                ++i;
            } else if (arg.rfind("--script=", 0) == 0) {
                continue;
            } else if (arg.rfind("-T", 0) == 0
                       && arg.rfind("-Ttext=", 0) != 0
                       && arg.rfind("-Tdata=", 0) != 0
                       && arg.rfind("-Tbss=", 0) != 0) {
                continue;
            } else if (arg.rfind("-Map=", 0) == 0) {
                opts.map_file = std::filesystem::path(arg.substr(5));
            } else if (arg == "-Map") {
                opts.map_file = std::filesystem::path(
                    require_arg(argc, argv, i, arg));
            } else if (arg == "-o") {
                opts.output_file = require_arg(argc, argv, i, arg);
            } else if (arg.rfind("-o", 0) == 0 && arg.size() > 2) {
                opts.output_file = arg.substr(2);
            } else if (arg == "-e") {
                opts.entry_symbol = require_arg(argc, argv, i, arg);
                entry_explicit = true;
            } else if (arg == "-r") {
                opts.reserved_ranges.push_back(parse_range_arg(
                    arg, require_arg(argc, argv, i, arg)));
            } else if (arg.rfind("--reserve=", 0) == 0) {
                opts.reserved_ranges.push_back(parse_range_arg(
                    "--reserve", arg.substr(std::string("--reserve=").size())));
            } else if (arg == "-b") {
                auto [area_name, base] = parse_area_base_arg(
                    arg, require_arg(argc, argv, i, arg));
                opts.area_bases[area_name] = base;
            } else if (arg.rfind("--section-start=", 0) == 0) {
                auto [area_name, base] = parse_area_base_arg(
                    "--section-start",
                    arg.substr(std::string("--section-start=").size()));
                opts.area_bases[area_name] = base;
            } else if (arg.rfind("-Ttext=", 0) == 0) {
                text_base_alias =
                    parse_hex16(arg.substr(std::string("-Ttext=").size()));
            } else if (arg.rfind("-Tdata=", 0) == 0) {
                data_base_alias =
                    parse_hex16(arg.substr(std::string("-Tdata=").size()));
            } else if (arg.rfind("-Tbss=", 0) == 0) {
                bss_base_alias =
                    parse_hex16(arg.substr(std::string("-Tbss=").size()));
            } else if (arg == "-f") {
                std::string format = require_arg(argc, argv, i, arg);
                if (format == "xl") {
                    opts.format = output_format::xl;
                } else if (format == "bin" || format == "binary") {
                    opts.format = output_format::bin;
                } else if (format == "ihx") {
                    opts.format = output_format::ihx;
                } else if (format == "elf") {
                    throw xld_error("output format '" + format
                                      + "' is not implemented yet");
                } else {
                    throw xld_error("unsupported output format: " + format);
                }
            } else if (arg.rfind("--oformat=", 0) == 0) {
                std::string format = arg.substr(std::string("--oformat=").size());
                if (format == "xl") {
                    opts.format = output_format::xl;
                } else if (format == "binary") {
                    opts.format = output_format::bin;
                } else if (format == "ihx") {
                    opts.format = output_format::ihx;
                } else if (format == "elf") {
                    throw xld_error("output format '" + format
                                      + "' is not implemented yet");
                } else {
                    throw xld_error("unsupported output format: " + format);
                }
            } else if (arg == "-x") {
                opts.output_range = parse_range_arg(
                    arg, require_arg(argc, argv, i, arg));
            } else if (arg.rfind("--binary-range=", 0) == 0) {
                opts.output_range = parse_range_arg(
                    "--binary-range",
                    arg.substr(std::string("--binary-range=").size()));
            } else if (arg == "-m" || arg == "-M" || arg == "--print-map") {
                opts.print_map = true;
            } else if (arg == "-v" || arg == "--verbose") {
                opts.verbose = true;
            } else if (arg[0] == '-') {
                throw xld_error("unknown option: " + arg);
            } else {
                opts.input_files.emplace_back(arg);
            }
        }

        if (!entry_explicit && !entry_from_script && opts.mode == link_mode::gnu)
            opts.entry_symbol = "_start";

        if (text_base_alias.has_value()) {
            opts.area_bases[opts.mode == link_mode::gnu ? ".text" : "_CODE"] =
                text_base_alias.value();
        }
        if (data_base_alias.has_value()) {
            opts.area_bases[opts.mode == link_mode::gnu ? ".data" : "_DATA"] =
                data_base_alias.value();
        }
        if (bss_base_alias.has_value()) {
            opts.area_bases[opts.mode == link_mode::gnu ? ".bss" : "_BSS"] =
                bss_base_alias.value();
        }

        if (!opts.show_help && !opts.show_version && opts.input_files.empty())
            throw xld_error("no input files");

        return opts;
    }

    void cli::print_usage(const char* argv0) {
        std::cerr
            << "Usage: " << argv0 << " [options] <input> ...\n"
            << "\n"
            << "X Linker (xld) - linker for Z80\n"
            << "\n"
            << "options:\n"
            << "  -o <file>                  Output file (default: a.out)\n"
            << "  -e <symbol>                Entry symbol\n"
            << "                             (default: _main with --mode=sdcc,\n"
            << "                                       _start with --mode=gnu)\n"
            << "  --mode=sdcc                Accept SDCC/ASxxxx inputs (default)\n"
            << "  --mode=gnu                 Accept GNU inputs\n"
            << "  -B <prefix>                Add startup/runtime/toolchain search prefix\n"
            << "  -L<dir>                    Add library search directory\n"
            << "  -l<name>                   Link against library\n"
            << "  -nostartfiles              Do not use implicit startup files\n"
            << "  -nostdlib                  Do not use implicit startup files or default libs\n"
            << "  --oformat=xl               Emit XL relocatable image (default)\n"
            << "  --oformat=binary           Emit flat binary image\n"
            << "  --oformat=elf              Emit ELF image\n"
            << "  --oformat=ihx              Emit Intel HEX image\n"
            << "  -T <file>                  Use linker script\n"
            << "  --script=<file>            Use linker script (long form)\n"
            << "  --section-start=<name>=<addr>\n"
            << "                             Set base address for named section/area\n"
            << "  -Ttext=<addr>              Alias for text/code base address\n"
            << "  -Tdata=<addr>              Alias for data base address\n"
            << "  -Tbss=<addr>               Alias for bss base address\n"
            << "  --binary-range=<lo>-<hi>   Limit emitted range for --oformat=binary\n"
            << "  --reserve=<lo>-<hi>        Reserve address range (repeatable)\n"
            << "  -g                         Emit debug outputs for the selected mode\n"
            << "  -M, --print-map            Print memory map to stdout\n"
            << "  -Map=<file>                Write memory map to file\n"
            << "  -v, --verbose              Verbose output\n"
            << "  --version                  Print version\n"
            << "  -h, --help                 Show this help\n";
    }

} // namespace xld
