//
// z80gen_data.cpp — Module-level emission: globals, TLS, string literals.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"
#include <string>

namespace xcc {

void z80_gen::emit_module(const ir_module &mod) {
    asm_.module_header();

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

    asm_.section_code();
    for (auto &fn : mod.functions)
        emit_function(fn);

    if (debug_) debug_->end_module("xcc 0.1.0");
}

void z80_gen::emit_global_body(const ir_module::global_var &g, bool tls_template) {
    if (!g.init_vals.empty() && !tls_template) {
        for (auto &e : g.init_vals) {
            if (e.size == 1)      asm_.db((int)e.value);
            else if (e.size == 2) asm_.dw((int)(e.value & 0xFFFF));
            else if (e.size == 4) {
                asm_.dw((int)(e.value & 0xFFFF));
                asm_.dw((int)((e.value >> 16) & 0xFFFF));
            } else asm_.ds(e.size);
        }
    } else {
        int sz = g.type ? g.type->size() : 2;
        if (!tls_template && g.has_init && g.init_val != 0) {
            if (sz == 1)      asm_.db((int)g.init_val);
            else if (sz == 2) asm_.dw((int)g.init_val);
            else              asm_.ds(sz > 0 ? sz : 2);
        } else {
            asm_.ds(sz > 0 ? sz : 2);
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

    bool any_data = false;
    for (auto &g : mod.globals)
        if (!g.is_tls && g.at_address < 0 && g.sfr_port < 0) { any_data = true; break; }
    if (any_data) {
        asm_.section_data();
        for (auto &g : mod.globals) {
            if (g.is_tls) continue;
            if (g.at_address >= 0 || g.sfr_port >= 0) continue; // handled above
            std::string lbl = mangle(g.name);
            if (!g.is_static) asm_.global_decl(lbl);
            asm_.label(lbl, false);
            emit_global_body(g, false);
        }
        asm_.raw("\n");
    }

    if (tls_size_ > 0) {
        asm_.section_tls();
        asm_.global_decl("__tls_template");
        asm_.label("__tls_template", false);
        for (auto &g : mod.globals) {
            if (!g.is_tls) continue;
            std::string lbl = mangle(g.name);
            asm_.comment("tls: " + lbl + " @ offset " + std::to_string(tls_offsets_[lbl]));
            emit_global_body(g, true);
        }
        asm_.global_decl("__tls_size");
        asm_.symbol_assign("__tls_size", tls_size_);
        asm_.raw("\n");
    }
}

void z80_gen::emit_strings(const ir_module &mod) {
    if (mod.string_literals.empty()) return;
    asm_.section_rodata();
    for (auto &s : mod.string_literals) {
        asm_.label(mangle(s.name), false);
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
    }
    asm_.raw("\n");
}

} // namespace xcc
