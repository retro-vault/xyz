// reloc.hpp
//
// Relocation descriptor used inside bfd::section.  Each reloc records
// where in a section a value must be patched, what kind of patch is
// needed, whether the reference is to a symbol or another section, and
// an optional addend.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XBFD_RELOC_HPP
#define XBFD_RELOC_HPP

#include <cstdint>
#include <string>

#include <xbfd/types.hpp>

namespace bfd {

    struct reloc {
        uint32_t    offset;         // byte offset within the section's data
        reloc_type  type;           // how to patch the location
        bool        sym_relative;   // true = symbol ref, false = section ref
        std::string name;           // symbol or section name being referenced
        int32_t     addend;         // value added to the resolved address
    };

} // namespace bfd

#endif // XBFD_RELOC_HPP
