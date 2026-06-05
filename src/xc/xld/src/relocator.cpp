// relocator.cpp
//
// relocation processor
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <cstring>
#include <iostream>

#include <xld/relocator.h>
#include <xld/errors.h>

namespace xld {

    void relocator::relocate(link_context& ctx) {
        // Allocate code buffer.
        ctx.code_buffer.resize(ctx.code_size, 0x00);

        for (auto& mod : ctx.modules) {
            for (auto& tr : mod->texts()) {
                // Get the area this T record belongs to.
                if (tr.area_index < 0 ||
                    tr.area_index >= static_cast<int>(mod->areas().size()))
                    throw reloc_error("T record references invalid area index "
                        + std::to_string(tr.area_index)
                        + " in module " + mod->name());

                auto& area = mod->area_by_index(tr.area_index);
                if (!area.placed_addr().has_value())
                    throw reloc_error("area '" + area.name()
                        + "' not placed in module " + mod->name());

                uint16_t base = area.placed_addr().value();
                uint16_t dest = base + tr.offset;

                // Copy T data into code buffer.
                if (dest + tr.data.size() > ctx.code_buffer.size()) {
                    // Grow buffer if needed.
                    ctx.code_buffer.resize(dest + tr.data.size(), 0x00);
                    ctx.code_size = static_cast<uint16_t>(
                        ctx.code_buffer.size());
                }
                std::memcpy(&ctx.code_buffer[dest],
                            tr.data.data(), tr.data.size());

                // Process relocations.
                for (auto& re : tr.relocs) {
                    // Determine the target address.
                    uint16_t target = 0;

                    if (has_flag(re.mode, reloc_mode::sym)) {
                        // Symbol reference.
                        if (re.ref_index < 0 || re.ref_index >=
                            static_cast<int>(mod->symbols().size()))
                            throw reloc_error(
                                "R entry references invalid symbol index");

                        auto& sym = mod->symbol_by_index(re.ref_index);
                        if (sym.is_ref()) {
                            // Look up in global table.
                            auto it = ctx.global_symbols.find(sym.name());
                            if (it == ctx.global_symbols.end()) {
                                auto lit = ctx.linker_symbols.find(sym.name());
                                if (lit == ctx.linker_symbols.end())
                                    throw reloc_error(
                                        "unresolved symbol: " + sym.name());
                                target = lit->second;
                            } else {
                                auto [def_mod, def_idx] = it->second;
                                auto& def_sym =
                                    def_mod->symbol_by_index(def_idx);
                                // Symbol value is relative to its area.
                                // We need to find which area it belongs to
                                // and add the placed address.
                                // In SDCC, symbol values are area-relative.
                                // The area is typically determined by context.
                                // For simplicity, use area 0's placed address
                                // if the symbol value looks like an offset,
                                // or just use the value directly if it's
                                // already absolute.
                                target = def_sym.value();
                                int def_area_idx = def_sym.area_index();
                                if (def_area_idx >= 0
                                    && def_area_idx < static_cast<int>(
                                        def_mod->areas().size())) {
                                    auto& def_area =
                                        def_mod->area_by_index(def_area_idx);
                                    if (!def_area.placed_addr().has_value())
                                        throw reloc_error("def area not placed");
                                    target += def_area.placed_addr().value();
                                } else if (!def_mod->areas().empty()) {
                                    auto& def_area = def_mod->areas()[0];
                                    if (def_area.placed_addr().has_value())
                                        target += def_area.placed_addr().value();
                                }
                            }
                        } else {
                            // Direct def in same module.
                            target = sym.value();
                            int def_area_idx = sym.area_index();
                            if (def_area_idx >= 0
                                && def_area_idx < static_cast<int>(
                                    mod->areas().size())) {
                                auto& def_area = mod->area_by_index(def_area_idx);
                                if (!def_area.placed_addr().has_value())
                                    throw reloc_error("def area not placed");
                                target += def_area.placed_addr().value();
                            } else if (!mod->areas().empty()
                                && mod->areas()[0].placed_addr().has_value()) {
                                target += mod->areas()[0].placed_addr().value();
                            }
                        }
                    } else {
                        // Area reference.
                        if (re.ref_index < 0 || re.ref_index >=
                            static_cast<int>(mod->areas().size()))
                            throw reloc_error(
                                "R entry references invalid area index");

                        auto& ref_area = mod->area_by_index(re.ref_index);
                        if (!ref_area.placed_addr().has_value())
                            throw reloc_error("referenced area '"
                                + ref_area.name() + "' not placed");
                        target = ref_area.placed_addr().value();
                    }

                    // Calculate the patch address in the code buffer.
                    uint16_t patch_offset = dest + re.offset_in_t;
                    bool is_word = has_flag(re.mode, reloc_mode::word);
                    bool is_pc_rel = has_flag(re.mode, reloc_mode::pc_rel);
                    bool is_msb = has_flag(re.mode, reloc_mode::msb);

                    if (patch_offset >= ctx.code_buffer.size() ||
                        (is_word && patch_offset + 1 >= ctx.code_buffer.size()))
                        throw reloc_error("relocation offset out of bounds");

                    // Read existing value from buffer.
                    uint16_t existing = 0;
                    if (is_word) {
                        existing = ctx.code_buffer[patch_offset]
                                 | (ctx.code_buffer[patch_offset + 1] << 8);
                    } else {
                        existing = ctx.code_buffer[patch_offset];
                    }

                    // Add target to existing value.
                    uint16_t value = existing + target;

                    if (is_pc_rel) {
                        // PC-relative: subtract the address after the
                        // relocated field.
                        uint16_t pc = patch_offset + (is_word ? 2 : 1);
                        value = existing + target - pc;
                    }

                    // Patch the buffer.
                    if (is_word) {
                        ctx.code_buffer[patch_offset] =
                            static_cast<uint8_t>(value & 0xFF);
                        ctx.code_buffer[patch_offset + 1] =
                            static_cast<uint8_t>((value >> 8) & 0xFF);
                    } else if (is_msb) {
                        ctx.code_buffer[patch_offset] =
                            static_cast<uint8_t>((value >> 8) & 0xFF);
                    } else {
                        ctx.code_buffer[patch_offset] =
                            static_cast<uint8_t>(value & 0xFF);
                    }

                    // Add to output relocation table for non-PC-relative
                    // absolute relocations.
                    if (!is_pc_rel) {
                        output_reloc or_entry;
                        or_entry.offset = patch_offset;
                        or_entry.size = is_word ? 2 : 1;
                        or_entry.pad = 0;
                        ctx.reloc_table.push_back(or_entry);
                    }
                }
            }
        }
    }

} // namespace xld
