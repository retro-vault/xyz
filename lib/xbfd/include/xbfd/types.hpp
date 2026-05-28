// types.hpp
//
// Core enums and bitfield types for the libbfd object-file abstraction.
// Covers byte order, section flags, symbol flags, relocation kinds,
// object format, archive format, and file flavour.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XBFD_TYPES_HPP
#define XBFD_TYPES_HPP

#include <cstdint>
#include <type_traits>

namespace bfd {

    // ---------------------------------------------------------------------------
    // byte_order
    // ---------------------------------------------------------------------------
    enum class byte_order {
        little_endian,
        big_endian
    };

    // ---------------------------------------------------------------------------
    // format  — mirrors bfd_format
    // ---------------------------------------------------------------------------
    enum class format {
        unknown,
        object,     // relocatable object (.rel / .o / .elf)
        archive,    // library (.lib / .a)
        core        // core dump (not used, reserved)
    };

    // ---------------------------------------------------------------------------
    // flavour — what on-disk encoding is used
    // ---------------------------------------------------------------------------
    enum class flavour {
        unknown,
        rel,        // SDCC text-format .rel
        elf,        // ELF32 (z80-elf target)
        ar_text,    // SDCC text-index .lib  (list of paths)
        ar_binary   // BSD/GNU ar archive
    };

    // ---------------------------------------------------------------------------
    // section_flags — mirrors SEC_* bits
    // ---------------------------------------------------------------------------
    enum class section_flags : uint32_t {
        none        = 0x0000,
        alloc       = 0x0001,   // takes up space at run time
        load        = 0x0002,   // loaded from file
        code        = 0x0004,   // executable
        data        = 0x0008,   // initialised data (read/write)
        readonly    = 0x0010,   // read-only data
        reloc       = 0x0020,   // contains relocations
        debugging   = 0x0040,   // debug section
        never_load  = 0x0080,   // not loaded (NOLOAD)
        abs         = 0x0100,   // absolute origin
        overlay     = 0x0200    // SDCC OVR flag
    };

    inline section_flags operator|(section_flags a, section_flags b) {
        return static_cast<section_flags>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    inline section_flags operator&(section_flags a, section_flags b) {
        return static_cast<section_flags>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }
    inline bool has_flag(section_flags val, section_flags f) {
        return (val & f) == f;
    }

    // ---------------------------------------------------------------------------
    // symbol_flags — mirrors BSF_* bits
    // ---------------------------------------------------------------------------
    enum class symbol_flags : uint32_t {
        none        = 0x0000,
        local       = 0x0001,   // local (not exported)
        global      = 0x0002,   // externally visible
        undefined   = 0x0004,   // referenced but not defined here
        function    = 0x0008,   // entry point
        object      = 0x0010,   // data object
        weak        = 0x0020,   // weak reference
        absolute    = 0x0040    // value is absolute (not section-relative)
    };

    inline symbol_flags operator|(symbol_flags a, symbol_flags b) {
        return static_cast<symbol_flags>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    inline symbol_flags operator&(symbol_flags a, symbol_flags b) {
        return static_cast<symbol_flags>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }
    inline bool has_flag(symbol_flags val, symbol_flags f) {
        return (val & f) == f;
    }

    // ---------------------------------------------------------------------------
    // reloc_type — target-specific relocation kinds
    // ---------------------------------------------------------------------------
    enum class reloc_type : uint8_t {
        none        = 0,
        z80_8       = 1,    // 8-bit absolute
        z80_pc8     = 3,    // 8-bit PC-relative (JR/DJNZ offset)
        z80_16      = 4,    // 16-bit absolute (most Z80 addresses)
        z80_16_msb  = 5     // high byte of 16-bit address
    };

} // namespace bfd

#endif // XBFD_TYPES_HPP
