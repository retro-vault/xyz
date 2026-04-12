#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace appmake {

namespace fs = std::filesystem;

std::vector<uint8_t> read_file(const fs::path& path);
void write_file(const fs::path& path, const std::vector<uint8_t>& data);

uint16_t rd16(const uint8_t* p);
uint32_t rd24(const uint8_t* p);
uint32_t rd32(const uint8_t* p);
void wr16(std::vector<uint8_t>& out, std::size_t off, uint16_t value);

void ensure_size(
    const std::vector<uint8_t>& bytes,
    std::size_t pos,
    std::size_t need,
    std::string_view what
);

std::string trim_name(std::string s);
std::string lower_copy(std::string s);
std::string upper_copy(std::string s);
uint16_t parse_u16(std::string_view text);

bool tape_checksum_ok(std::span<const uint8_t> block);
uint8_t tape_checksum_byte(uint8_t flag, std::span<const uint8_t> payload);

}  // namespace appmake
