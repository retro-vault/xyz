//
// z80gen_data.cpp — Module-level emission: globals, TLS, string literals.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"
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

        if (regalloc_enabled())
            regalloc_prepass(fn);
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
    iy_preserving_local_callees_.clear();

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

    for (auto &fn : mod.functions)
        emit_function(fn);

    if (debug_) debug_->end_module(std::string("xcc ") + XCC_VERSION);
}

void z80_gen::emit_global_body(const ir_module::global_var &g, bool tls_template) {
    if (!g.init_vals.empty() && !tls_template) {
        for (auto &e : g.init_vals) {
            if (!e.label.empty()) asm_.dw_sym(mangle(e.label));
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
        if (!tls_template && g.has_init && g.init_val != 0) {
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

    int current_data_bank = -2;
    bool emitted_data = false;
    for (auto &g : mod.globals) {
        if (g.is_tls) continue;
        if (g.at_address >= 0 || g.sfr_port >= 0) continue; // handled above

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

    if (tls_size_ > 0) {
        asm_.section_tls();
        asm_.global_decl("__tls_template");
        asm_.symbol_type_object("__tls_template");
        asm_.label("__tls_template", false);
        for (auto &g : mod.globals) {
            if (!g.is_tls) continue;
            std::string lbl = mangle(g.name);
            asm_.comment("tls: " + lbl + " @ offset " + std::to_string(tls_offsets_[lbl]));
            emit_global_body(g, true);
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

    if (!emitted.empty())
        asm_.raw("\n");
}

void z80_gen::emit_strings(const ir_module &mod) {
    if (mod.string_literals.empty()) return;
    if (size_opt_enabled() && !debug_)
        asm_.section_code();
    else
        asm_.section_rodata();
    for (auto &s : mod.string_literals) {
        const std::string lbl = mangle(s.name);
        asm_.symbol_type_object(lbl);
        asm_.label(lbl, false);
        if (s.char_width <= 1) {
            std::vector<int> bytes;
            for (unsigned char c : s.str_init) bytes.push_back((int)c);
            bytes.push_back(0);
            asm_.db_list(bytes);
        } else if (s.char_width == 2) {
            for (unsigned char c : s.str_init) asm_.dw((int)c);
            asm_.dw(0);
        } else {
            for (unsigned char c : s.str_init) asm_.dl((int)c);
            asm_.dl(0);
        }
        asm_.symbol_size(lbl, std::to_string(global_object_size(s)));
    }
    asm_.raw("\n");
}

} // namespace xcc
