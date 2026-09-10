// text_record.h
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

#include <xld/types.h>

namespace xld {

    // A single relocation entry from an R record.
    struct reloc_entry {
        reloc_mode mode;        // flags: word/byte, area/sym, pc-rel, msb
        uint16_t offset_in_t;   // offset into the T record data
        int ref_index;          // index of referenced area or symbol

        // Constant added to the resolved symbol or area address. ASxxxx
        // keeps it in the data bytes and GNU keeps it in the relocation
        // record; the object readers normalize both into this field, so the
        // relocator never has to re-derive it from the image.
        int32_t addend = 0;

        // Where a PC-relative relocation is measured from. GNU applies it at
        // the relocated byte itself (S + A - P) and biases the addend to
        // match; ASxxxx applies it after the field and leaves the bias to the
        // linker (S + A - (P + size)).
        bool pc_rel_at_field = false;
    };

    // A T record: code bytes for an area at a given offset.
    struct text_record {
        int area_index;                 // which area this belongs to
        uint16_t offset;                // offset within the area
        std::vector<uint8_t> data;      // raw code/data bytes
        std::vector<reloc_entry> relocs;// associated relocations
    };

} // namespace xld

#endif // XLINK_TEXT_RECORD_HPP
