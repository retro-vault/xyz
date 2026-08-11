#ifndef XPROG_FORMAT_H
#define XPROG_FORMAT_H

#include <cstddef>
#include <cstdint>

namespace xprog {

constexpr std::size_t header_size = 64;
constexpr std::size_t jump_entry_size = 3;
constexpr std::uint8_t format_version = 1;
constexpr std::uint16_t no_entry = 0xffff;

enum class image_kind : std::uint8_t {
    process = 1,
    service = 2
};

enum image_flags : std::uint8_t {
    fixed_load = 0x01,
    has_entry = 0x02,
    has_jump_table = 0x04
};

} // namespace xprog

#endif
