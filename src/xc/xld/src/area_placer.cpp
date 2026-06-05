// area_placer.cpp
//
// area placement algorithm
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <xld/area_placer.h>
#include <xld/errors.h>

namespace xld {

    static bool ranges_overlap(uint16_t start_a, uint16_t end_a,
                               uint16_t start_b, uint16_t end_b)
    {
        return start_a <= end_b && end_a >= start_b;
    }

    static std::vector<address_range> effective_holes_for_placement(
        const link_context& ctx)
    {
        std::vector<address_range> holes = ctx.holes;
        if (ctx.format != output_format::bin)
            return holes;

        const uint16_t emit_start = ctx.output_range.has_value()
            ? ctx.output_range->start
            : 0x0000;
        const uint16_t emit_end = ctx.output_range.has_value()
            ? ctx.output_range->end
            : 0xFFFF;

        for (const auto& hole : ctx.holes) {
            uint16_t hs = std::max<uint16_t>(hole.start, emit_start);
            uint16_t he = std::min<uint16_t>(hole.end, emit_end);
            if (hs > he)
                continue;

            uint32_t hole_size = static_cast<uint32_t>(he) - hs + 1u;
            if (static_cast<uint32_t>(hs) >= static_cast<uint32_t>(emit_start) + 2u
                && hole_size <= 0x7Fu
                && static_cast<uint32_t>(he) + 1u <= 0xFFFFu) {
                holes.push_back({
                    static_cast<uint16_t>(hs - 2u),
                    static_cast<uint16_t>(hs - 1u)
                });
            } else if (static_cast<uint32_t>(hs)
                           >= static_cast<uint32_t>(emit_start) + 3u
                       && static_cast<uint32_t>(he) + 1u <= 0xFFFFu) {
                holes.push_back({
                    static_cast<uint16_t>(hs - 3u),
                    static_cast<uint16_t>(hs - 1u)
                });
            }
        }

        return holes;
    }

    uint16_t area_placer::next_free_address(
        uint16_t cursor, uint16_t size,
        const std::vector<address_range>& holes)
    {
        if (size == 0) return cursor;

        bool changed = true;
        while (changed) {
            changed = false;
            uint16_t end = cursor + size - 1;
            for (auto& hole : holes) {
                // Check if [cursor, end] overlaps [hole.start, hole.end].
                if (cursor <= hole.end && end >= hole.start) {
                    cursor = hole.end + 1;
                    changed = true;
                    break;
                }
            }
        }
        return cursor;
    }

    void area_placer::place(link_context& ctx) {
        const auto placement_holes = effective_holes_for_placement(ctx);

        // Group areas by name across all modules.
        // Maintain insertion order by first occurrence.
        struct area_group {
            std::string name;
            std::vector<std::pair<module*, int>> members; // module + area idx
        };

        std::vector<area_group> groups;
        std::map<std::string, size_t> group_map;

        for (auto& mod : ctx.modules) {
            for (auto& a : mod->areas()) {
                auto it = group_map.find(a.name());
                if (it == group_map.end()) {
                    group_map[a.name()] = groups.size();
                    area_group g;
                    g.name = a.name();
                    g.members.push_back({mod.get(), a.index()});
                    groups.push_back(g);
                } else {
                    groups[it->second].members.push_back(
                        {mod.get(), a.index()});
                }
            }
        }

        // Place areas group by group.
        uint16_t cursor = 0;

        for (auto& group : groups) {
            if (group.members.empty()) continue;

            auto base_it = ctx.area_bases.find(group.name);
            if (base_it != ctx.area_bases.end()) {
                if (base_it->second < cursor) {
                    throw placement_error(
                        "area base for '" + group.name
                        + "' overlaps previous placement");
                }
                cursor = base_it->second;
            }

            // Check the first member to determine area type.
            auto& first_area = group.members[0].first->area_by_index(
                group.members[0].second);

            if (first_area.is_abs()) {
                // ABS: place at org_addr if available.
                for (auto& [mod, idx] : group.members) {
                    auto& a = mod->area_by_index(idx);
                    if (a.org_addr().has_value()) {
                        a.set_placed_addr(a.org_addr().value());
                    } else {
                        throw placement_error(
                            "ABS area '" + a.name() + "' has no org address");
                    }
                }
            } else if (first_area.is_ovr()) {
                // OVR: all overlay at same address.
                // Find max size.
                uint16_t max_size = 0;
                for (auto& [mod, idx] : group.members) {
                    auto& a = mod->area_by_index(idx);
                    if (a.size() > max_size) max_size = a.size();
                }

                cursor = next_free_address(cursor, max_size, placement_holes);

                for (auto& [mod, idx] : group.members) {
                    auto& a = mod->area_by_index(idx);
                    a.set_placed_addr(cursor);
                }

                cursor += max_size;
            } else {
                // CON: concatenate sequentially.
                for (auto& [mod, idx] : group.members) {
                    auto& a = mod->area_by_index(idx);
                    if (a.size() == 0) {
                        a.set_placed_addr(cursor);
                        continue;
                    }

                    cursor = next_free_address(cursor, a.size(), placement_holes);
                    a.set_placed_addr(cursor);
                    cursor += a.size();
                }
            }
        }

        struct placed_area_ref {
            const area* area_ptr;
            const module* mod_ptr;
        };

        std::vector<placed_area_ref> placed_areas;
        uint32_t max_end = 0;

        for (const auto& mod : ctx.modules) {
            for (const auto& a : mod->areas()) {
                if (!a.placed_addr().has_value() || a.size() == 0)
                    continue;

                const uint16_t start = a.placed_addr().value();
                const uint16_t end = static_cast<uint16_t>(start + a.size() - 1);

                for (const auto& hole : placement_holes) {
                    if (ranges_overlap(start, end, hole.start, hole.end)) {
                        throw placement_error(
                            "area '" + a.name()
                            + "' overlaps reserved range");
                    }
                }

                for (const auto& placed : placed_areas) {
                    const auto& other = *placed.area_ptr;
                    const uint16_t other_start = other.placed_addr().value();
                    const uint16_t other_end = static_cast<uint16_t>(
                        other_start + other.size() - 1);

                    const bool same_overlay_group =
                        a.is_ovr() && other.is_ovr() && a.name() == other.name();
                    if (!same_overlay_group
                        && ranges_overlap(start, end, other_start, other_end)) {
                        throw placement_error(
                            "area '" + a.name() + "' overlaps area '"
                            + other.name() + "'");
                    }
                }

                placed_areas.push_back({&a, mod.get()});
                max_end = std::max<uint32_t>(max_end,
                    static_cast<uint32_t>(start) + a.size());
            }
        }

        ctx.code_size = static_cast<uint16_t>(max_end);

        // Print memory map if requested.
        if (ctx.print_map) {
            std::cout << "\nMemory map:\n";
            std::cout << "  Area              Addr   Size   Flags\n";
            std::cout << "  ----              ----   ----   -----\n";
            for (auto& mod : ctx.modules) {
                for (auto& a : mod->areas()) {
                    if (!a.placed_addr().has_value()) continue;
                    char buf[80];
                    std::snprintf(buf, sizeof(buf),
                        "  %-16s  %04X   %04X   %s%s",
                        a.name().c_str(),
                        a.placed_addr().value(),
                        a.size(),
                        a.is_abs() ? "ABS" : "REL",
                        a.is_ovr() ? " OVR" : " CON");
                    std::cout << buf << "\n";
                }
            }
            std::cout << "  Total code size: 0x"
                      << std::hex << ctx.code_size << std::dec << "\n\n";
        }
    }

} // namespace xld
