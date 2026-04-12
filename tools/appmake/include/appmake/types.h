#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace appmake {

constexpr uint8_t k_app_version = 0x01;
constexpr uint8_t k_header_size = 24;

constexpr uint8_t k_kind_tape_code = 0x01;
constexpr uint8_t k_kind_snapshot_48 = 0x02;

constexpr uint8_t k_flag_legacy_zx = 0x02;
constexpr uint8_t k_flag_has_state = 0x04;
constexpr uint8_t k_flag_absolute_load = 0x08;

constexpr uint16_t k_rom_top = 0x4000;
constexpr uint16_t k_sysvar_begin = 0x5c00;
constexpr uint16_t k_sysvar_end = 0x5d00;
constexpr std::size_t k_analysis_state_limit = 200000;
constexpr std::size_t k_analysis_steps_per_state = 2000;

struct app_header {
    uint8_t kind = 0;
    uint8_t flags = 0;
    uint16_t load_addr = 0;
    uint16_t entry_addr = 0;
    uint16_t payload_size = 0;
    uint16_t state_size = 0;
    uint16_t stack_ptr = 0;
    uint8_t tape_flag = 0;
    uint8_t tape_checksum = 0;
};

struct tap_code_block {
    std::string name;
    uint16_t load_addr = 0;
    std::vector<uint8_t> data;
};

struct snapshot_48 {
    uint16_t af = 0;
    uint16_t bc = 0;
    uint16_t de = 0;
    uint16_t hl = 0;
    uint16_t af_alt = 0;
    uint16_t bc_alt = 0;
    uint16_t de_alt = 0;
    uint16_t hl_alt = 0;
    uint16_t ix = 0;
    uint16_t iy = 0;
    uint16_t sp = 0;
    uint16_t pc = 0;
    uint8_t i = 0;
    uint8_t r = 0;
    uint8_t iff2 = 0;
    uint8_t im = 0;
    uint8_t border = 0;
    std::vector<uint8_t> ram;
};

struct zx_header_block {
    uint8_t type = 0;
    std::string name;
    uint16_t data_len = 0;
    uint16_t param1 = 0;
    uint16_t param2 = 0;
};

struct tap_file {
    zx_header_block header;
    std::vector<uint8_t> data;
    uint8_t tape_flag = 0;
    uint8_t tape_checksum = 0;
};

struct tape_list_entry {
    std::size_t index = 0;
    std::string source;
    std::string kind;
    std::string role;
    std::string name;
    std::optional<std::size_t> size;
    std::string details;
};

struct cli_options {
    std::optional<std::string> name;
    std::optional<std::string> app_name;
    std::optional<std::string> cart_name;
    std::optional<uint16_t> load_addr;
    std::optional<uint16_t> entry_addr;
    std::optional<uint16_t> stack_ptr;
};

struct basic_line {
    uint16_t number = 0;
    std::vector<uint8_t> bytes;
    std::string text;
};

struct basic_program {
    std::vector<basic_line> lines;
    std::optional<uint16_t> clear_addr;
    std::optional<uint16_t> usr_addr;
};

struct address_range {
    uint32_t begin = 0;
    uint32_t end = 0;
};

struct rom_dependency {
    uint16_t from = 0;
    uint16_t target = 0;
    std::string instruction;
};

struct sysvar_dependency {
    uint16_t pc = 0;
    uint16_t addr = 0;
    std::string instruction;
};

struct analysis_report {
    basic_program basic;
    uint16_t entry_addr = 0;
    uint16_t stack_ptr = 0;
    tap_file program;
    std::vector<tap_file> files;
    std::vector<tape_list_entry> listing;
    std::set<uint16_t> code_bytes;
    std::vector<rom_dependency> rom_dependencies;
    std::vector<sysvar_dependency> sysvar_dependencies;
    std::vector<std::pair<uint16_t, std::string>> instructions;
};

}  // namespace appmake
