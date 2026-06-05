//
// z80gen_convention.cpp — Concrete Z80 calling-convention classes.
//
// The same abi_convention objects are used by both IR lowering and the
// backend so ABI layout and ABI emission stay consistent.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"
#include "backend/z80/convention.h"

namespace xcc {

namespace {

int arg_size(type_ptr type) {
    return type ? type->size() : 2;
}

bool sdcccall1_caller_cleans(type_ptr ret_type) {
    return ret_type && ret_type->size() >= 4;
}

} // namespace

call_abi effective_call_abi(call_abi abi) {
    return abi == call_abi::DEFAULT ? call_abi::SDCCCALL1 : abi;
}

// ─── abi_convention: shared protected helpers ────────────────────────────────

void abi_convention::exact_stack_drop(z80_gen &g, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        g.emit_line("inc\tsp");
}

void abi_convention::callee_stack_return(z80_gen &g, int bytes)
{
    g.emit_line("pop\tbc");
    exact_stack_drop(g, bytes);
    g.emit_line("push\tbc");
    g.emit_line("ret");
}

void abi_convention::emit_bc_indirect_call(z80_gen &g, const operand &target,
                                           bool preserve_af,
                                           bool preserve_hl,
                                           bool preserve_de)
{
    g.asm_.global_decl("__sdcc_call_bc");
    if (preserve_de)
        g.emit_line("push\tde");
    if (preserve_hl)
        g.emit_line("push\thl");
    if (preserve_af)
        g.emit_line("push\taf");
    g.load_bc(target);
    if (preserve_af)
        g.emit_line("pop\taf");
    if (preserve_hl)
        g.emit_line("pop\thl");
    if (preserve_de)
        g.emit_line("pop\tde");
    g.emit_line("call\t__sdcc_call_bc");
}

void abi_convention::emit_legacy_return_value(z80_gen &g, const operand &value)
{
    if (value.is_none()) return;

    int sz = g.op_size(value);
    if (sz == 1) {
        g.load_a(value);
        g.emit_line("ld\tl, a");
    } else if (sz == 8) {
        g.load_hl_word(value, 0);
        g.emit_line("push\thl");
        g.load_hl_word(value, 1);
        g.emit_line("ex\tde, hl");
        g.emit_line("pop\thl");
    } else if (sz == 4) {
        g.load_hl_hi32(value);
        g.emit_line("push\thl");
        g.load_hl_lo32(value);
        g.emit_line("pop\tde");
    } else {
        g.load_hl(value);
    }
}

void abi_convention::emit_store_legacy_result(z80_gen &g, const icode &ic)
{
    if (ic.result.is_none()) return;

    int sz = g.op_size(ic.result);
    if (sz == 1) {
        g.emit_line("ld\ta, l");
        g.store_a(ic.result);
    } else if (sz == 4) {
        g.store_hl_lo32(ic.result);
        g.emit_line("push\tde");
        g.emit_line("pop\thl");
        g.store_hl_hi32(ic.result);
    } else {
        g.store_hl(ic.result);
    }
}

void abi_convention::emit_modern_return_value(z80_gen &g, const operand &value)
{
    if (value.is_none()) return;

    int sz = g.op_size(value);
    if (sz == 1) {
        g.load_a(value);
    } else if (sz == 8) {
        g.load_hl_word(value, 0);
        g.emit_line("push\thl");
        g.load_hl_word(value, 1);
        g.emit_line("ex\tde, hl");
        g.emit_line("pop\thl");
    } else if (sz == 4) {
        g.load_hl_lo32(value);
        g.emit_line("push\thl");
        g.load_hl_hi32(value);
        g.emit_line("pop\tde");
    } else {
        g.load_hl(value);
        g.emit_line("ex\tde, hl");
    }
}

void abi_convention::emit_store_modern_result(z80_gen &g, const icode &ic)
{
    if (ic.result.is_none()) return;

    int sz = g.op_size(ic.result);
    if (sz == 1) {
        g.store_a(ic.result);
    } else if (sz == 4) {
        g.emit_line("ld\tb, h");
        g.emit_line("ld\tc, l");
        g.emit_line("push\tde");
        g.emit_line("pop\thl");
        g.store_hl_lo32(ic.result);
        g.emit_line("ld\th, b");
        g.emit_line("ld\tl, c");
        g.store_hl_hi32(ic.result);
    } else {
        g.emit_line("push\tde");
        g.emit_line("pop\thl");
        g.store_hl(ic.result);
    }
}

void abi_convention::spill_fastcall_receive(z80_gen &g, const icode &ic)
{
    int off = ic.result.stack_offset;
    switch (ic.arg_loc) {
    case abi_arg_loc::REG_L:
        g.store_frame_byte(off, 'l');
        break;
    case abi_arg_loc::REG_HL:
        g.store_frame_byte(off, 'l');
        g.store_frame_byte(off + 1, 'h');
        break;
    case abi_arg_loc::REG_DEHL:
        g.store_frame_byte(off, 'l');
        g.store_frame_byte(off + 1, 'h');
        g.store_frame_byte(off + 2, 'e');
        g.store_frame_byte(off + 3, 'd');
        break;
    default:
        break;
    }
}

void abi_convention::spill_modern_receive(z80_gen &g, const icode &ic)
{
    int off = ic.result.stack_offset;
    switch (ic.arg_loc) {
    case abi_arg_loc::REG_A:
        g.store_frame_byte(off, 'a');
        break;
    case abi_arg_loc::REG_L:
        g.store_frame_byte(off, 'l');
        break;
    case abi_arg_loc::REG_HL:
        g.store_frame_byte(off, 'l');
        g.store_frame_byte(off + 1, 'h');
        break;
    case abi_arg_loc::REG_DE:
        g.store_frame_byte(off, 'e');
        g.store_frame_byte(off + 1, 'd');
        break;
    case abi_arg_loc::REG_DEHL:
        g.store_frame_byte(off, 'e');
        g.store_frame_byte(off + 1, 'd');
        g.store_frame_byte(off + 2, 'l');
        g.store_frame_byte(off + 3, 'h');
        break;
    default:
        break;
    }
}

void abi_convention::spill_leading_receives(
    z80_gen &g, const ir_function &fn,
    void (*spill_one)(z80_gen &, const icode &))
{
    for (size_t i = 1; i < fn.icodes.size(); ++i) {
        const auto &ic = fn.icodes[i];
        if (ic.op != icode_op::RECEIVE) break;
        if (ic.arg_loc != abi_arg_loc::STACK)
            spill_one(g, ic);
    }
}

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
    if (fn.is_noreturn) {
        g.emit_comment("epilogue omitted: %s is [[noreturn]]", fn.name.c_str());
    } else {
        g.emit_comment("epilogue: %s", fn.name.c_str());
        g.emit_line("ld\tsp, ix");
        g.emit_line("pop\tix");
    }
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
        g.load_hl_hi32(ic.left);
        g.emit_line("push\thl");
        g.load_hl_lo32(ic.left);
        g.emit_line("push\thl");
    } else {
        g.load_hl(ic.left);
        g.emit_line("push\thl");
    }
}

void abi_convention::std_call_cleanup(z80_gen &g, const icode &ic)
{
    int bytes = ic.arg_bytes;
    for (int n = 0; n < bytes / 2; ++n)
        g.emit_line("pop\tbc");
    if (bytes & 1)
        exact_stack_drop(g, 1);
}

// ─── shared stack-linkage base ───────────────────────────────────────────────

struct stack_linkage_convention : abi_convention {
    virtual const char *name() const = 0;
    virtual bool callee_repairs_stack() const { return false; }

    std::vector<abi_arg_loc>
    classify_args(const std::vector<type_ptr> &types) const override {
        return std::vector<abi_arg_loc>(types.size(), abi_arg_loc::STACK);
    }

    int stack_arg_bytes(type_ptr type, abi_arg_loc loc) const override {
        if (loc != abi_arg_loc::STACK) return 0;
        int sz = arg_size(type);
        return sz < 2 ? 2 : sz;
    }

    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("%s prologue: %s (locals=%d, stack_params=%d)",
                       name(), fn.name.c_str(), fn.local_bytes, fn.stack_param_bytes);
        std_prologue_frame(g, fn);
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        std_epilogue_frame(g, fn);
        if (!fn.is_noreturn) {
            if (callee_repairs_stack() && fn.stack_param_bytes > 0)
                callee_stack_return(g, fn.stack_param_bytes);
            else
                g.emit_line("ret");
        }
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        g.emit_comment("receive (%s) param %s at %s",
                       name(), ic.result.name.c_str(), g.addr_of(ic.result).c_str());
    }

    void emit_send(z80_gen &g, const icode &ic) override {
        std_send_push(g, ic);
    }

    void emit_call_cleanup(z80_gen &g, const icode &ic) override {
        if (!callee_repairs_stack())
            std_call_cleanup(g, ic);
    }

    void emit_indirect_call(z80_gen &g, const icode &ic) const override {
        emit_bc_indirect_call(g, ic.left, false, true, true);
    }

    void emit_return_value(z80_gen &g, const operand &value) const override {
        emit_legacy_return_value(g, value);
    }

    void emit_store_call_result(z80_gen &g, const icode &ic) const override {
        emit_store_legacy_result(g, ic);
    }
};

struct cc_sdcccall0 final : stack_linkage_convention {
    call_abi abi_tag() const override { return call_abi::SDCCCALL0; }
    const char *name() const override { return "sdcccall(0)"; }
};

struct cc_z88dk_callee final : stack_linkage_convention {
    call_abi abi_tag() const override { return call_abi::Z88DK_CALLEE; }
    const char *name() const override { return "z88dk callee"; }
    bool callee_repairs_stack() const override { return true; }
};

// ─── z88dk fastcall ──────────────────────────────────────────────────────────

struct cc_z88dk_fastcall final : abi_convention {
    call_abi abi_tag() const override { return call_abi::Z88DK_FASTCALL; }

    std::vector<abi_arg_loc>
    classify_args(const std::vector<type_ptr> &types) const override {
        std::vector<abi_arg_loc> locs(types.size(), abi_arg_loc::STACK);
        if (types.size() != 1) return locs;

        switch (arg_size(types[0])) {
        case 1: locs[0] = abi_arg_loc::REG_L;    break;
        case 2: locs[0] = abi_arg_loc::REG_HL;   break;
        case 4: locs[0] = abi_arg_loc::REG_DEHL; break;
        default: break;
        }
        return locs;
    }

    int stack_arg_bytes(type_ptr type, abi_arg_loc loc) const override {
        if (loc != abi_arg_loc::STACK) return 0;
        int sz = arg_size(type);
        return sz < 2 ? 2 : sz;
    }

    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("z88dk fastcall prologue: %s (locals=%d, stack_params=%d)",
                       fn.name.c_str(), fn.local_bytes, fn.stack_param_bytes);
        g.emit_line("push\tix");
        g.emit_line("ld\tix, %s", g.asm_.imm(0).c_str());
        g.emit_line("add\tix, sp");
        spill_leading_receives(g, fn, spill_fastcall_receive);
        if (fn.local_bytes > 0) {
            g.emit_line("ld\thl, %s", g.asm_.imm(-fn.local_bytes).c_str());
            g.emit_line("add\thl, sp");
            g.emit_line("ld\tsp, hl");
        }
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        std_epilogue_frame(g, fn);
        if (!fn.is_noreturn)
            g.emit_line("ret");
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        g.emit_comment("receive (z88dk fastcall) param %s at %s",
                       ic.result.name.c_str(), g.addr_of(ic.result).c_str());
    }

    void emit_send(z80_gen &g, const icode &ic) override {
        switch (ic.arg_loc) {
        case abi_arg_loc::REG_L:
            g.load_a(ic.left);
            g.emit_line("ld\tl, a");
            break;
        case abi_arg_loc::REG_HL:
            g.load_hl(ic.left);
            break;
        case abi_arg_loc::REG_DEHL:
            g.load_hl_hi32(ic.left);
            g.emit_line("push\thl");
            g.load_hl_lo32(ic.left);
            g.emit_line("pop\tde");
            break;
        default:
            std_send_push(g, ic);
            break;
        }
    }

    void emit_call_cleanup(z80_gen &g, const icode &ic) override {
        std_call_cleanup(g, ic);
    }

    void emit_indirect_call(z80_gen &g, const icode &ic) const override {
        g.asm_.global_decl("__call_hl");
        g.load_hl(ic.left);
        g.emit_line("call\t__call_hl");
    }

    void emit_return_value(z80_gen &g, const operand &value) const override {
        emit_legacy_return_value(g, value);
    }

    void emit_store_call_result(z80_gen &g, const icode &ic) const override {
        emit_store_legacy_result(g, ic);
    }
};

// ─── sdcccall(1) ─────────────────────────────────────────────────────────────

struct cc_sdcccall1 final : abi_convention {
    call_abi abi_tag() const override { return call_abi::SDCCCALL1; }

    std::vector<abi_arg_loc>
    classify_args(const std::vector<type_ptr> &types) const override {
        std::vector<abi_arg_loc> locs(types.size(), abi_arg_loc::STACK);
        if (types.empty()) return locs;

        int s0 = arg_size(types[0]);
        if (s0 == 1) {
            locs[0] = abi_arg_loc::REG_A;
            if (types.size() > 1) {
                int s1 = arg_size(types[1]);
                if (s1 == 1)      locs[1] = abi_arg_loc::REG_L;
                else if (s1 == 2) locs[1] = abi_arg_loc::REG_DE;
            }
        } else if (s0 == 2) {
            locs[0] = abi_arg_loc::REG_HL;
            if (types.size() > 1 && arg_size(types[1]) == 2)
                locs[1] = abi_arg_loc::REG_DE;
        } else if (s0 == 4) {
            locs[0] = abi_arg_loc::REG_DEHL;
        }

        return locs;
    }

    int stack_arg_bytes(type_ptr type, abi_arg_loc loc) const override {
        return loc == abi_arg_loc::STACK ? arg_size(type) : 0;
    }

    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("sdcccall(1) prologue: %s (locals=%d, stack_params=%d)",
                       fn.name.c_str(), fn.local_bytes, fn.stack_param_bytes);
        g.emit_line("push\tix");
        g.emit_line("ld\tix, %s", g.asm_.imm(0).c_str());
        g.emit_line("add\tix, sp");
        spill_leading_receives(g, fn, spill_modern_receive);
        if (fn.local_bytes > 0) {
            g.emit_line("ld\thl, %s", g.asm_.imm(-fn.local_bytes).c_str());
            g.emit_line("add\thl, sp");
            g.emit_line("ld\tsp, hl");
        }
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        std_epilogue_frame(g, fn);
        if (!fn.is_noreturn) {
            if (fn.stack_param_bytes > 0 && !sdcccall1_caller_cleans(fn.ret_type))
                callee_stack_return(g, fn.stack_param_bytes);
            else
                g.emit_line("ret");
        }
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        g.emit_comment("receive (sdcccall1) param %s at %s",
                       ic.result.name.c_str(), g.addr_of(ic.result).c_str());
    }

    void emit_send(z80_gen &g, const icode &ic) override {
        switch (ic.arg_loc) {
        case abi_arg_loc::REG_A:
            g.load_a(ic.left);
            break;
        case abi_arg_loc::REG_L:
            g.load_a(ic.left);
            g.emit_line("ld\tl, a");
            break;
        case abi_arg_loc::REG_HL:
            g.load_hl(ic.left);
            break;
        case abi_arg_loc::REG_DE:
            g.load_de(ic.left);
            break;
        case abi_arg_loc::REG_DEHL:
            g.load_hl_lo32(ic.left);
            g.emit_line("push\thl");
            g.load_hl_hi32(ic.left);
            g.emit_line("pop\tde");
            break;
        case abi_arg_loc::STACK:
            if (g.op_size(ic.left) == 1) {
                g.load_a(ic.left);
                g.emit_line("push\taf");
                g.emit_line("inc\tsp");
            } else {
                std_send_push(g, ic);
            }
            break;
        }
    }

    void emit_call_cleanup(z80_gen &g, const icode &ic) override {
        if (ic.arg_bytes > 0 && sdcccall1_caller_cleans(ic.result.type))
            exact_stack_drop(g, ic.arg_bytes);
    }

    void emit_indirect_call(z80_gen &g, const icode &ic) const override {
        emit_bc_indirect_call(g, ic.left, true, true, true);
    }

    void emit_return_value(z80_gen &g, const operand &value) const override {
        emit_modern_return_value(g, value);
    }

    void emit_store_call_result(z80_gen &g, const icode &ic) const override {
        emit_store_modern_result(g, ic);
    }
};

// ─── special attributes ──────────────────────────────────────────────────────

struct cc_naked final : abi_convention {
    call_abi abi_tag() const override { return call_abi::NAKED; }

    std::vector<abi_arg_loc>
    classify_args(const std::vector<type_ptr> &types) const override {
        return std::vector<abi_arg_loc>(types.size(), abi_arg_loc::STACK);
    }

    int stack_arg_bytes(type_ptr type, abi_arg_loc loc) const override {
        if (loc != abi_arg_loc::STACK) return 0;
        int sz = arg_size(type);
        return sz < 2 ? 2 : sz;
    }

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

    void emit_indirect_call(z80_gen &g, const icode &ic) const override {
        g.asm_.global_decl("__call_hl");
        g.load_hl(ic.left);
        g.emit_line("call\t__call_hl");
    }

    void emit_return_value(z80_gen &g, const operand &value) const override {
        emit_legacy_return_value(g, value);
    }

    void emit_store_call_result(z80_gen &g, const icode &ic) const override {
        emit_store_legacy_result(g, ic);
    }
};

struct cc_interrupt final : abi_convention {
    call_abi abi_tag() const override { return call_abi::INTERRUPT; }

    std::vector<abi_arg_loc>
    classify_args(const std::vector<type_ptr> &types) const override {
        return std::vector<abi_arg_loc>(types.size(), abi_arg_loc::STACK);
    }

    int stack_arg_bytes(type_ptr type, abi_arg_loc loc) const override {
        if (loc != abi_arg_loc::STACK) return 0;
        int sz = arg_size(type);
        return sz < 2 ? 2 : sz;
    }

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

    void emit_indirect_call(z80_gen &g, const icode &ic) const override {
        g.asm_.global_decl("__call_hl");
        g.load_hl(ic.left);
        g.emit_line("call\t__call_hl");
    }

    void emit_return_value(z80_gen &g, const operand &value) const override {
        emit_legacy_return_value(g, value);
    }

    void emit_store_call_result(z80_gen &g, const icode &ic) const override {
        emit_store_legacy_result(g, ic);
    }
};

struct cc_critical final : abi_convention {
    call_abi abi_tag() const override { return call_abi::CRITICAL; }

    std::vector<abi_arg_loc>
    classify_args(const std::vector<type_ptr> &types) const override {
        return std::vector<abi_arg_loc>(types.size(), abi_arg_loc::STACK);
    }

    int stack_arg_bytes(type_ptr type, abi_arg_loc loc) const override {
        if (loc != abi_arg_loc::STACK) return 0;
        int sz = arg_size(type);
        return sz < 2 ? 2 : sz;
    }

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
        if (!fn.is_noreturn) {
            g.emit_comment("critical epilogue: %s", fn.name.c_str());
            g.emit_line("ei");
            g.emit_line("ret");
        }
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

    void emit_indirect_call(z80_gen &g, const icode &ic) const override {
        g.asm_.global_decl("__call_hl");
        g.load_hl(ic.left);
        g.emit_line("call\t__call_hl");
    }

    void emit_return_value(z80_gen &g, const operand &value) const override {
        emit_legacy_return_value(g, value);
    }

    void emit_store_call_result(z80_gen &g, const icode &ic) const override {
        emit_store_legacy_result(g, ic);
    }
};

abi_convention &get_abi_convention(call_abi abi)
{
    static cc_sdcccall0      sdcccall0;
    static cc_sdcccall1      sdcccall1;
    static cc_z88dk_fastcall z88dk_fastcall;
    static cc_z88dk_callee   z88dk_callee;
    static cc_naked          naked;
    static cc_interrupt      interrupt;
    static cc_critical       critical;

    switch (effective_call_abi(abi)) {
    case call_abi::SDCCCALL0:      return sdcccall0;
    case call_abi::SDCCCALL1:      return sdcccall1;
    case call_abi::Z88DK_FASTCALL: return z88dk_fastcall;
    case call_abi::Z88DK_CALLEE:   return z88dk_callee;
    case call_abi::NAKED:          return naked;
    case call_abi::INTERRUPT:      return interrupt;
    case call_abi::CRITICAL:       return critical;
    default:                       return sdcccall1;
    }
}

} // namespace xcc
