// branch_relaxer.cpp
//
// Link-time JP->JR relaxation for local in-area branches.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <xld/area_placer.h>
#include <xld/branch_relaxer.h>

namespace xld {
namespace {

struct branch_candidate {
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

struct area_state {
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

static uint16_t remap_offset(const area_state &state,
                             const std::vector<branch_candidate> &candidates,
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

static void gather_candidates(link_context &ctx,
                              std::vector<branch_candidate> &candidates,
                              std::vector<area_state> &areas) {
    std::unordered_map<area*, int> area_map;

    for (auto &mod_ptr : ctx.modules) {
        auto *mod = mod_ptr.get();
        for (auto &a : mod->areas()) {
            if (a.is_abs() || a.is_ovr())
                continue;
            area_state state;
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

                branch_candidate cand;
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

static void apply_area_sizes(const std::vector<area_state> &areas,
                             const std::vector<bool> &active) {
    for (auto &state : areas) {
        uint16_t shrink = 0;
        for (int idx : state.candidates)
            if (active[idx])
                ++shrink;
        state.mod->area_by_index(state.area_index)
            .set_size(static_cast<uint16_t>(state.original_size - shrink));
    }
}

static bool candidate_fits(const branch_candidate &cand,
                           const std::vector<branch_candidate> &candidates,
                           const std::vector<area_state> &areas,
                           const std::vector<bool> &active) {
    const area_state *state = nullptr;
    for (auto &a : areas) {
        if (a.mod == cand.mod && a.area_index == cand.area_index) {
            state = &a;
            break;
        }
    }
    if (!state)
        return false;

    auto &area = cand.mod->area_by_index(cand.area_index);
    if (!area.placed_addr().has_value())
        return false;

    const uint16_t base = area.placed_addr().value();
    const uint16_t opcode_new =
        static_cast<uint16_t>(base + remap_offset(*state, candidates, active,
                                                  cand.opcode_offset));
    const uint16_t pc_after = static_cast<uint16_t>(opcode_new + 2);
    const uint16_t target_new =
        static_cast<uint16_t>(base + remap_offset(*state, candidates, active,
                                                  cand.target_offset));
    const int disp =
        static_cast<int>(target_new) - static_cast<int>(pc_after);
    return disp >= -128 && disp <= 127;
}

static void remap_area_contents(const area_state &state,
                                const std::vector<branch_candidate> &candidates,
                                const std::vector<bool> &active) {
    auto &mod = *state.mod;

    for (auto &sym : mod.symbols()) {
        if (!sym.is_def() || sym.area_index() != state.area_index)
            continue;
        sym.set_value(remap_offset(state, candidates, active, sym.value()));
    }

    for (auto &tr : mod.texts()) {
        if (tr.area_index != state.area_index)
            continue;
        tr.offset = remap_offset(state, candidates, active, tr.offset);
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

} // namespace

void branch_relaxer::relax(link_context &ctx) {
    std::vector<branch_candidate> candidates;
    std::vector<area_state> areas;
    gather_candidates(ctx, candidates, areas);
    if (candidates.empty())
        return;

    std::vector<bool> active(candidates.size(), false);
    std::vector<bool> next(candidates.size(), false);

    for (int iter = 0; iter < 16; ++iter) {
        apply_area_sizes(areas, active);
        area_placer::place(ctx);

        bool changed = false;
        for (size_t i = 0; i < candidates.size(); ++i) {
            next[i] = candidate_fits(candidates[i], candidates, areas, active);
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
        apply_area_sizes(areas, active);
        return;
    }

    apply_area_sizes(areas, active);
    for (auto &state : areas)
        remap_area_contents(state, candidates, active);
}

} // namespace xld
