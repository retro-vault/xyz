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

#include <xlink/area_placer.hpp>
#include <xlink/errors.hpp>

namespace xlink {

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

                cursor = next_free_address(cursor, max_size, ctx.holes);

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

                    cursor = next_free_address(cursor, a.size(), ctx.holes);
                    a.set_placed_addr(cursor);
                    cursor += a.size();
                }
            }
        }

        ctx.code_size = cursor;

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

} // namespace xlink
