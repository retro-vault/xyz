#include "appmake/util.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <stdexcept>

namespace appmake {

std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error(std::format("cannot open input file: {}", path.string()));
    }

    in.seekg(0, std::ios::end);
    const auto end = in.tellg();
    if (end < 0) {
        throw std::runtime_error(std::format("cannot determine file size: {}", path.string()));
    }

    std::vector<uint8_t> data(static_cast<std::size_t>(end));
    in.seekg(0, std::ios::beg);
    if (!data.empty()) {
        in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    if (!in && !data.empty()) {
        throw std::runtime_error(std::format("cannot read input file: {}", path.string()));
    }
    return data;
}

void write_file(const fs::path& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error(std::format("cannot open output file: {}", path.string()));
    }
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    if (!out) {
        throw std::runtime_error(std::format("cannot write output file: {}", path.string()));
    }
}

uint16_t rd16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) |
        (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t rd24(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
        (static_cast<uint32_t>(p[1]) << 8) |
        (static_cast<uint32_t>(p[2]) << 16);
}

uint32_t rd32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
        (static_cast<uint32_t>(p[1]) << 8) |
        (static_cast<uint32_t>(p[2]) << 16) |
        (static_cast<uint32_t>(p[3]) << 24);
}

void wr16(std::vector<uint8_t>& out, std::size_t off, uint16_t value) {
    out.at(off + 0) = static_cast<uint8_t>(value & 0xff);
    out.at(off + 1) = static_cast<uint8_t>((value >> 8) & 0xff);
}

void ensure_size(
    const std::vector<uint8_t>& bytes,
    std::size_t pos,
    std::size_t need,
    std::string_view what
) {
    if (pos + need > bytes.size()) {
        throw std::runtime_error(std::format("malformed {}: truncated data", what));
    }
}

std::string trim_name(std::string s) {
    while (!s.empty() && s.back() == ' ') {
        s.pop_back();
    }
    return s;
}

std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return s;
}

std::string upper_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return s;
}

uint16_t parse_u16(std::string_view text) {
    std::string s(text);
    std::size_t parsed = 0;
    const unsigned long value = std::stoul(s, &parsed, 0);
    if (parsed != s.size() || value > 0xffffUL) {
        throw std::runtime_error(std::format("invalid 16-bit value: {}", s));
    }
    return static_cast<uint16_t>(value);
}

bool tape_checksum_ok(std::span<const uint8_t> block) {
    if (block.size() < 2) {
        return false;
    }
    uint8_t x = 0;
    for (uint8_t b : block) {
        x ^= b;
    }
    return x == 0;
}

uint8_t tape_checksum_byte(uint8_t flag, std::span<const uint8_t> payload) {
    uint8_t x = flag;
    for (const uint8_t b : payload) {
        x ^= b;
    }
    return x;
}

}  // namespace appmake
