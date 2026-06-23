// Declares low-level utility helpers shared across the `appmake` codebase.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace appmake {

// Filesystem namespace alias used by the public API.
namespace fs = std::filesystem;

// Read one whole file into memory.
std::vector<uint8_t> read_file(const fs::path& path);
// Write one whole file to disk.
void write_file(const fs::path& path, const std::vector<uint8_t>& data);

// Read one little-endian 16-bit value.
uint16_t rd16(const uint8_t* p);
// Read one little-endian 24-bit value.
uint32_t rd24(const uint8_t* p);
// Read one little-endian 32-bit value.
uint32_t rd32(const uint8_t* p);
// Write one little-endian 16-bit value into an output buffer.
void wr16(std::vector<uint8_t>& out, std::size_t off, uint16_t value);

// Ensure a byte range exists before decoding structured tape data.
void ensure_size(
    const std::vector<uint8_t>& bytes,
    std::size_t pos,
    std::size_t need,
    std::string_view what
);

// Trim surrounding and trailing spaces from a ZX-style name.
std::string trim_name(std::string s);
// Return a lowercase copy of a string.
std::string lower_copy(std::string s);
// Return an uppercase copy of a string.
std::string upper_copy(std::string s);
// Parse a 16-bit integer from user-facing text.
uint16_t parse_u16(std::string_view text);

// Validate a ZX tape checksum over one block.
bool tape_checksum_ok(std::span<const uint8_t> block);
// Compute the ZX tape checksum byte for one flag and payload.
uint8_t tape_checksum_byte(uint8_t flag, std::span<const uint8_t> payload);

}  // namespace appmake
