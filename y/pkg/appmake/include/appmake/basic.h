// Declares ZX Spectrum BASIC decoding helpers used by `appmake` to inspect
// loader programs and derive launch details from tape content.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#include "appmake/types.h"

namespace appmake {

// Filesystem namespace alias used by the public API.
namespace fs = std::filesystem;

// Return the printable text for one ZX Spectrum BASIC token.
std::string_view basic_token_text(uint8_t token);
// Decode one ZX Spectrum BASIC text byte stream into a readable string.
std::string decode_basic_text(std::span<const uint8_t> bytes);
// Extract one numeric argument that follows the named BASIC keyword.
std::optional<uint16_t> extract_keyword_number(std::string_view text, std::string_view keyword);
// Parse one normalized BASIC program from raw bytes.
basic_program parse_basic_program(const std::vector<uint8_t>& data);
// Parse one BASIC program from already-decoded tape files.
basic_program parse_basic_from_files(const std::vector<tap_file>& files);
// Parse one BASIC program from a generic supported tape file.
basic_program parse_basic_from_tape(const fs::path& path);
// Parse one BASIC program from a TAP file.
basic_program parse_basic_from_tap(const fs::path& path);
// Print a normalized BASIC listing.
void print_basic_listing(const basic_program& program);

}  // namespace appmake
