//
// z80gen_data.cpp — Module-level emission: globals, TLS, string literals.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_set>

#ifndef XCC_VERSION
#define XCC_VERSION "0.1.0"
#endif

namespace xcc {

namespace {

std::string banked_data_section_name(int bank) {
    return "_DATA_BANK_" + std::to_string(bank);
}

int global_object_size(const ir_module::global_var &g) {
    int sz = g.type ? g.type->size() : 2;
    return sz > 0 ? sz : 2;
}

// Qualification belongs to the stored object, not a pointer's pointee.
// Arrays inherit their elements' qualification, including nested arrays.
bool has_volatile_or_atomic_subobject(const type_ptr &ty) {
    if (!ty)
        return false;
    if (ty->is_volatile || ty->is_atomic)
        return true;
    if (ty->kind == type_kind::ARRAY)
        return has_volatile_or_atomic_subobject(ty->base);
    if (ty->kind == type_kind::STRUCT || ty->kind == type_kind::UNION) {
        for (const auto &field : ty->fields) {
            if (has_volatile_or_atomic_subobject(field.type))
                return true;
        }
    }
    return false;
}

bool is_readonly_global(const ir_module::global_var &g) {
    if (g.is_tls || g.bank >= 0 || g.at_address >= 0 || g.sfr_port >= 0 ||
        has_volatile_or_atomic_subobject(g.type)) {
        return false;
    }
    for (type_ptr ty = g.type; ty; ty = ty->base) {
        if (ty->is_const)
            return true;
        if (ty->kind != type_kind::ARRAY)
            break;
    }
    return false;
}

bool has_all_zero_initializer(const ir_module::global_var &g) {
    if (!g.init_vals.empty()) {
        for (const auto &e : g.init_vals) {
            if (!e.label.empty() || e.value != 0)
                return false;
        }
        return true;
    }
    return !g.has_init || g.init_val == 0;
}

std::string z88dk_hex_mask(uint32_t mask) {
    std::ostringstream out;
    out << '$' << std::uppercase << std::hex << std::setw(8)
        << std::setfill('0') << mask;
    return out.str();
}

void emit_z88dk_format_definition(asm_emitter &out, const char *stem,
                                  const ir_module::format_usage &usage,
                                  uint32_t full_mask) {
    if (!usage.used)
        return;

    const uint32_t mask = usage.requires_full ? full_mask : usage.mask;
    const std::string symbol = std::string("CRT_") + stem + "_format";
    const std::string guard = std::string("DEFINED_") + symbol;
    const std::string temporary = std::string("temp_") + stem + "_format";
    const std::string value = z88dk_hex_mask(mask);

    std::ostringstream text;
    if (mask != 0) {
        text << "\nIF !" << guard << "\n"
             << "\tdefc " << guard << " = 1\n"
             << "\tdefc " << symbol << " = " << value << "\n"
             << "ELSE\n"
             << "\tUNDEFINE " << temporary << "\n"
             << "\tdefc " << temporary << " = " << symbol << "\n"
             << "\tUNDEFINE " << symbol << "\n"
             << "\tdefc " << symbol << " = " << temporary
             << " | " << value << "\n"
             << "ENDIF\n\n";
    }
    text << "IF !NEED_" << stem << "\n"
         << "\tDEFINE NEED_" << stem << "\n"
         << "ENDIF\n\n";
    out.raw(text.str());
}

void emit_z88dk_printf_long_long_definition(
    asm_emitter &out, const ir_module::format_usage &usage) {
    const uint32_t mask = usage.requires_full ? 0x0000005Fu : usage.mask2;
    if (!usage.used || mask == 0)
        return;
    const std::string value = z88dk_hex_mask(mask);
    std::ostringstream text;
    text << "\nIF !DEFINED_CLIB_OPT_PRINTF_2\n"
         << "\tdefc DEFINED_CLIB_OPT_PRINTF_2 = 1\n"
         << "\tdefc CLIB_OPT_PRINTF_2 = " << value << "\n"
         << "ELSE\n"
         << "\tUNDEFINE temp_CLIB_OPT_PRINTF_2\n"
         << "\tdefc temp_CLIB_OPT_PRINTF_2 = CLIB_OPT_PRINTF_2\n"
         << "\tUNDEFINE CLIB_OPT_PRINTF_2\n"
         << "\tdefc CLIB_OPT_PRINTF_2 = temp_CLIB_OPT_PRINTF_2 | "
         << value << "\n"
         << "ENDIF\n\n";
    out.raw(text.str());
}

} // namespace

void z80_gen::plan_size_shared_ix_helpers(const ir_module &mod) {
    size_shared_ix_helpers_ = false;
    if (debug_ || opt_settings_.level != opt_level::Os)
        return;

    int shared_enter_count = 0;
    int shared_leave_count = 0;
    for (const auto &fn : mod.functions) {
        const call_abi abi = effective_call_abi(fn.abi);
        if (abi == call_abi::NAKED)
            continue;

        cur_fn_ = &fn;
        local_bytes_ = fn.local_bytes;
        cur_convention_ = &get_abi_convention(fn.abi);
        temp_slots_.clear();
        temp_regs_.clear();
        incoming_symbol_homes_.clear();
        symbol_regs_.clear();
        next_temp_slot_ = 0;
        temp_stack_bytes_ = 0;
        temp_frame_bytes_ = 0;

        if (regalloc_enabled()) {
            // Planning shared IX helpers performs the same prospective-frame
            // register allocation as real emission.  Suppress any late-slot
            // stack instructions here: this pass has no function body or
            // prologue into which such instructions could legally be emitted.
            reserving_prologue_spills_ = true;
            regalloc_prepass(fn);
            reserving_prologue_spills_ = false;
            temp_stack_bytes_ = 0;
            temp_frame_bytes_ = 0;
        }
        temp_stack_bytes_ = compute_temp_frame_bytes(fn);
        if (can_omit_frame_pointer(fn))
            continue;

        ++shared_enter_count;
        if (fn.is_noreturn || abi == call_abi::INTERRUPT ||
            abi == call_abi::CRITICAL) {
            continue;
        }

        const bool callee_repairs_stack =
            abi == call_abi::Z88DK_CALLEE ||
            (abi == call_abi::SDCCCALL1 && fn.callee_cleans_stack);
        if (!(callee_repairs_stack && fn.stack_param_bytes > 0))
            ++shared_leave_count;
    }

    cur_fn_ = nullptr;
    cur_convention_ = nullptr;
    temp_slots_.clear();
    temp_regs_.clear();
    incoming_symbol_homes_.clear();
    symbol_regs_.clear();
    temp_stack_bytes_ = 0;
    temp_frame_bytes_ = 0;

    // Enter saves 5 bytes per frame and leave saves 2.  The two runtime
    // helpers occupy 16 bytes in total, so use them only when the module's
    // direct site savings exceed their complete cost.  The previous
    // 24-byte gate double-counted unrelated late tail sharing and missed the
    // common three-framed-function break-even case.
    const int site_savings = shared_enter_count * 5 + shared_leave_count * 2;
    size_shared_ix_helpers_ = site_savings > 16;
}

void z80_gen::emit_module(const ir_module &mod) {
    asm_.module_header();
    asm_.default_calling_convention(get_default_call_abi());
    if (z88dk_classic_runtime_) {
        emit_z88dk_format_definition(asm_, "printf", mod.printf_formats,
                                     0xC01BF7BFu);
        emit_z88dk_printf_long_long_definition(asm_, mod.printf_formats);
        emit_z88dk_format_definition(asm_, "scanf", mod.scanf_formats,
                                     0x403FFFFFu);
    }
    iy_preserving_local_callees_.clear();
    internal_function_names_.clear();
    defined_function_names_.clear();
    for (const auto &fn : mod.functions) {
        defined_function_names_.insert(fn.name);
        if (!fn.is_global)
            internal_function_names_.insert(fn.name);
    }

    if (debug_) debug_->begin_module();

    // Assign TLS offsets: each _Thread_local global gets a slot in the per-thread block.
    tls_offsets_.clear();
    tls_size_ = 0;
    for (auto &g : mod.globals) {
        if (!g.is_tls) continue;
        tls_offsets_[mangle(g.name)] = tls_size_;
        int sz = g.type ? g.type->size() : 2;
        tls_size_ += (sz > 0) ? sz : 2;
    }

    emit_globals(mod);
    emit_strings(mod);
    emit_external_data_refs(mod);

    plan_size_shared_ix_helpers(mod);
    plan_size_mul16_helper_reuse(mod);

    for (auto &fn : mod.functions)
        emit_function(fn);

    if (debug_) debug_->end_module(std::string("xcc ") + XCC_VERSION);
}

void z80_gen::emit_global_body(const ir_module::global_var &g, bool zero_fill) {
    if (!g.init_vals.empty() && !zero_fill) {
        for (auto &e : g.init_vals) {
            if (!e.label.empty()) {
                std::string address = mangle(e.label);
                if (e.value > 0)
                    address += " + " + std::to_string(e.value);
                else if (e.value < 0)
                    address += " - " + std::to_string(e.value).substr(1);
                asm_.dw_sym(address);
            }
            else if (e.size == 1) asm_.db((int)e.value);
            else if (e.size == 2) asm_.dw((int)(e.value & 0xFFFF));
            else if (e.size == 4) {
                asm_.dw((int)(e.value & 0xFFFF));
                asm_.dw((int)((e.value >> 16) & 0xFFFF));
            } else if (e.size == 8) {
                for (int w = 0; w < 4; ++w)
                    asm_.dw((int)((e.value >> (w * 16)) & 0xFFFF));
            } else asm_.ds(e.size);
        }
    } else {
        int sz = global_object_size(g);
        if (!zero_fill && g.has_init && g.init_val != 0) {
            if (sz == 1)      asm_.db((int)g.init_val);
            else if (sz == 2) asm_.dw((int)g.init_val);
            else if (sz == 4) {
                asm_.dw((int)(g.init_val & 0xFFFF));
                asm_.dw((int)((g.init_val >> 16) & 0xFFFF));
            } else if (sz == 8) {
                for (int w = 0; w < 4; ++w)
                    asm_.dw((int)((g.init_val >> (w * 16)) & 0xFFFF));
            } else {
                asm_.ds(sz > 0 ? sz : 2);
            }
        } else {
            asm_.ds(sz);
        }
    }
}

void z80_gen::emit_globals(const ir_module &mod) {
    if (mod.globals.empty()) return;

    // Emit [[sdcc::at(N)]] variables as absolute symbol assignments first.
    // These do NOT live in any section; they are pure address aliases.
    for (auto &g : mod.globals) {
        if (g.at_address < 0 || g.sfr_port >= 0) continue;
        std::string lbl = mangle(g.name);
        if (!g.is_static) asm_.global_decl(lbl);
        asm_.symbol_assign(lbl, (long long)g.at_address);
    }

    // [[sdcc::sfr(N)]] variables: no data section entry; reads/writes use IN/OUT.
    // Emit an absolute symbol so the address is available if taken.
    for (auto &g : mod.globals) {
        if (g.sfr_port < 0) continue;
        std::string lbl = mangle(g.name);
        if (!g.is_static) asm_.global_decl(lbl);
        asm_.symbol_assign(lbl, (long long)g.sfr_port);
    }

    bool emitted_rodata = false;
    for (const auto &g : mod.globals) {
        if (!is_readonly_global(g))
            continue;
        if (!emitted_rodata)
            asm_.section_rodata();

        const std::string lbl = mangle(g.name);
        if (!g.is_static) asm_.global_decl(lbl);
        if (debug_) debug_->emit_global(g.name, g.type.get(), g.is_static);
        asm_.symbol_type_object(lbl);
        asm_.label(lbl, false);
        emit_global_body(g, false);
        asm_.symbol_size(lbl, std::to_string(global_object_size(g)));
        emitted_rodata = true;
    }
    if (emitted_rodata)
        asm_.raw("\n");

    int current_data_bank = -2;
    bool emitted_data = false;
    for (auto &g : mod.globals) {
        if (g.is_tls || is_readonly_global(g)) continue;
        if (g.at_address >= 0 || g.sfr_port >= 0) continue; // handled above
        if (g.bank < 0 && has_all_zero_initializer(g)) continue;

        if (g.bank != current_data_bank) {
            if (g.bank < 0) asm_.section_data();
            else asm_.section_data_named(banked_data_section_name(g.bank));
            current_data_bank = g.bank;
        }

        std::string lbl = mangle(g.name);
        if (!g.is_static) asm_.global_decl(lbl);
        if (debug_) debug_->emit_global(g.name, g.type.get(), g.is_static);
        asm_.symbol_type_object(lbl);
        asm_.label(lbl, false);
        emit_global_body(g, false);
        asm_.symbol_size(lbl, std::to_string(global_object_size(g)));
        emitted_data = true;
    }
    if (emitted_data) {
        asm_.raw("\n");
    }

    bool emitted_bss = false;
    for (const auto &g : mod.globals) {
        if (g.is_tls || g.bank >= 0 || is_readonly_global(g) ||
            g.at_address >= 0 || g.sfr_port >= 0 ||
            !has_all_zero_initializer(g)) {
            continue;
        }
        if (!emitted_bss)
            asm_.section_bss();

        const std::string lbl = mangle(g.name);
        if (!g.is_static) asm_.global_decl(lbl);
        if (debug_) debug_->emit_global(g.name, g.type.get(), g.is_static);
        asm_.symbol_type_object(lbl);
        asm_.label(lbl, false);
        asm_.ds(global_object_size(g));
        asm_.symbol_size(lbl, std::to_string(global_object_size(g)));
        emitted_bss = true;
    }
    if (emitted_bss)
        asm_.raw("\n");

    if (tls_size_ > 0) {
        asm_.section_tls();
        asm_.global_decl("__tls_template");
        asm_.symbol_type_object("__tls_template");
        asm_.label("__tls_template", false);
        for (auto &g : mod.globals) {
            if (!g.is_tls) continue;
            std::string lbl = mangle(g.name);
            asm_.comment("tls: " + lbl + " @ offset " + std::to_string(tls_offsets_[lbl]));
            // The template supplies each new thread's declared initial values.
            emit_global_body(g, false);
        }
        asm_.symbol_size("__tls_template", std::to_string(tls_size_));
        asm_.global_decl("__tls_size");
        asm_.symbol_assign("__tls_size", tls_size_);
        asm_.raw("\n");
    }
}

void z80_gen::emit_external_data_refs(const ir_module &mod) {
    std::unordered_set<std::string> defined;
    for (const auto &g : mod.globals)
        defined.insert(mangle(g.name));
    for (const auto &s : mod.string_literals)
        defined.insert(mangle(s.name));
    for (const auto &fn : mod.functions)
        defined.insert(mangle(fn.name));

    std::unordered_set<std::string> emitted;
    auto maybe_emit = [&](const operand &op) {
        if (op.kind != operand_kind::SYMBOL || !op.is_global || op.is_func)
            return;
        // C23 static compound literals currently lower through internal
        // pseudo-globals named __sclitN. They are module-local compiler
        // implementation details, not external data imports.
        if (op.name.rfind("__sclit", 0) == 0)
            return;
        const std::string sym = mangle(op.name);
        if (defined.count(sym) != 0 || emitted.count(sym) != 0)
            return;
        asm_.global_decl(sym);
        emitted.insert(sym);
    };

    for (const auto &fn : mod.functions) {
        for (const auto &ic : fn.icodes) {
            maybe_emit(ic.result);
            maybe_emit(ic.left);
            maybe_emit(ic.right);
        }
    }

    for (const auto &g : mod.globals) {
        for (const auto &elem : g.init_vals) {
            if (elem.label.empty())
                continue;
            const std::string sym = mangle(elem.label);
            if (defined.count(sym) != 0 || emitted.count(sym) != 0)
                continue;
            asm_.global_decl(sym);
            emitted.insert(sym);
        }
    }

    if (!emitted.empty())
        asm_.raw("\n");
}

void z80_gen::emit_strings(const ir_module &mod) {
    if (mod.string_literals.empty()) return;
    if (size_opt_enabled() && !debug_)
        asm_.section_code();
    else
        asm_.section_rodata();

    const auto &strings = mod.string_literals;
    std::vector<size_t> order(strings.size());
    std::iota(order.begin(), order.end(), size_t{0});
    if (tuned_profile_enabled() && !debug_) {
        // Reversed lexical order places each suffix after its extensions.
        // Thus a suffix can join the most recent root without a quadratic
        // search through all literals. Compare complete strings, including
        // embedded zeroes; C-string comparisons would merge unequal data.
        std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            const auto &lhs = strings[a];
            const auto &rhs = strings[b];
            if (lhs.char_width != rhs.char_width)
                return lhs.char_width < rhs.char_width;
            return std::lexicographical_compare(
                rhs.str_init.rbegin(), rhs.str_init.rend(),
                lhs.str_init.rbegin(), lhs.str_init.rend());
        });
    }

    std::vector<std::vector<size_t>> pools;
    for (size_t index : order) {
        const auto &suffix = strings[index];
        if (tuned_profile_enabled() && !debug_ && !pools.empty()) {
            const auto &root = strings[pools.back().front()];
            if (root.char_width == suffix.char_width &&
                root.str_init.size() >= suffix.str_init.size() &&
                root.str_init.compare(
                    root.str_init.size() - suffix.str_init.size(),
                    suffix.str_init.size(), suffix.str_init) == 0) {
                pools.back().push_back(index);
                continue;
            }
        }
        pools.push_back({index});
    }
    // Keep independent roots in source order. Only immutable anonymous
    // literals share storage; named arrays remain separately emitted globals.
    std::sort(pools.begin(), pools.end(), [](const auto &a, const auto &b) {
        return a.front() < b.front();
    });
    for (const auto &pool : pools) {
        const auto &root = strings[pool.front()];
        size_t position = 0;
        auto emit_until = [&](size_t end) {
            if (position == end)
                return;
            // The lexer uses 8 to distinguish the u8 prefix, not to request
            // eight-byte (or four-byte) code units. Its payload is UTF-8.
            if (root.char_width <= 1 || root.char_width == 8) {
                std::vector<int> bytes;
                bytes.reserve(end - position);
                for (; position < end; ++position)
                    bytes.push_back(position == root.str_init.size() ? 0 :
                        static_cast<unsigned char>(root.str_init[position]));
                asm_.db_list(bytes);
            } else {
                for (; position < end; ++position) {
                    const int value = position == root.str_init.size() ? 0 :
                        static_cast<unsigned char>(root.str_init[position]);
                    if (root.char_width == 2)
                        asm_.dw(value);
                    else
                        asm_.dl(value);
                }
            }
        };
        for (size_t index : pool) {
            const auto &suffix = strings[index];
            emit_until(root.str_init.size() - suffix.str_init.size());
            const std::string lbl = mangle(suffix.name);
            asm_.symbol_type_object(lbl);
            asm_.label(lbl, false);
        }
        emit_until(root.str_init.size() + 1);
        for (size_t index : pool) {
            const auto &s = strings[index];
            const int width = s.char_width == 2 ? 2 :
                              s.char_width == 4 ? 4 : 1;
            asm_.symbol_size(mangle(s.name),
                std::to_string((s.str_init.size() + 1) * width));
        }
    }
    asm_.raw("\n");
}

} // namespace xcc
