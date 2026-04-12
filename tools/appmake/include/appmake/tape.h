#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "appmake/types.h"

namespace appmake {

namespace fs = std::filesystem;

std::string zx_block_type_name(uint8_t type);
std::string zx_code_role(uint16_t load_addr, uint16_t data_len);
std::string zx_code_kind(uint16_t load_addr, uint16_t data_len);
std::string zx_header_details(const zx_header_block& header);
std::optional<zx_header_block> decode_zx_header(std::span<const uint8_t> block);
bool is_zx_data_block(std::span<const uint8_t> block);

std::vector<tape_list_entry> parse_tap_list(const fs::path& path);
std::vector<tape_list_entry> parse_tzx_list(const fs::path& path);
std::vector<tap_file> parse_tap_files(const fs::path& path);
std::vector<tap_file> parse_tzx_files(const fs::path& path);
std::vector<tap_file> parse_tape_files(const fs::path& path);
tap_code_block parse_tap_code(
    const fs::path& path,
    const std::optional<std::string>& wanted_name
);

}  // namespace appmake
