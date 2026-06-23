//
// cli.h
//
// GNU-style command-line parsing for xobjcopy.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XOBJCOPY_CLI_HPP
#define XOBJCOPY_CLI_HPP

#include <filesystem>
#include <optional>
#include <string>

namespace xobjcopy {

    enum class target_kind {
        rel,
        elf,
        ar_text,
        ar_binary
    };

    struct cli_options {
        std::filesystem::path input_file;
        std::filesystem::path output_file;
        std::optional<target_kind> input_target;
        std::optional<target_kind> output_target;
        bool strip_debug = false;
        bool show_help = false;
        bool show_version = false;
    };

    class cli {
    public:
        static cli_options parse(int argc, char* argv[]);
        static void print_usage(const char* argv0);
    };

} // namespace xobjcopy

#endif // XOBJCOPY_CLI_HPP
