// branch_relaxer.cpp
//
// Link-time branch canonicalization:
// - shrink local in-area JP to JR when safe
// - promote out-of-range short branches back to long forms when final
//   placement (including reserved holes) makes them overflow
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include <xld/area_placer.h>
#include <xld/branch_relaxer.h>

namespace xld {
namespace {

struct shrink_candidate {
    module* mod = nullptr;
    int area_index = -1;
    int text_index = -1;
    int reloc_index = -1;
    int symbol_index = -1;
    uint16_t opcode_offset = 0;
    uint16_t patch_offset = 0;
    uint16_t remove_offset = 0;
    uint16_t target_offset = 0;
    uint16_t patch_in_t = 0;
    uint8_t jr_opcode = 0;
};

struct shrink_area_state {
    module* mod = nullptr;
    int area_index = -1;
    uint16_t original_size = 0;
    std::vector<int> candidates;
};

struct grow_candidate {
    module* mod = nullptr;
    int area_index = -1;
    int text_index = -1;
    int reloc_index = -1;
    int ref_index = -1;
    bool sym_relative = false;
    bool djnz = false;
    uint16_t opcode_offset = 0;
    uint16_t insert_offset = 0;
    uint16_t patch_in_t = 0;
    uint16_t insert_in_t = 0;
    uint16_t delta_bytes = 0;
    uint8_t original_mode = 0;
    uint8_t original_opcode = 0;
    uint8_t addend = 0;
    uint8_t long_opcode = 0;
};

struct grow_area_state {
    module* mod = nullptr;
    int area_index = -1;
    uint16_t original_size = 0;
    std::vector<int> candidates;
};

static bool is_relaxable_jp(uint8_t opcode, uint8_t &jr_opcode) {
    switch (opcode) {
    case 0xC3: jr_opcode = 0x18; return true; // jp -> jr
    case 0xC2: jr_opcode = 0x20; return true; // jp nz -> jr nz
    case 0xCA: jr_opcode = 0x28; return true; // jp z  -> jr z
    case 0xD2: jr_opcode = 0x30; return true; // jp nc -> jr nc
    case 0xDA: jr_opcode = 0x38; return true; // jp c  -> jr c
    default:
        return false;
    }
}

static bool is_promotable_short_branch(uint8_t opcode,
                                       uint8_t &jp_opcode,
                                       bool &djnz,
                                       uint16_t &delta_bytes) {
    djnz = false;
    switch (opcode) {
    case 0x18: jp_opcode = 0xC3; delta_bytes = 1; return true; // jr -> jp
    case 0x20: jp_opcode = 0xC2; delta_bytes = 1; return true; // jr nz -> jp nz
    case 0x28: jp_opcode = 0xCA; delta_bytes = 1; return true; // jr z  -> jp z
    case 0x30: jp_opcode = 0xD2; delta_bytes = 1; return true; // jr nc -> jp nc
    case 0x38: jp_opcode = 0xDA; delta_bytes = 1; return true; // jr c  -> jp c
    case 0x10:
        djnz = true;
        jp_opcode = 0xC3;
        delta_bytes = 5; // djnz + jr + jp = 7 bytes total instead of 2
        return true;
    default:
        return false;
    }
}

static const shrink_area_state* find_shrink_area_state(
    const std::vector<shrink_area_state> &areas,
    const module* mod,
    int area_index) {
    for (const auto &state : areas) {
        if (state.mod == mod && state.area_index == area_index)
            return &state;
    }
    return nullptr;
}

static const grow_area_state* find_grow_area_state(
    const std::vector<grow_area_state> &areas,
    const module* mod,
    int area_index) {
    for (const auto &state : areas) {
        if (state.mod == mod && state.area_index == area_index)
            return &state;
    }
    return nullptr;
}

static uint16_t remap_offset_shrink(const shrink_area_state &state,
                                    const std::vector<shrink_candidate> &candidates,
                                    const std::vector<bool> &active,
                                    uint16_t original_offset) {
    uint16_t shrinks = 0;
    for (int idx : state.candidates) {
        if (!active[idx])
            continue;
        if (candidates[idx].remove_offset < original_offset)
            ++shrinks;
    }
    return static_cast<uint16_t>(original_offset - shrinks);
}

static uint16_t remap_offset_grow(const grow_area_state &state,
                                  const std::vector<grow_candidate> &candidates,
                                  const std::vector<bool> &active,
                                  uint16_t original_offset) {
    uint16_t growth = 0;
    for (int idx : state.candidates) {
        if (!active[idx])
            continue;
        if (candidates[idx].insert_offset <= original_offset)
            growth = static_cast<uint16_t>(growth + candidates[idx].delta_bytes);
    }
    return static_cast<uint16_t>(original_offset + growth);
}

static void gather_shrink_candidates(link_context &ctx,
                                     std::vector<shrink_candidate> &candidates,
                                     std::vector<shrink_area_state> &areas) {
    std::unordered_map<area*, int> area_map;

    for (auto &mod_ptr : ctx.modules) {
        auto *mod = mod_ptr.get();
        for (auto &a : mod->areas()) {
            if (a.is_abs() || a.is_ovr())
                continue;
            shrink_area_state state;
            state.mod = mod;
            state.area_index = a.index();
            state.original_size = a.size();
            area_map[&a] = static_cast<int>(areas.size());
            areas.push_back(state);
        }
    }

    for (auto &mod_ptr : ctx.modules) {
        auto *mod = mod_ptr.get();
        for (size_t ti = 0; ti < mod->texts().size(); ++ti) {
            auto &tr = mod->texts()[ti];
            if (tr.area_index < 0 ||
                tr.area_index >= static_cast<int>(mod->areas().size())) {
                continue;
            }

            auto &area = mod->area_by_index(tr.area_index);
            auto ait = area_map.find(&area);
            if (ait == area_map.end())
                continue;

            for (size_t ri = 0; ri < tr.relocs.size(); ++ri) {
                auto &re = tr.relocs[ri];
                if (!has_flag(re.mode, reloc_mode::word) ||
                    has_flag(re.mode, reloc_mode::pc_rel) ||
                    !has_flag(re.mode, reloc_mode::sym)) {
                    continue;
                }
                if (re.offset_in_t == 0 || re.offset_in_t + 1 >= tr.data.size())
                    continue;

                auto &sym = mod->symbol_by_index(re.ref_index);
                if (!sym.is_def() || sym.area_index() != tr.area_index)
                    continue;

                const uint8_t opcode = tr.data[re.offset_in_t - 1];
                uint8_t jr_opcode = 0;
                if (!is_relaxable_jp(opcode, jr_opcode))
                    continue;

                const uint16_t addend = static_cast<uint16_t>(
                    tr.data[re.offset_in_t] |
                    (static_cast<uint16_t>(tr.data[re.offset_in_t + 1]) << 8));
                if (addend != 0)
                    continue;

                shrink_candidate cand;
                cand.mod = mod;
                cand.area_index = tr.area_index;
                cand.text_index = static_cast<int>(ti);
                cand.reloc_index = static_cast<int>(ri);
                cand.symbol_index = re.ref_index;
                cand.opcode_offset =
                    static_cast<uint16_t>(tr.offset + re.offset_in_t - 1);
                cand.patch_offset =
                    static_cast<uint16_t>(tr.offset + re.offset_in_t);
                cand.remove_offset =
                    static_cast<uint16_t>(tr.offset + re.offset_in_t + 1);
                cand.target_offset = sym.value();
                cand.patch_in_t = re.offset_in_t;
                cand.jr_opcode = jr_opcode;
                areas[ait->second].candidates.push_back(
                    static_cast<int>(candidates.size()));
                candidates.push_back(cand);
            }
        }
    }
}

static void gather_growth_candidates(link_context &ctx,
                                     std::vector<grow_candidate> &candidates,
                                     std::vector<grow_area_state> &areas) {
    std::unordered_map<area*, int> area_map;

    for (auto &mod_ptr : ctx.modules) {
        auto *mod = mod_ptr.get();
        for (auto &a : mod->areas()) {
            grow_area_state state;
            state.mod = mod;
            state.area_index = a.index();
            state.original_size = a.size();
            area_map[&a] = static_cast<int>(areas.size());
            areas.push_back(state);
        }
    }

    for (auto &mod_ptr : ctx.modules) {
        auto *mod = mod_ptr.get();
        for (size_t ti = 0; ti < mod->texts().size(); ++ti) {
            auto &tr = mod->texts()[ti];
            if (tr.area_index < 0 ||
                tr.area_index >= static_cast<int>(mod->areas().size())) {
                continue;
            }

            auto &area = mod->area_by_index(tr.area_index);
            auto ait = area_map.find(&area);
            if (ait == area_map.end())
                continue;

            for (size_t ri = 0; ri < tr.relocs.size(); ++ri) {
                auto &re = tr.relocs[ri];
                if (!has_flag(re.mode, reloc_mode::pc_rel) ||
                    has_flag(re.mode, reloc_mode::word) ||
                    has_flag(re.mode, reloc_mode::msb)) {
                    continue;
                }
                if (re.offset_in_t == 0 || re.offset_in_t >= tr.data.size())
                    continue;

                bool djnz = false;
                uint8_t long_opcode = 0;
                uint16_t delta_bytes = 0;
                const uint8_t opcode = tr.data[re.offset_in_t - 1];
                if (!is_promotable_short_branch(opcode,
                                                long_opcode,
                                                djnz,
                                                delta_bytes)) {
                    continue;
                }

                grow_candidate cand;
                cand.mod = mod;
                cand.area_index = tr.area_index;
                cand.text_index = static_cast<int>(ti);
                cand.reloc_index = static_cast<int>(ri);
                cand.ref_index = re.ref_index;
                cand.sym_relative = has_flag(re.mode, reloc_mode::sym);
                cand.djnz = djnz;
                cand.opcode_offset =
                    static_cast<uint16_t>(tr.offset + re.offset_in_t - 1);
                cand.insert_offset =
                    static_cast<uint16_t>(tr.offset + re.offset_in_t + 1);
                cand.patch_in_t = re.offset_in_t;
                cand.insert_in_t = static_cast<uint16_t>(re.offset_in_t + 1);
                cand.delta_bytes = delta_bytes;
                cand.original_mode = static_cast<uint8_t>(re.mode);
                cand.original_opcode = opcode;
                cand.addend = tr.data[re.offset_in_t];
                cand.long_opcode = long_opcode;
                areas[ait->second].candidates.push_back(
                    static_cast<int>(candidates.size()));
                candidates.push_back(cand);
            }
        }
    }
}

static void apply_shrink_area_sizes(const std::vector<shrink_area_state> &areas,
                                    const std::vector<bool> &active) {
    for (const auto &state : areas) {
        uint16_t shrink = 0;
        for (int idx : state.candidates)
            if (active[idx])
                ++shrink;
        state.mod->area_by_index(state.area_index)
            .set_size(static_cast<uint16_t>(state.original_size - shrink));
    }
}

static void apply_grow_area_sizes(const std::vector<grow_area_state> &areas,
                                  const std::vector<grow_candidate> &candidates,
                                  const std::vector<bool> &active) {
    for (const auto &state : areas) {
        uint16_t growth = 0;
        for (int idx : state.candidates) {
            if (active[idx])
                growth = static_cast<uint16_t>(growth + candidates[idx].delta_bytes);
        }
        state.mod->area_by_index(state.area_index)
            .set_size(static_cast<uint16_t>(state.original_size + growth));
    }
}

static bool shrink_candidate_fits(const shrink_candidate &cand,
                                  const std::vector<shrink_candidate> &candidates,
                                  const std::vector<shrink_area_state> &areas,
                                  const std::vector<bool> &active) {
    const shrink_area_state *state =
        find_shrink_area_state(areas, cand.mod, cand.area_index);
    if (!state)
        return false;

    auto &area = cand.mod->area_by_index(cand.area_index);
    if (!area.placed_addr().has_value())
        return false;

    const uint16_t base = area.placed_addr().value();
    const uint16_t opcode_new =
        static_cast<uint16_t>(base + remap_offset_shrink(*state, candidates,
                                                         active,
                                                         cand.opcode_offset));
    const uint16_t pc_after = static_cast<uint16_t>(opcode_new + 2);
    const uint16_t target_new =
        static_cast<uint16_t>(base + remap_offset_shrink(*state, candidates,
                                                         active,
                                                         cand.target_offset));
    const int disp =
        static_cast<int>(target_new) - static_cast<int>(pc_after);
    return disp >= -128 && disp <= 127;
}

static std::optional<uint32_t> resolve_growth_target(
    const link_context &ctx,
    const grow_candidate &cand,
    const std::vector<grow_candidate> &candidates,
    const std::vector<grow_area_state> &areas,
    const std::vector<bool> &active) {
    if (cand.sym_relative) {
        const symbol *def_sym = nullptr;
        const module *def_mod = nullptr;

        auto &sym = cand.mod->symbol_by_index(cand.ref_index);
        if (sym.is_ref()) {
            auto git = ctx.global_symbols.find(sym.name());
            if (git != ctx.global_symbols.end()) {
                def_mod = git->second.first;
                def_sym = &def_mod->symbol_by_index(git->second.second);
            } else {
                auto lit = ctx.linker_symbols.find(sym.name());
                if (lit != ctx.linker_symbols.end())
                    return lit->second;
                return std::nullopt;
            }
        } else {
            def_mod = cand.mod;
            def_sym = &sym;
        }

        if (!def_sym || !def_mod)
            return std::nullopt;

        uint32_t target = def_sym->value();
        int def_area_idx = def_sym->area_index();
        if (def_area_idx >= 0 &&
            def_area_idx < static_cast<int>(def_mod->areas().size())) {
            const auto &def_area = def_mod->area_by_index(def_area_idx);
            if (!def_area.placed_addr().has_value())
                return std::nullopt;
            if (const auto *state =
                    find_grow_area_state(areas, def_mod, def_area_idx)) {
                target = remap_offset_grow(*state, candidates, active,
                                           static_cast<uint16_t>(target));
            }
            return target + def_area.placed_addr().value();
        }

        if (!def_mod->areas().empty()) {
            const auto &fallback_area = def_mod->areas()[0];
            if (!fallback_area.placed_addr().has_value())
                return std::nullopt;
            return target + fallback_area.placed_addr().value();
        }

        return target;
    }

    if (cand.ref_index < 0 ||
        cand.ref_index >= static_cast<int>(cand.mod->areas().size())) {
        return std::nullopt;
    }

    const auto &ref_area = cand.mod->area_by_index(cand.ref_index);
    if (!ref_area.placed_addr().has_value())
        return std::nullopt;
    return ref_area.placed_addr().value();
}

static bool grow_candidate_fits(const link_context &ctx,
                                const grow_candidate &cand,
                                const std::vector<grow_candidate> &candidates,
                                const std::vector<grow_area_state> &areas,
                                const std::vector<bool> &active) {
    const auto *state = find_grow_area_state(areas, cand.mod, cand.area_index);
    if (!state)
        return true;

    const auto &area = cand.mod->area_by_index(cand.area_index);
    if (!area.placed_addr().has_value())
        return true;

    const auto target = resolve_growth_target(ctx, cand, candidates, areas, active);
    if (!target.has_value())
        return true;

    const uint16_t source_offset =
        remap_offset_grow(*state, candidates, active, cand.opcode_offset);
    const uint32_t opcode_addr =
        area.placed_addr().value() + static_cast<uint32_t>(source_offset);
    const uint32_t pc_after = opcode_addr + 2u;
    const int addend = static_cast<int>(static_cast<int8_t>(cand.addend));
    const int disp = static_cast<int>(*target) + addend
                   - static_cast<int>(pc_after);
    return disp >= -128 && disp <= 127;
}

static void remap_area_contents_after_shrink(
    const shrink_area_state &state,
    const std::vector<shrink_candidate> &candidates,
    const std::vector<bool> &active) {
    auto &mod = *state.mod;

    for (auto &sym : mod.symbols()) {
        if (!sym.is_def() || sym.area_index() != state.area_index)
            continue;
        sym.set_value(remap_offset_shrink(state, candidates, active, sym.value()));
    }

    for (auto &tr : mod.texts()) {
        if (tr.area_index != state.area_index)
            continue;
        tr.offset = remap_offset_shrink(state, candidates, active, tr.offset);
    }

    std::unordered_map<int, std::vector<int>> by_text;
    for (int idx : state.candidates) {
        if (active[idx])
            by_text[candidates[idx].text_index].push_back(idx);
    }

    for (auto &[text_index, ids] : by_text) {
        auto &tr = mod.texts()[text_index];
        std::sort(ids.begin(), ids.end(), [&](int lhs, int rhs) {
            return candidates[lhs].patch_in_t > candidates[rhs].patch_in_t;
        });

        for (int idx : ids) {
            const auto &cand = candidates[idx];
            auto &re = tr.relocs[cand.reloc_index];
            tr.data[cand.patch_in_t - 1] = cand.jr_opcode;
            tr.data.erase(tr.data.begin() + cand.patch_in_t + 1);
            re.mode = reloc_mode::pc_rel | reloc_mode::sym;

            for (auto &other : tr.relocs) {
                if (other.offset_in_t > cand.patch_in_t + 1)
                    --other.offset_in_t;
            }
        }
    }
}

static void remap_area_contents_after_growth(
    const grow_area_state &state,
    const std::vector<grow_candidate> &candidates,
    const std::vector<bool> &active) {
    auto &mod = *state.mod;

    for (auto &sym : mod.symbols()) {
        if (!sym.is_def() || sym.area_index() != state.area_index)
            continue;
        sym.set_value(remap_offset_grow(state, candidates, active, sym.value()));
    }

    for (auto &tr : mod.texts()) {
        if (tr.area_index != state.area_index)
            continue;
        tr.offset = remap_offset_grow(state, candidates, active, tr.offset);
    }

    std::unordered_map<int, std::vector<int>> by_text;
    for (int idx : state.candidates) {
        if (active[idx])
            by_text[candidates[idx].text_index].push_back(idx);
    }

    for (auto &[text_index, ids] : by_text) {
        auto &tr = mod.texts()[text_index];
        std::sort(ids.begin(), ids.end(), [&](int lhs, int rhs) {
            if (candidates[lhs].insert_in_t != candidates[rhs].insert_in_t)
                return candidates[lhs].insert_in_t > candidates[rhs].insert_in_t;
            return lhs > rhs;
        });

        for (int idx : ids) {
            const auto &cand = candidates[idx];
            auto &re = tr.relocs[cand.reloc_index];

            if (cand.djnz) {
                const size_t opcode_pos = cand.patch_in_t - 1;
                const std::vector<uint8_t> replacement = {
                    0x10, 0x02,       // djnz +2 -> jp below
                    0x18, 0x03,       // jr +3   -> skip jp when B reached zero
                    0xC3, cand.addend, 0x00
                };
                tr.data.erase(tr.data.begin() + opcode_pos,
                              tr.data.begin() + opcode_pos + 2);
                tr.data.insert(tr.data.begin() + opcode_pos,
                               replacement.begin(),
                               replacement.end());
                re.offset_in_t = static_cast<uint16_t>(cand.patch_in_t + 4);
            } else {
                tr.data[cand.patch_in_t - 1] = cand.long_opcode;
                tr.data.insert(tr.data.begin() + cand.insert_in_t, 0x00);
                re.offset_in_t = cand.patch_in_t;
            }

            re.mode = static_cast<reloc_mode>(
                static_cast<uint8_t>(reloc_mode::word) |
                (cand.sym_relative
                    ? static_cast<uint8_t>(reloc_mode::sym)
                    : static_cast<uint8_t>(reloc_mode::none)));

            for (auto &other : tr.relocs) {
                if (&other == &re)
                    continue;
                if (other.offset_in_t >= cand.insert_in_t) {
                    other.offset_in_t = static_cast<uint16_t>(
                        other.offset_in_t + cand.delta_bytes);
                }
            }
        }
    }
}

static void shrink_local_jps(link_context &ctx) {
    std::vector<shrink_candidate> candidates;
    std::vector<shrink_area_state> areas;
    gather_shrink_candidates(ctx, candidates, areas);
    if (candidates.empty())
        return;

    std::vector<bool> active(candidates.size(), false);
    std::vector<bool> next(candidates.size(), false);

    for (int iter = 0; iter < 16; ++iter) {
        apply_shrink_area_sizes(areas, active);
        area_placer::place(ctx);

        bool changed = false;
        for (size_t i = 0; i < candidates.size(); ++i) {
            next[i] = shrink_candidate_fits(candidates[i], candidates, areas, active);
            if (next[i] != active[i])
                changed = true;
        }
        active.swap(next);
        if (!changed)
            break;
    }

    bool any = false;
    for (bool v : active)
        any = any || v;
    if (!any) {
        apply_shrink_area_sizes(areas, active);
        return;
    }

    apply_shrink_area_sizes(areas, active);
    for (const auto &state : areas)
        remap_area_contents_after_shrink(state, candidates, active);
}

static void promote_out_of_range_short_branches(link_context &ctx) {
    std::vector<grow_candidate> candidates;
    std::vector<grow_area_state> areas;
    gather_growth_candidates(ctx, candidates, areas);
    if (candidates.empty())
        return;

    std::vector<bool> active(candidates.size(), false);
    std::vector<bool> next(candidates.size(), false);

    for (int iter = 0; iter < 32; ++iter) {
        apply_grow_area_sizes(areas, candidates, active);
        area_placer::place(ctx);

        bool changed = false;
        for (size_t i = 0; i < candidates.size(); ++i) {
            next[i] = !grow_candidate_fits(ctx, candidates[i], candidates,
                                           areas, active);
            if (next[i] != active[i])
                changed = true;
        }
        active.swap(next);
        if (!changed)
            break;
    }

    bool any = false;
    for (bool v : active)
        any = any || v;
    if (!any) {
        apply_grow_area_sizes(areas, candidates, active);
        return;
    }

    apply_grow_area_sizes(areas, candidates, active);
    for (const auto &state : areas)
        remap_area_contents_after_growth(state, candidates, active);
}

} // namespace

void branch_relaxer::relax(link_context &ctx) {
    shrink_local_jps(ctx);
    promote_out_of_range_short_branches(ctx);
}

} // namespace xld
