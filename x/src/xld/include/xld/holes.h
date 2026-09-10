// holes.h
//
// reserved address range normalization and pre-hole skip instructions
//
// A reserved range n..m must never receive code.  Flat images plant a skip
// instruction immediately before n so that straight-line execution reaching
// the range continues at m+1.  Placement and emission both derive their
// view of the reserved space from this one place, so the bytes the placer
// keeps free are exactly the bytes the emitter writes.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XLINK_HOLES_HPP
#define XLINK_HOLES_HPP

#include <cstdint>
#include <vector>

#include <xld/types.h>

namespace xld {

    // A synthesized JR/JP planted immediately before a reserved range.
    struct hole_guard {
        uint16_t address = 0;           // first byte of the instruction
        std::vector<uint8_t> bytes;     // JR d, or JP nn
    };

    // Sort reserved ranges and fuse the ones that overlap or touch.
    std::vector<address_range> merge_reserved_ranges(
        const std::vector<address_range>& ranges);

    // Reserved ranges as a flat image window sees them: merged, clipped to
    // the window, and fused again whenever the gap between two ranges is too
    // small to hold the skip instruction of the second one.  Such a gap can
    // never be used, so it belongs to the reserved space.
    std::vector<address_range> clipped_reserved_ranges(
        const std::vector<address_range>& ranges,
        uint16_t window_start,
        uint16_t window_end);

    // Skip instructions for the ranges returned by clipped_reserved_ranges.
    // A range with no room for either form gets no guard.
    std::vector<hole_guard> reserved_range_guards(
        const std::vector<address_range>& ranges,
        uint16_t window_start,
        uint16_t window_end);

} // namespace xld

#endif // XLINK_HOLES_HPP
