// holes.cpp
//
// reserved address range normalization and pre-hole skip instructions
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>

#include <xld/holes.h>

namespace xld {

    namespace {

        // Bytes needed before `range` for a skip instruction, or 0 when
        // neither form fits.  JR clears up to 0x7F bytes in two bytes; JP
        // clears anything in three.  A range that ends at 0xFFFF has no
        // address to jump to.
        static size_t guard_size(const address_range& range,
                                 uint16_t window_start)
        {
            const uint32_t start = range.start;
            const uint32_t end = range.end;
            if (end + 1u > 0xFFFFu)
                return 0;

            const uint32_t size = end - start + 1u;
            if (size <= 0x7Fu && start >= static_cast<uint32_t>(window_start) + 2u)
                return 2;
            if (start >= static_cast<uint32_t>(window_start) + 3u)
                return 3;
            return 0;
        }

    } // namespace

    std::vector<address_range> merge_reserved_ranges(
        const std::vector<address_range>& ranges)
    {
        std::vector<address_range> sorted = ranges;
        std::sort(sorted.begin(), sorted.end(),
                  [](const address_range& a, const address_range& b) {
                      if (a.start != b.start)
                          return a.start < b.start;
                      return a.end < b.end;
                  });

        std::vector<address_range> merged;
        for (const auto& range : sorted) {
            if (range.start > range.end)
                continue;
            if (!merged.empty()
                && static_cast<uint32_t>(range.start)
                       <= static_cast<uint32_t>(merged.back().end) + 1u) {
                merged.back().end = std::max(merged.back().end, range.end);
                continue;
            }
            merged.push_back(range);
        }

        return merged;
    }

    std::vector<address_range> clipped_reserved_ranges(
        const std::vector<address_range>& ranges,
        uint16_t window_start,
        uint16_t window_end)
    {
        std::vector<address_range> clipped;
        for (const auto& range : merge_reserved_ranges(ranges)) {
            const uint16_t start = std::max(range.start, window_start);
            const uint16_t end = std::min(range.end, window_end);
            if (start > end)
                continue;
            clipped.push_back({start, end});
        }

        // Absorb any gap that cannot hold the following range's guard.
        // Fusing grows a range, which can turn its JR into a JP and open a
        // new conflict, so repeat until nothing changes.
        bool fused = true;
        while (fused) {
            fused = false;
            for (size_t i = 1; i < clipped.size(); ++i) {
                const size_t needed = guard_size(clipped[i], window_start);
                if (needed == 0)
                    continue;
                if (static_cast<uint32_t>(clipped[i].start) - needed
                        > static_cast<uint32_t>(clipped[i - 1].end)) {
                    continue;
                }
                clipped[i - 1].end =
                    std::max(clipped[i - 1].end, clipped[i].end);
                clipped.erase(clipped.begin() + i);
                fused = true;
                break;
            }
        }

        return clipped;
    }

    std::vector<hole_guard> reserved_range_guards(
        const std::vector<address_range>& ranges,
        uint16_t window_start,
        uint16_t window_end)
    {
        std::vector<hole_guard> guards;
        for (const auto& range :
                 clipped_reserved_ranges(ranges, window_start, window_end)) {
            const size_t needed = guard_size(range, window_start);
            if (needed == 0)
                continue;

            hole_guard guard;
            guard.address =
                static_cast<uint16_t>(range.start - needed);
            if (needed == 2) {
                const uint32_t size =
                    static_cast<uint32_t>(range.end) - range.start + 1u;
                guard.bytes = {0x18, static_cast<uint8_t>(size)};
            } else {
                const uint16_t target = static_cast<uint16_t>(range.end + 1u);
                guard.bytes = {
                    0xC3,
                    static_cast<uint8_t>(target & 0xFF),
                    static_cast<uint8_t>((target >> 8) & 0xFF)
                };
            }
            guards.push_back(guard);
        }

        return guards;
    }

} // namespace xld
