// types.hpp
//
// enums, strong types, bitwise helpers
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_TYPES_HPP
#define XLINK_TYPES_HPP

#include <cstdint>
#include <type_traits>

namespace xlink {

    // Byte order for the target architecture.
    enum class byte_order {
        little_endian,
        big_endian
    };

    // Area flags from SDCC .rel format.
    // Bit 0: OVR (1) vs CON (0)
    // Bit 2: ABS (1) vs REL (0)
    enum class area_flags : uint8_t {
        none    = 0x00,
        ovr     = 0x01,     // overlay mode (vs concatenate)
        abs     = 0x04      // absolute area (vs relocatable)
    };

    inline area_flags operator|(area_flags a, area_flags b) {
        return static_cast<area_flags>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline area_flags operator&(area_flags a, area_flags b) {
        return static_cast<area_flags>(
            static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    inline bool has_flag(area_flags val, area_flags flag) {
        return (val & flag) == flag;
    }

    // Symbol type: defined or referenced.
    enum class symbol_type {
        def,
        ref
    };

    // Relocation mode flags from SDCC R records.
    // Bit 0: byte (0) or word (1)
    // Bit 1: area (0) or symbol (1) reference
    // Bit 2: PC-relative (1) or absolute (0)
    // Bit 7: MSB (1) or full/LSB (0) for byte relocs
    enum class reloc_mode : uint8_t {
        none        = 0x00,
        word        = 0x01,     // 16-bit relocation
        sym         = 0x02,     // symbol reference (vs area)
        pc_rel      = 0x04,     // PC-relative
        msb         = 0x80      // MSB byte only
    };

    inline reloc_mode operator|(reloc_mode a, reloc_mode b) {
        return static_cast<reloc_mode>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline reloc_mode operator&(reloc_mode a, reloc_mode b) {
        return static_cast<reloc_mode>(
            static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    inline bool has_flag(reloc_mode val, reloc_mode flag) {
        return (val & flag) == flag;
    }

    // Reserved address range (hole in memory map).
    struct address_range {
        uint16_t start;
        uint16_t end;       // inclusive
    };

} // namespace xlink

#endif // XLINK_TYPES_HPP
