#ifndef XPROG_CLI_H
#define XPROG_CLI_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <xprog/format.h>

namespace xprog {

enum class command_kind { none, process, service, inspect, tap, tzx };

struct cli_options {
    command_kind command = command_kind::none;
    std::filesystem::path input_file;
    std::filesystem::path output_file;
    std::string name;
    std::optional<std::uint32_t> image_id;
    std::uint8_t abi_version = 1;
    std::uint16_t minimum_os_version = 0;
    std::uint16_t load_address = 0;
    std::optional<std::uint16_t> entry_point;
    std::optional<std::uint16_t> stack_size;
    bool require_fixed_load = false;
    std::vector<std::uint16_t> exports;
    bool show_help = false;
    bool show_version = false;
};

class cli {
public:
    static cli_options parse(int argc, char* argv[]);
    static void print_usage(const char* argv0);
};

} // namespace xprog

#endif
