// cli.h
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
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <xld/types.h>

namespace xld {

    enum class link_mode {
        sdcc,
        gnu
    };

    struct cli_options {
        std::vector<std::filesystem::path> input_files;
        std::vector<std::filesystem::path> library_search_paths;
        std::vector<std::string> libraries;
        std::filesystem::path output_file = "a.out";
        std::optional<std::filesystem::path> script_file;
        std::optional<std::filesystem::path> cdb_file;
        std::optional<std::filesystem::path> map_file;
        std::optional<std::filesystem::path> sdcc_runtime_dir;
        std::string invocation_target;
        std::optional<std::string> platform_name;
        std::string entry_symbol = "_main";
        link_mode mode = link_mode::sdcc;
        std::vector<address_range> reserved_ranges;
        std::map<std::string, uint16_t> area_bases;
        std::vector<std::string> area_order;
        std::vector<std::string> load_copy_areas;
        std::optional<address_range> output_range;
        output_format format = output_format::xl;
        std::set<std::string> explicit_area_bases;
        bool entry_symbol_explicit = false;
        bool output_range_explicit = false;
        bool format_explicit = false;
        bool debug_info = false;
        bool no_startfiles = false;
        bool no_stdlib = false;
        bool disable_default_sdcc_runtime = false;
        bool verbose = false;
        bool print_map = false;
        bool show_help = false;
        bool show_version = false;
    };

    class cli {
    public:
        static cli_options parse(int argc, char* argv[]);
        static void resolve_libraries(cli_options& opts);
        static void print_usage(const char* argv0);
    };

} // namespace xld

#endif // XLINK_CLI_HPP
