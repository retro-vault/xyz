//
// cli.cpp
//
// xobjcopy command-line parser.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>

#include <xobjcopy/cli.h>
#include <xobjcopy/errors.h>

namespace xobjcopy {

    namespace {

        static std::optional<target_kind> parse_target_name(
            const std::string& name)
        {
            if (name == "rel" || name == "sdcc-rel")
                return target_kind::rel;
            if (name == "elf" || name == "elf32-z80" || name == "elf32-littlez80")
                return target_kind::elf;
            if (name == "lib" || name == "text-archive")
                return target_kind::ar_text;
            if (name == "a" || name == "ar" || name == "gnu-ar"
                || name == "binary-archive") {
                return target_kind::ar_binary;
            }
            return std::nullopt;
        }

        static std::optional<target_kind> infer_target_from_extension(
            const std::filesystem::path& path)
        {
            auto ext = path.extension().string();
            for (auto& c : ext)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (ext == ".rel")
                return target_kind::rel;
            if (ext == ".o" || ext == ".obj" || ext == ".elf")
                return target_kind::elf;
            if (ext == ".lib")
                return target_kind::ar_text;
            if (ext == ".a")
                return target_kind::ar_binary;
            return std::nullopt;
        }

    } // namespace

    cli_options cli::parse(int argc, char* argv[]) {
        cli_options opts;
        std::optional<std::filesystem::path> positional_output;

        if (argc < 2) {
            opts.show_help = true;
            return opts;
        }

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                opts.show_help = true;
                return opts;
            } else if (arg == "--version") {
                opts.show_version = true;
                return opts;
            } else if (arg == "-g" || arg == "--strip-debug") {
                opts.strip_debug = true;
            } else if (arg == "-I") {
                if (++i >= argc)
                    throw usage_error("-I requires an argument");
                auto target = parse_target_name(argv[i]);
                if (!target.has_value())
                    throw usage_error("unsupported input target: " + std::string(argv[i]));
                opts.input_target = target;
            } else if (arg.rfind("--input-target=", 0) == 0) {
                auto value = arg.substr(std::string("--input-target=").size());
                auto target = parse_target_name(value);
                if (!target.has_value())
                    throw usage_error("unsupported input target: " + value);
                opts.input_target = target;
            } else if (arg == "-O") {
                if (++i >= argc)
                    throw usage_error("-O requires an argument");
                auto target = parse_target_name(argv[i]);
                if (!target.has_value())
                    throw usage_error("unsupported output target: " + std::string(argv[i]));
                opts.output_target = target;
            } else if (arg.rfind("--output-target=", 0) == 0) {
                auto value = arg.substr(std::string("--output-target=").size());
                auto target = parse_target_name(value);
                if (!target.has_value())
                    throw usage_error("unsupported output target: " + value);
                opts.output_target = target;
            } else if (arg == "-o") {
                if (++i >= argc)
                    throw usage_error("-o requires an argument");
                opts.output_file = argv[i];
            } else if (arg.rfind("-o", 0) == 0 && arg.size() > 2) {
                opts.output_file = arg.substr(2);
            } else if (!arg.empty() && arg[0] == '-') {
                throw usage_error("unknown option: " + arg);
            } else if (opts.input_file.empty()) {
                opts.input_file = arg;
            } else if (!positional_output.has_value()) {
                positional_output = std::filesystem::path(arg);
            } else {
                throw usage_error("too many positional arguments");
            }
        }

        if (opts.input_file.empty())
            throw usage_error("no input file");

        if (opts.output_file.empty()) {
            if (positional_output.has_value()) {
                opts.output_file = positional_output.value();
            } else {
                auto out = opts.input_file;
                if (opts.output_target.has_value()) {
                    switch (opts.output_target.value()) {
                    case target_kind::rel:
                        out.replace_extension(".rel");
                        break;
                    case target_kind::elf:
                        out.replace_extension(".o");
                        break;
                    case target_kind::ar_text:
                        out.replace_extension(".lib");
                        break;
                    case target_kind::ar_binary:
                        out.replace_extension(".a");
                        break;
                    }
                } else {
                    out += ".copy";
                }
                opts.output_file = out;
            }
        } else if (positional_output.has_value()) {
            throw usage_error("use either -o or a positional output, not both");
        }

        if (!opts.output_target.has_value()) {
            opts.output_target = infer_target_from_extension(opts.output_file);
        }

        return opts;
    }

    void cli::print_usage(const char* argv0) {
        std::cerr
            << "Usage: " << argv0 << " [options] <input> [output]\n"
            << "\n"
            << "X Tools Object Copy (xobjcopy) — Z80 object/archive converter\n"
            << "\n"
            << "options:\n"
            << "  -I <bfdname>              Input target format\n"
            << "  --input-target=<bfdname>  Input target format\n"
            << "  -O <bfdname>              Output target format\n"
            << "  --output-target=<bfdname> Output target format\n"
            << "  -o <file>                 Output file (or use positional [output])\n"
            << "  -g, --strip-debug         Remove inline debug sections/metadata\n"
            << "  --version                 Show version\n"
            << "  -h, --help                Show this help\n"
            << "\n"
            << "supported targets:\n"
            << "  rel, sdcc-rel             SDCC .rel object\n"
            << "  elf, elf32-z80, elf32-littlez80\n"
            << "                            GNU ELF32 Z80 object\n"
            << "  lib, text-archive         xobjcopy text-index archive\n"
            << "  a, ar, gnu-ar, binary-archive\n"
            << "                            GNU ar archive\n";
    }

} // namespace xobjcopy
