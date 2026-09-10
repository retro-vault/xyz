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
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <xld/area_placer.h>
#include <xld/errors.h>
#include <xld/holes.h>

namespace xld {

    namespace {

        static bool starts_with(std::string_view value, std::string_view prefix)
        {
            return value.substr(0, prefix.size()) == prefix;
        }

    } // namespace

    static std::optional<int> default_area_priority(const std::string& name)
    {
        // Keep common SDCC/GNU ROM/code sections ahead of RAM sections when
        // no linker-script ordering was provided. This prevents "first seen"
        // extraction order from placing _HEAP/_BSS before an explicitly based
        // _DATA area.
        static const std::map<std::string, int> priorities = {
            {"_HEADER", 0},
            {"_HOME", 10},
            {"_CODE", 20},
            {".text", 20},
            {"_CONST", 30},
            {".rodata", 30},
            {".vectors", 35},
            {"_INITIALIZER", 40},
            {"_GSINIT", 50},
            {"_GSFINAL", 60},
            {"_DATA", 70},
            {".data", 70},
            {"_INITIALIZED", 80},
            {"_BSS", 90},
            {".bss", 90},
            {"_HEAP", 100}
        };

        auto it = priorities.find(name);
        if (it != priorities.end())
            return it->second;

        if (starts_with(name, "_CODE_BANK_") || starts_with(name, ".text.bank."))
            return 20;
        if (starts_with(name, "_CONST_BANK_") || starts_with(name, ".rodata.bank."))
            return 30;
        if (starts_with(name, "_DATA_BANK_") || starts_with(name, ".data.bank."))
            return 70;

        return std::nullopt;
    }

    static bool ranges_overlap(uint32_t start_a, uint32_t end_a,
                               uint32_t start_b, uint32_t end_b)
    {
        return start_a <= end_b && end_a >= start_b;
    }

    // Every s__NAME / l__NAME this link actually consumes.
    static std::set<std::string> referenced_span_symbols(
        const link_context& ctx)
    {
        std::set<std::string> names;
        for (const auto& mod : ctx.modules) {
            for (const auto& sym : mod->symbols()) {
                if (!sym.is_ref())
                    continue;
                const auto& name = sym.name();
                if (name.rfind("s__", 0) == 0 || name.rfind("l__", 0) == 0)
                    names.insert(name);
            }
        }
        return names;
    }

    // The s__NAME / l__NAME pair describes one contiguous run, so a group
    // whose span symbols are consumed must not be split by a reserved range.
    // crt0 zeroes s__BSS..+l__BSS and copies s__INITIALIZER..+l__INITIALIZER;
    // a split group would make it walk straight through reserved memory.
    // Code groups stay splittable -- the emitted jump carries execution
    // across the range, which is the whole point of reserving one.
    static bool group_span_is_consumed(const std::set<std::string>& consumed,
                                       const std::string& group_name)
    {
        // SDCC startup always treats these as one byte span, whether or not
        // this particular link references the symbols.
        if (group_name == "_INITIALIZER" || group_name == "_INITIALIZED")
            return true;

        std::string suffix = group_name;
        if (!suffix.empty() && suffix[0] == '_')
            suffix.erase(0, 1);
        return consumed.count("s__" + suffix) != 0
            || consumed.count("l__" + suffix) != 0;
    }

    std::vector<address_range> area_placer::effective_holes_for_placement(
        const link_context& ctx)
    {
        // The declared ranges stay unclipped: a range reaching past the
        // emitted window still blocks placement there.
        std::vector<address_range> holes = merge_reserved_ranges(ctx.holes);
        if (ctx.format != output_format::bin
            && ctx.format != output_format::ihx)
            return holes;

        const uint16_t emit_start = ctx.output_range.has_value()
            ? ctx.output_range->start
            : 0x0000;
        const uint16_t emit_end = ctx.output_range.has_value()
            ? ctx.output_range->end
            : 0xFFFF;

        // Gaps too narrow for a guard were folded into the reserved space,
        // so keep them free as well.  Ranges the fusing left alone are
        // already covered by the declared list.
        for (const auto& range :
                 clipped_reserved_ranges(ctx.holes, emit_start, emit_end)) {
            const bool covered = std::any_of(holes.begin(), holes.end(),
                [&](const address_range& declared) {
                    return range.start >= declared.start
                        && range.end <= declared.end;
                });
            if (!covered)
                holes.push_back(range);
        }

        for (const auto& guard :
                 reserved_range_guards(ctx.holes, emit_start, emit_end)) {
            holes.push_back({
                guard.address,
                static_cast<uint16_t>(guard.address + guard.bytes.size() - 1u)
            });
        }

        return holes;
    }

    uint32_t area_placer::next_free_address(
        uint32_t cursor, uint32_t size,
        const std::vector<address_range>& holes)
    {
        constexpr uint32_t address_space_size = 0x10000u;
        if (size == 0) {
            if (cursor >= address_space_size)
                throw placement_error("area placement exceeds 64 KiB address space");
            return cursor;
        }

        bool changed = true;
        while (changed) {
            changed = false;
            if (cursor >= address_space_size
                || size > address_space_size - cursor) {
                throw placement_error(
                    "area placement exceeds 64 KiB address space");
            }
            const uint32_t end = cursor + size - 1u;
            for (auto& hole : holes) {
                // Check if [cursor, end] overlaps [hole.start, hole.end].
                if (cursor <= hole.end && end >= hole.start) {
                    cursor = static_cast<uint32_t>(hole.end) + 1u;
                    changed = true;
                    break;
                }
            }
        }
        return cursor;
    }

    void area_placer::place(link_context& ctx) {
        const auto placement_holes = effective_holes_for_placement(ctx);
        const auto consumed_spans = referenced_span_symbols(ctx);

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

        if (!ctx.area_order.empty()) {
            std::vector<area_group> ordered;
            std::vector<bool> used(groups.size(), false);

            for (const auto& scripted_name : ctx.area_order) {
                auto it = group_map.find(scripted_name);
                if (it == group_map.end())
                    continue;
                if (used[it->second])
                    continue;
                ordered.push_back(groups[it->second]);
                used[it->second] = true;
            }

            for (size_t i = 0; i < groups.size(); ++i) {
                if (!used[i])
                    ordered.push_back(groups[i]);
            }

            groups = std::move(ordered);
        } else {
            std::stable_sort(groups.begin(), groups.end(),
                             [](const area_group& a, const area_group& b) {
                auto pa = default_area_priority(a.name);
                auto pb = default_area_priority(b.name);
                if (pa.has_value() && pb.has_value())
                    return *pa < *pb;
                if (pa.has_value())
                    return true;
                if (pb.has_value())
                    return false;
                return false;
            });
        }

        // Place areas group by group.
        uint32_t cursor = 0;

        for (auto& group : groups) {
            if (group.members.empty()) continue;

            auto base_it = ctx.area_bases.find(group.name);
            const uint32_t resume_cursor = cursor;
            const bool has_explicit_base = base_it != ctx.area_bases.end();
            if (base_it != ctx.area_bases.end()) {
                cursor = static_cast<uint32_t>(base_it->second);
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
                    a.set_placed_addr(static_cast<uint16_t>(cursor));
                }

                cursor += max_size;
            } else {
                // CON: concatenate sequentially.
                // A group whose span symbols are consumed must land in one
                // piece: a reserved range between two members would leave
                // s__NAME/l__NAME covering the reserved bytes. Reserve the
                // whole run up front so the members simply follow it.
                if (group_span_is_consumed(consumed_spans, group.name)) {
                    uint32_t group_size = 0;
                    for (const auto& [mod, idx] : group.members)
                        group_size += mod->area_by_index(idx).size();
                    cursor = next_free_address(
                        cursor, group_size, placement_holes);
                }
                for (auto& [mod, idx] : group.members) {
                    auto& a = mod->area_by_index(idx);
                    if (a.size() == 0) {
                        if (cursor >= 0x10000u) {
                            throw placement_error(
                                "area '" + a.name()
                                + "' begins outside 64 KiB address space");
                        }
                        a.set_placed_addr(static_cast<uint16_t>(cursor));
                        continue;
                    }

                    cursor = next_free_address(cursor, a.size(), placement_holes);
                    a.set_placed_addr(static_cast<uint16_t>(cursor));
                    cursor += a.size();
                }
            }

            // An explicit base is an independent placement request. It may
            // describe a fixed vector below an area encountered earlier in
            // group order. Preserve the high-water mark for later unbased
            // areas; the complete overlap audit below still rejects actual
            // collisions.
            if (has_explicit_base)
                cursor = std::max(cursor, resume_cursor);
        }

        struct placed_area_ref {
            const area* area_ptr;
            const module* mod_ptr;
        };

        std::vector<placed_area_ref> placed_areas;
        uint32_t max_end = 0;
        uint32_t min_start = UINT32_MAX;

        for (const auto& mod : ctx.modules) {
            for (const auto& a : mod->areas()) {
                if (!a.placed_addr().has_value() || a.size() == 0)
                    continue;

                const uint32_t start = a.placed_addr().value();
                const uint32_t end = start + a.size() - 1u;
                if (end >= 0x10000u) {
                    throw placement_error(
                        "area '" + a.name()
                        + "' exceeds 64 KiB address space");
                }

                for (const auto& hole : placement_holes) {
                    if (ranges_overlap(start, end, hole.start, hole.end)) {
                        throw placement_error(
                            "area '" + a.name()
                            + "' overlaps reserved range");
                    }
                }

                for (const auto& placed : placed_areas) {
                    const auto& other = *placed.area_ptr;
                    const uint32_t other_start = other.placed_addr().value();
                    const uint32_t other_end =
                        other_start + other.size() - 1u;

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
                if (a.size() > 0)
                    min_start = std::min<uint32_t>(min_start, start);
            }
        }

        ctx.code_size = max_end;
        ctx.image_base = (min_start == UINT32_MAX) ? 0 : min_start;
        ctx.code_occupancy.clear();

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
