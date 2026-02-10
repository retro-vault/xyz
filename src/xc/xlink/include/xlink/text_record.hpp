// text_record.hpp
//
// text record and relocation entry
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_TEXT_RECORD_HPP
#define XLINK_TEXT_RECORD_HPP

#include <cstdint>
#include <vector>

#include <xlink/types.hpp>

namespace xlink {

    // A single relocation entry from an R record.
    struct reloc_entry {
        reloc_mode mode;        // flags: word/byte, area/sym, pc-rel, msb
        uint16_t offset_in_t;   // offset into the T record data
        int ref_index;          // index of referenced area or symbol
    };

    // A T record: code bytes for an area at a given offset.
    struct text_record {
        int area_index;                 // which area this belongs to
        uint16_t offset;                // offset within the area
        std::vector<uint8_t> data;      // raw code/data bytes
        std::vector<reloc_entry> relocs;// associated relocations
    };

} // namespace xlink

#endif // XLINK_TEXT_RECORD_HPP
