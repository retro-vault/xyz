//
// z80gen_convention.cpp — Concrete Z80 calling-convention classes.
//
// abi_convention is a friend of z80_gen, so all methods defined here can
// access z80_gen's private members (emit_line, load_hl, asm_, etc.).
// The shared helpers are defined as protected methods of abi_convention
// rather than free functions so that friendship is properly inherited.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"
#include "backend/z80/convention.h"
#include <algorithm>

namespace xcc {

// ─── abi_convention: shared protected helpers ────────────────────────────────
//
// These are defined in the .cpp so they only compile once.
// They are protected: all concrete subclasses inherit them.

void abi_convention::std_prologue_frame(z80_gen &g, const ir_function &fn)
{
    g.emit_line("push\tix");
    g.emit_line("ld\tix, %s", g.asm_.imm(0).c_str());
    g.emit_line("add\tix, sp");
    if (fn.local_bytes > 0) {
        g.emit_line("ld\thl, %s", g.asm_.imm(-fn.local_bytes).c_str());
        g.emit_line("add\thl, sp");
        g.emit_line("ld\tsp, hl");
    }
}

void abi_convention::std_epilogue_frame(z80_gen &g, const ir_function &fn)
{
    g.emit_label(g.fn_end_lbl_, false);
    g.emit_comment("epilogue: %s", fn.name.c_str());
    g.emit_line("ld\tsp, ix");
    g.emit_line("pop\tix");
}

void abi_convention::std_send_push(z80_gen &g, const icode &ic)
{
    int sz = g.op_size(ic.left);
    if (sz == 1) {
        g.load_a(ic.left);
        g.emit_line("ld\tl, a");
        g.emit_line("ld\th, %s", g.asm_.imm(0).c_str());
        g.emit_line("push\thl");
    } else if (sz == 8) {
        for (int w = 3; w >= 0; --w) {
            g.load_hl_word(ic.left, w);
            g.emit_line("push\thl");
        }
    } else if (sz == 4) {
        g.load_hl_hi32(ic.left); g.emit_line("push\thl");
        g.load_hl_lo32(ic.left); g.emit_line("push\thl");
    } else {
        g.load_hl(ic.left);
        g.emit_line("push\thl");
    }
}

void abi_convention::std_call_cleanup(z80_gen &g, const icode &ic)
{
    if (ic.num_params > 0) {
        int bytes = ic.arg_bytes > 0 ? ic.arg_bytes : ic.num_params * 2;
        for (int n = 0; n < bytes / 2; ++n)
            g.emit_line("pop\tbc");
    }
}

// ─── cc_default ─────────────────────────────────────────────────────────────

struct cc_default : abi_convention {
    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("prologue: %s (locals=%d)", fn.name.c_str(), fn.local_bytes);
        std_prologue_frame(g, fn);
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        std_epilogue_frame(g, fn);
        g.emit_line("ret");
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        g.emit_comment("receive param %s at %s",
                       ic.result.name.c_str(), g.addr_of(ic.result).c_str());
    }

    void emit_send(z80_gen &g, const icode &ic) override {
        std_send_push(g, ic);
    }

    void emit_call_cleanup(z80_gen &g, const icode &ic) override {
        std_call_cleanup(g, ic);
    }
};

// ─── cc_sdcccall1 ────────────────────────────────────────────────────────────

struct cc_sdcccall1 : abi_convention {
    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("sdcccall(1) prologue: %s (locals=%d, reg_params=%d)",
                       fn.name.c_str(), fn.local_bytes, fn.reg_param_count);

        g.emit_line("push\tix");
        g.emit_line("ld\tix, %s", g.asm_.imm(0).c_str());
        g.emit_line("add\tix, sp");

        // Spill incoming register params to local frame slots BEFORE adjusting SP.
        // Writing below the current SP is safe on Z80 (no memory protection).
        static const char lo_reg[] = {'l', 'e', 'c'};
        static const char hi_reg[] = {'h', 'd', 'b'};
        for (int i = 0; i < fn.reg_param_count; ++i) {
            int off = -(fn.orig_local_bytes + 2 * (i + 1));
            g.emit_line("ld\t%s, %c", g.asm_.ix_rel(off    ).c_str(), lo_reg[i]);
            g.emit_line("ld\t%s, %c", g.asm_.ix_rel(off + 1).c_str(), hi_reg[i]);
        }

        if (fn.local_bytes > 0) {
            g.emit_line("ld\thl, %s", g.asm_.imm(-fn.local_bytes).c_str());
            g.emit_line("add\thl, sp");
            g.emit_line("ld\tsp, hl");
        }
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        std_epilogue_frame(g, fn);
        g.emit_line("ret");
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        // Register params already spilled by prologue; stack params already on frame.
        g.emit_comment("receive (sdcccall1) param %s at %s",
                       ic.result.name.c_str(), g.addr_of(ic.result).c_str());
    }

    void emit_send(z80_gen &g, const icode &ic) override {
        int argreg = ic.argreg;
        int sz     = g.op_size(ic.left);

        if (argreg == 0) {
            if (sz == 1) { g.load_a(ic.left); g.emit_line("ld\tl, a"); }
            else           g.load_hl(ic.left);
        } else if (argreg == 1) {
            if (sz == 1) { g.load_a(ic.left); g.emit_line("ld\te, a"); }
            else           g.load_de(ic.left);
        } else if (argreg == 2) {
            if (sz == 1) { g.load_a(ic.left); g.emit_line("ld\tc, a"); }
            else           g.load_bc(ic.left);
        } else {
            std_send_push(g, ic);
        }
    }

    void emit_call_cleanup(z80_gen &g, const icode &ic) override {
        // For sdccall(1), arg_bytes is exactly the stack-pushed bytes (0 when all args
        // are register-passed). Skip the num_params fallback in std_call_cleanup.
        if (ic.arg_bytes > 0) {
            for (int n = 0; n < ic.arg_bytes / 2; ++n)
                g.emit_line("pop\tbc");
        }
    }

    int stack_bytes_for(int argreg, int /*arg_sz*/) const override {
        return argreg < 3 ? 0 : abi_convention::stack_bytes_for(argreg, 2);
    }
};

// ─── cc_naked ────────────────────────────────────────────────────────────────

struct cc_naked : abi_convention {
    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("naked: %s", fn.name.c_str());
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        g.emit_label(g.fn_end_lbl_, false);
        g.emit_comment("naked epilogue: %s", fn.name.c_str());
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &, const icode &) override {}

    void emit_send(z80_gen &g, const icode &ic) override {
        std_send_push(g, ic);
    }

    void emit_call_cleanup(z80_gen &g, const icode &ic) override {
        std_call_cleanup(g, ic);
    }
};

// ─── cc_interrupt ────────────────────────────────────────────────────────────

struct cc_interrupt : abi_convention {
    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("interrupt prologue: %s", fn.name.c_str());
        g.emit_line("push\taf");
        g.emit_line("push\tbc");
        g.emit_line("push\tde");
        g.emit_line("push\thl");
        g.emit_line("push\tiy");
        std_prologue_frame(g, fn);
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        g.emit_label(g.fn_end_lbl_, false);
        g.emit_comment("interrupt epilogue: %s", fn.name.c_str());
        g.emit_line("ld\tsp, ix");
        g.emit_line("pop\tix");
        g.emit_line("pop\tiy");
        g.emit_line("pop\thl");
        g.emit_line("pop\tde");
        g.emit_line("pop\tbc");
        g.emit_line("pop\taf");
        g.emit_line("reti");
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        g.emit_comment("receive param %s at %s",
                       ic.result.name.c_str(), g.addr_of(ic.result).c_str());
    }

    void emit_send(z80_gen &g, const icode &ic) override {
        std_send_push(g, ic);
    }

    void emit_call_cleanup(z80_gen &g, const icode &ic) override {
        std_call_cleanup(g, ic);
    }
};

// ─── cc_critical ─────────────────────────────────────────────────────────────

struct cc_critical : abi_convention {
    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("critical prologue: %s", fn.name.c_str());
        g.emit_line("di");
        std_prologue_frame(g, fn);
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        std_epilogue_frame(g, fn);
        g.emit_comment("critical epilogue: %s", fn.name.c_str());
        g.emit_line("ei");
        g.emit_line("ret");
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        g.emit_comment("receive param %s at %s",
                       ic.result.name.c_str(), g.addr_of(ic.result).c_str());
    }

    void emit_send(z80_gen &g, const icode &ic) override {
        std_send_push(g, ic);
    }

    void emit_call_cleanup(z80_gen &g, const icode &ic) override {
        std_call_cleanup(g, ic);
    }
};

// ─── factory ─────────────────────────────────────────────────────────────────

std::unique_ptr<abi_convention> make_abi_convention(call_abi abi)
{
    switch (abi) {
    case call_abi::SDCCCALL1: return std::make_unique<cc_sdcccall1>();
    case call_abi::NAKED:     return std::make_unique<cc_naked>();
    case call_abi::INTERRUPT: return std::make_unique<cc_interrupt>();
    case call_abi::CRITICAL:  return std::make_unique<cc_critical>();
    default:                  return std::make_unique<cc_default>();
    }
}

} // namespace xcc
