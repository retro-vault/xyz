#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#include "appmake/types.h"

namespace appmake {

namespace fs = std::filesystem;

std::string_view basic_token_text(uint8_t token);
std::string decode_basic_text(std::span<const uint8_t> bytes);
std::optional<uint16_t> extract_keyword_number(std::string_view text, std::string_view keyword);
basic_program parse_basic_program(const std::vector<uint8_t>& data);
basic_program parse_basic_from_files(const std::vector<tap_file>& files);
basic_program parse_basic_from_tape(const fs::path& path);
basic_program parse_basic_from_tap(const fs::path& path);
void print_basic_listing(const basic_program& program);

}  // namespace appmake
