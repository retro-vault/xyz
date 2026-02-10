// cli.hpp
//
// command-line argument parsing
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_CLI_HPP
#define XLINK_CLI_HPP

#include <filesystem>
#include <string>
#include <vector>

#include <xlink/types.hpp>

namespace xlink {

    struct cli_options {
        std::vector<std::filesystem::path> input_files;
        std::filesystem::path output_file = "a.out";
        std::string entry_symbol = "_main";
        std::vector<address_range> reserved_ranges;
        bool verbose = false;
        bool print_map = false;
        bool show_help = false;
    };

    class cli {
    public:
        static cli_options parse(int argc, char* argv[]);
        static void print_usage();
    };

} // namespace xlink

#endif // XLINK_CLI_HPP
