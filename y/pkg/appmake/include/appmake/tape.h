// Declares tape-decoding helpers for TAP and TZX files used by `appmake`.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "appmake/types.h"

namespace appmake {

// Filesystem namespace alias used by the public API.
namespace fs = std::filesystem;

// Return the ZX header block type name.
std::string zx_block_type_name(uint8_t type);
// Classify one ZX code block by loader role.
std::string zx_code_role(uint16_t load_addr, uint16_t data_len);
// Classify one ZX code block by binary kind.
std::string zx_code_kind(uint16_t load_addr, uint16_t data_len);
// Format one ZX header block into a readable summary.
std::string zx_header_details(const zx_header_block& header);
// Decode one 17-byte ZX header block when valid.
std::optional<zx_header_block> decode_zx_header(std::span<const uint8_t> block);
// Check whether one tape block is a ZX data block.
bool is_zx_data_block(std::span<const uint8_t> block);

// Return a listing view of a TAP file.
std::vector<tape_list_entry> parse_tap_list(const fs::path& path);
// Return a listing view of a TZX file.
std::vector<tape_list_entry> parse_tzx_list(const fs::path& path);
// Parse all logical files from a TAP image.
std::vector<tap_file> parse_tap_files(const fs::path& path);
// Parse all logical files from a TZX image.
std::vector<tap_file> parse_tzx_files(const fs::path& path);
// Parse all logical files from a supported tape image.
std::vector<tap_file> parse_tape_files(const fs::path& path);
// Extract one code payload from a TAP image.
tap_code_block parse_tap_code(
    const fs::path& path,
    const std::optional<std::string>& wanted_name
);

}  // namespace appmake
