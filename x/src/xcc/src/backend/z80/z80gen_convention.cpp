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

bool has_fixed_frame_hazards(const ir_function &fn) {
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::ALLOCA || ic.op == icode_op::INLINE_ASM)
            return true;
    }
    return false;
}

bool uses_temp(const operand &op, int temp_id) {
    return op.is_temp() && op.temp_id == temp_id;
}

bool same_local_symbol(const operand &a, const operand &b) {
    return a.kind == operand_kind::SYMBOL &&
           b.kind == operand_kind::SYMBOL &&
           !a.is_global &&
           !b.is_global &&
           a.stack_offset == b.stack_offset &&
           a.name == b.name;
}

bool icode_uses_temp(const icode &ic, int temp_id) {
    return uses_temp(ic.left, temp_id) ||
           uses_temp(ic.right, temp_id);
}

bool icode_uses_symbol(const icode &ic, const operand &sym) {
    return same_local_symbol(ic.left, sym) ||
           same_local_symbol(ic.right, sym);
}

bool is_passive_leading_op(const icode &ic) {
    return ic.op == icode_op::RECEIVE;
}

bool is_safe_receive_prefix_init(const icode &ic) {
    if (ic.op != icode_op::ASSIGN ||
        !ic.right.is_none() ||
        ic.left.kind != operand_kind::INT_CONST) {
        return false;
    }

    if (!ic.result.is_symbol() || ic.result.is_global || !ic.result.type)
        return false;

    // Byte local/param constant stores lower to an A + IX store sequence and
    // do not need to consume an incoming word argument register pair first.
    return ic.result.type->size() == 1 &&
           ic.left.ival >= 0 && ic.left.ival <= 0xff;
}

[[maybe_unused]] bool is_straight_line_helper_fn(const ir_function &fn) {
    if (fn.local_bytes != 0)
        return false;
    for (size_t i = 1; i + 1 < fn.icodes.size(); ++i) {
        const auto &ic = fn.icodes[i];
        switch (ic.op) {
        case icode_op::RECEIVE:
        case icode_op::ASSIGN:
        case icode_op::ADDRESS_OF:
        case icode_op::GET_VALUE_AT:
        case icode_op::SET_VALUE_AT:
        case icode_op::ADD:
        case icode_op::SUB:
        case icode_op::NEG:
        case icode_op::MUL:
        case icode_op::DIV:
        case icode_op::MOD:
        case icode_op::BAND:
        case icode_op::BOR:
        case icode_op::BXOR:
        case icode_op::BNOT:
        case icode_op::SHL:
        case icode_op::SHR:
        case icode_op::ROL:
        case icode_op::ROR:
        case icode_op::EQ:
        case icode_op::NE:
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::GT:
        case icode_op::GE:
        case icode_op::CAST:
        case icode_op::RETURN:
        case icode_op::MAKE_COMPLEX:
        case icode_op::FADD:
        case icode_op::FSUB:
        case icode_op::FMUL:
        case icode_op::FDIV:
        case icode_op::FITOSF:
        case icode_op::FSTOI:
            break;
        default:
            return false;
        }
    }
    return true;
}

[[maybe_unused]] int count_symbol_uses_after(const ir_function &fn,
                                             size_t receive_idx,
                                             const operand &sym) {
    int count = 0;
    for (size_t i = receive_idx + 1; i < fn.icodes.size(); ++i) {
        const auto &scan = fn.icodes[i];
        if (icode_uses_symbol(scan, sym))
            ++count;
        if (same_local_symbol(scan.result, sym))
            ++count;
    }
    return count;
}

[[maybe_unused]] int count_temp_uses_after(const ir_function &fn,
                                           size_t receive_idx,
                                           int temp_id) {
    int count = 0;
    for (size_t i = receive_idx + 1; i < fn.icodes.size(); ++i) {
        const auto &scan = fn.icodes[i];
        if (icode_uses_temp(scan, temp_id))
            ++count;
        if (uses_temp(scan.result, temp_id))
            ++count;
    }
    return count;
}

[[maybe_unused]] size_t first_symbol_use_after(const ir_function &fn,
                                               size_t receive_idx,
                                               const operand &sym) {
    for (size_t i = receive_idx + 1; i < fn.icodes.size(); ++i) {
        const auto &scan = fn.icodes[i];
        if (icode_uses_symbol(scan, sym) ||
            same_local_symbol(scan.result, sym)) {
            return i;
        }
    }
    return fn.icodes.size();
}

[[maybe_unused]] size_t first_temp_use_after(const ir_function &fn,
                                             size_t receive_idx,
                                             int temp_id) {
    for (size_t i = receive_idx + 1; i < fn.icodes.size(); ++i) {
        const auto &scan = fn.icodes[i];
        if (icode_uses_temp(scan, temp_id) ||
            uses_temp(scan.result, temp_id)) {
            return i;
        }
    }
    return fn.icodes.size();
}

temp_home incoming_arg_home(abi_arg_loc loc) {
    switch (loc) {
    case abi_arg_loc::REG_A:  return temp_home::arg_a;
    case abi_arg_loc::REG_L:  return temp_home::arg_l;
    case abi_arg_loc::REG_HL: return temp_home::arg_hl;
    case abi_arg_loc::REG_DE: return temp_home::arg_de;
    default:                  return temp_home::stack;
    }
}

bool should_keep_modern_receive_in_register(const ir_function &fn,
                                            size_t receive_idx,
                                            const icode &ic) {
    if (!ic.result.is_temp() && !ic.result.is_symbol())
        return false;
    if (incoming_arg_home(ic.arg_loc) == temp_home::stack)
        return false;

    for (size_t i = receive_idx + 1; i < fn.icodes.size(); ++i) {
        const auto &scan = fn.icodes[i];
        if (is_passive_leading_op(scan))
            continue;
        // A label before the first real use means the "prefix" is not
        // single-entry straight-line code anymore: a backedge can re-enter
        // the later-emitted loads after the incoming register has already
        // been clobbered inside the loop body.
        if (scan.op == icode_op::LABEL)
            return false;
        if (scan.op == icode_op::CALL)
            return false;
        if (ic.result.is_temp()) {
            if (icode_uses_temp(scan, ic.result.temp_id))
                return true;
            if (uses_temp(scan.result, ic.result.temp_id))
                return false;
        } else {
            if (icode_uses_symbol(scan, ic.result))
                return true;
            if (same_local_symbol(scan.result, ic.result))
                return false;
        }
        if (is_safe_receive_prefix_init(scan))
            continue;
        return false;
    }
    return false;
}

bool sdcccall1_callee_cleans_stack(type_ptr,
                                   const std::vector<type_ptr> &,
                                   bool) {
    // xcc follows the modern SDCC register ABI for argument placement, but
    // stack-spilled arguments remain caller-clean throughout the toolchain.
    // That matches the documented ABI, the hand-written runtime/platform
    // helpers, and imported library symbols whose metadata records only the
    // calling convention family, not a separate callee-pop variant.
    return false;
}

} // namespace

bool z80_gen::can_omit_frame_pointer(const ir_function &fn) const {
    if (!frame_omit_enabled()) return false;
    if (temp_stack_bytes_ != 0) return false;
    if (required_frame_bytes() != 0) return false;
    return !needs_frame_without_temps(fn);
}

bool z80_gen::needs_frame_without_temps(const ir_function &fn) const {
    if (local_bytes_ != 0) return true;
    if (fn.stack_param_bytes != 0) return true;
    if (has_fixed_frame_hazards(fn)) return true;

    switch (effective_call_abi(fn.abi)) {
    case call_abi::SDCCCALL0:
    case call_abi::SDCCCALL1:
    case call_abi::Z88DK_SMALLC:
    case call_abi::Z88DK_FASTCALL:
    case call_abi::Z88DK_CALLEE:
        return false;
    default:
        return true;
    }
}

call_abi effective_call_abi(call_abi abi) {
    return abi == call_abi::DEFAULT ? get_default_call_abi() : abi;
}

bool abi_callee_cleans_stack(call_abi abi,
                             type_ptr ret_type,
                             const std::vector<type_ptr> &arg_types,
                             bool variadic) {
    switch (effective_call_abi(abi)) {
    case call_abi::SDCCCALL1:
        return sdcccall1_callee_cleans_stack(ret_type, arg_types, variadic);
    case call_abi::Z88DK_CALLEE:
        return true;
    default:
        return false;
    }
}

// ─── abi_convention: shared protected helpers ────────────────────────────────

void abi_convention::exact_stack_drop(z80_gen &g, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        g.emit_line("inc\tsp");
}

void abi_convention::callee_stack_return(z80_gen &g, int bytes,
                                         bool hl_is_scratch)
{
    // In size mode a modern-ABI void/small-integer result leaves HL free.
    // Use it as the return-address carrier and jump directly after discarding
    // the arguments.  This is smaller than preserving the address in BC,
    // adjusting SP byte by byte, pushing it again, and executing RET.
    if (g.size_opt_enabled() && hl_is_scratch) {
        g.emit_line("pop\thl");
        for (int n = 0; n < bytes / 2; ++n)
            g.emit_line("pop\tbc");
        if (bytes & 1)
            exact_stack_drop(g, 1);
        g.emit_line("jp\t(hl)");
        return;
    }

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
    if (g.operand_home_in_bc(target)) {
        g.emit_line("call\t__sdcc_call_bc");
        return;
    }
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

void abi_convention::emit_far_ptr_to_regs(z80_gen &g, const operand &value)
{
    // HL = address (bytes 0..1), E = bank (byte 2), D = 0.
    g.load_hl(value);
    g.load_far_bank(value);
    g.emit_line("ld\te, a");
    g.emit_line("ld\td, %s", g.asm_.imm(0).c_str());
}

void abi_convention::emit_far_ptr_from_regs(z80_gen &g, const operand &result)
{
    // Store HL -> address (bytes 0..1), E -> bank (byte 2).
    g.store_hl(result);
    g.emit_line("ld\ta, e");
    g.store_far_bank(result);
}

void abi_convention::emit_legacy_return_value(z80_gen &g, const operand &value)
{
    if (value.is_none()) return;

    int sz = g.op_size(value);
    if (sz == 1) {
        g.load_a(value);
        g.emit_line("ld\tl, a");
    } else if (sz == 8) {
        g.load_reg64(value);
    } else if (sz == 4) {
        g.load_hl_lo32(value);
        g.load_de_word(value, 1);
    } else if (sz == 3 && value.type && value.type->is_far_ptr()) {
        // Far (24-bit) pointer return: HL = address, E = bank.
        emit_far_ptr_to_regs(g, value);
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
    } else if (sz == 8) {
        g.store_reg64(ic.result);
    } else if (sz == 4) {
        g.store_hl_lo32(ic.result);
        g.store_de_word(ic.result, 1);
    } else if (sz == 3 && ic.result.type && ic.result.type->is_far_ptr()) {
        // Far (24-bit) pointer result: HL = address, E = bank.
        emit_far_ptr_from_regs(g, ic.result);
    } else {
        g.store_hl(ic.result);
    }
}

void abi_convention::emit_modern_return_value(z80_gen &g, const operand &value)
{
    if (value.is_none()) return;

    int sz = g.op_size(value);
    auto prefer_direct_de_return = [&]() -> bool {
        if (sz != 2)
            return false;

        if (g.direct_word_value_pending_ &&
            g.direct_word_value_.is_temp() &&
            value.is_temp() &&
            value.temp_id == g.direct_word_value_.temp_id) {
            return true;
        }

        switch (value.kind) {
        case operand_kind::INT_CONST:
        case operand_kind::FLOAT_CONST:
        case operand_kind::LABEL_REF:
            return true;
        case operand_kind::SYMBOL:
            if (value.is_global)
                return value.is_tls;
            if (g.symbol_home_in_bc(value) && value.byte_offset == 0)
                return true;
            if (value.byte_offset == 0) {
                auto it = g.incoming_symbol_homes_.find(value.stack_offset);
                if (it != g.incoming_symbol_homes_.end()) {
                    return it->second == temp_home::arg_de;
                }
            }
            return true;
        case operand_kind::TEMP: {
            auto it = g.temp_regs_.find(value.temp_id);
            if (it == g.temp_regs_.end())
                return true;
            switch (it->second) {
            case temp_home::main_hl:
            case temp_home::remat_hl:
            case temp_home::arg_hl:
                return false;
            default:
                return true;
            }
        }
        default:
            return false;
        }
    };

    if (sz > 8) {
        if (g.cur_fn_ && g.cur_fn_->stack_param_bytes > 0) {
            for (int w = 0; w < sz / 2; ++w) {
                g.load_hl_word(value, w);
                g.store_frame_word(z80_gen::reg_pair{"hl", 'l', 'h', false},
                                   4 + (w * 2));
            }
            if (sz & 1) {
                operand tail = value;
                tail.byte_offset += (sz - 1);
                tail.type = type::make_char();
                g.load_a(tail);
                g.store_frame_byte(4 + (sz - 1), 'a');
            }
            g.emit_line("push\tix");
            g.emit_line("pop\thl");
            g.emit_line("ld\tde, %s", g.asm_.imm(4).c_str());
            g.emit_line("add\thl, de");
            g.emit_line("ex\tde, hl");
        } else {
            if (prefer_direct_de_return()) {
                g.load_de(value);
            } else {
                g.load_hl(value);
                g.emit_line("ex\tde, hl");
            }
        }
        return;
    }

    if (sz == 1) {
        g.load_a(value);
    } else if (sz == 8) {
        g.load_reg64(value);
    } else if (sz == 4) {
        g.load_de_word(value, 0);
        g.load_hl_hi32(value);
    } else if (sz == 3 && value.type && value.type->is_far_ptr()) {
        // Far (24-bit) pointer return: HL = address, E = bank.
        emit_far_ptr_to_regs(g, value);
    } else {
        if (prefer_direct_de_return()) {
            g.load_de(value);
        } else {
            g.load_hl(value);
            g.emit_line("ex\tde, hl");
        }
    }
}

void abi_convention::emit_store_modern_result(z80_gen &g, const icode &ic)
{
    if (ic.result.is_none()) return;

    int sz = g.op_size(ic.result);
    if (sz == 1) {
        g.store_a(ic.result);
    } else if (sz > 8) {
        g.emit_line("push\tde");
        g.emit_line("pop\thl");
        for (int w = 0; w < sz / 2; ++w) {
            g.emit_line("ld\te, (hl)");
            g.emit_line("inc\thl");
            g.emit_line("ld\td, (hl)");
            g.emit_line("inc\thl");
            g.emit_line("push\thl");
            g.emit_line("ex\tde, hl");
            g.store_hl_word(ic.result, w);
            g.emit_line("pop\thl");
        }
        if (sz & 1) {
            g.emit_line("ld\ta, (hl)");
            operand tail = ic.result;
            tail.byte_offset += (sz - 1);
            tail.type = type::make_char();
            g.store_a(tail);
        }
    } else if (sz == 8) {
        g.store_reg64(ic.result);
    } else if (sz == 4) {
        g.store_de_word(ic.result, 0);
        g.store_hl_hi32(ic.result);
    } else if (sz == 3 && ic.result.type && ic.result.type->is_far_ptr()) {
        // Far (24-bit) pointer result: HL = address, E = bank.
        emit_far_ptr_from_regs(g, ic.result);
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
    if (ic.result.is_symbol() && ic.result.byte_offset == 0 &&
        !ic.result.is_global) {
        auto sri = g.symbol_regs_.find(g.symbol_reg_key(ic.result));
        if (sri != g.symbol_regs_.end()) {
            switch (sri->second) {
            case temp_home::main_a:
                if (ic.arg_loc == abi_arg_loc::REG_A)
                    return;
                break;
            case temp_home::main_b:
                switch (ic.arg_loc) {
                case abi_arg_loc::REG_A:
                    g.emit_line("ld\tb, a");
                    return;
                case abi_arg_loc::REG_L:
                    g.emit_line("ld\tb, l");
                    return;
                default:
                    break;
                }
                break;
            case temp_home::main_c:
                switch (ic.arg_loc) {
                case abi_arg_loc::REG_A:
                    g.emit_line("ld\tc, a");
                    return;
                case abi_arg_loc::REG_L:
                    g.emit_line("ld\tc, l");
                    return;
                default:
                    break;
                }
                break;
            case temp_home::main_bc:
                switch (ic.arg_loc) {
                case abi_arg_loc::REG_HL:
                    g.emit_line("ld\tb, h");
                    g.emit_line("ld\tc, l");
                    return;
                case abi_arg_loc::REG_DE:
                    g.emit_line("ld\tb, d");
                    g.emit_line("ld\tc, e");
                    return;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
    }

    if (ic.result.is_symbol() && g.symbol_home_in_bc(ic.result) &&
        ic.result.byte_offset == 0) {
        switch (ic.arg_loc) {
        case abi_arg_loc::REG_HL:
            g.emit_line("ld\tb, h");
            g.emit_line("ld\tc, l");
            return;
        case abi_arg_loc::REG_DE:
            g.emit_line("ld\tb, d");
            g.emit_line("ld\tc, e");
            return;
        default:
            break;
        }
    }

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

void abi_convention::materialize_modern_receive(z80_gen &g, const icode &ic)
{
    if (ic.arg_loc == abi_arg_loc::STACK)
        return;

    if (ic.result.is_symbol()) {
        if (g.symbol_home_in_bc(ic.result) && ic.result.byte_offset == 0) {
            spill_modern_receive(g, ic);
            return;
        }
        spill_modern_receive(g, ic);
        return;
    }

    if (!ic.result.is_temp())
        return;

    if (ic.arg_loc == abi_arg_loc::REG_A) {
        g.temp_regs_[ic.result.temp_id] = temp_home::stack;
        g.store_frame_byte(g.ix_offset_of(ic.result), 'a');
        return;
    }

    auto temp_home_it = g.temp_regs_.find(ic.result.temp_id);
    if (temp_home_it != g.temp_regs_.end() &&
        ic.result.byte_offset == 0) {
        switch (temp_home_it->second) {
        case temp_home::main_a:
            temp_home_it->second = temp_home::stack;
            break;
        case temp_home::main_b:
            switch (ic.arg_loc) {
            case abi_arg_loc::REG_A:
                g.emit_line("ld\tb, a");
                return;
            case abi_arg_loc::REG_L:
                g.emit_line("ld\tb, l");
                return;
            default:
                break;
            }
            break;
        case temp_home::main_c:
            switch (ic.arg_loc) {
            case abi_arg_loc::REG_A:
                g.emit_line("ld\tc, a");
                return;
            case abi_arg_loc::REG_L:
                g.emit_line("ld\tc, l");
                return;
            default:
                break;
            }
            break;
        case temp_home::main_bc:
            switch (ic.arg_loc) {
            case abi_arg_loc::REG_HL:
                g.emit_line("ld\tb, h");
                g.emit_line("ld\tc, l");
                return;
            case abi_arg_loc::REG_DE:
                g.emit_line("ld\tb, d");
                g.emit_line("ld\tc, e");
                return;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }

    switch (ic.arg_loc) {
    case abi_arg_loc::REG_A:
        g.store_a(ic.result);
        break;
    case abi_arg_loc::REG_L:
        g.emit_line("ld\ta, l");
        g.store_a(ic.result);
        break;
    case abi_arg_loc::REG_HL:
        g.store_hl(ic.result);
        break;
    case abi_arg_loc::REG_DE:
        g.store_de(ic.result);
        break;
    case abi_arg_loc::REG_DEHL:
        g.store_de(ic.result);
        g.store_hl_hi32(ic.result);
        break;
    case abi_arg_loc::STACK:
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
        if (ic.arg_loc != abi_arg_loc::STACK && ic.result.is_symbol())
            spill_one(g, ic);
    }
}

void abi_convention::std_prologue_frame(z80_gen &g, const ir_function &fn)
{
    if (g.can_omit_frame_pointer(fn)) {
        g.emit_comment("frameless function: no IX frame needed");
        g.clear_known_sp_ix_delta();
        return;
    }

    bool use_shared_enter = g.shared_ix_helpers_enabled();
    if (use_shared_enter) {
        g.asm_.global_decl("__sdcc_enter_ix");
        g.emit_line("call\t__sdcc_enter_ix");
    } else {
        g.emit_line("push\tix");
        g.emit_line("ld\tix, %s", g.asm_.imm(0).c_str());
        g.emit_line("add\tix, sp");
    }
    g.set_known_sp_ix_delta(0);
    if (g.total_frame_bytes() > 0) {
        g.emit_line("ld\thl, %s", g.asm_.imm(-g.total_frame_bytes()).c_str());
        g.emit_line("add\thl, sp");
        g.emit_line("ld\tsp, hl");
        g.set_known_sp_ix_delta(-g.total_frame_bytes());
    }
}

void abi_convention::std_epilogue_frame(z80_gen &g, const ir_function &fn)
{
    g.emit_label(g.fn_end_lbl_, false);
    if (fn.is_noreturn) {
        g.emit_comment("epilogue omitted: %s is [[noreturn]]", fn.name.c_str());
    } else {
        g.emit_comment("epilogue: %s", fn.name.c_str());
        if (!g.can_omit_frame_pointer(fn)) {
            g.emit_line("ld\tsp, ix");
            g.set_known_sp_ix_delta(0);
            g.emit_line("pop\tix");
            g.clear_known_sp_ix_delta();
        }
    }
}

void abi_convention::std_send_push(z80_gen &g, const icode &ic)
{
    const int value_size = g.op_size(ic.left);
    const int sz = ic.send_bytes > 0 ? ic.send_bytes : value_size;
    auto push_byte = [&](int byte_offset) {
        operand byte = ic.left;
        byte.byte_offset += byte_offset;
        byte.type = type::make_uchar();
        g.load_a(byte);
        g.emit_line("push\taf");
        g.emit_line("inc\tsp");
    };

    if (sz == 1) {
        push_byte(0);
    } else if (sz == 2 && value_size == 1) {
        // Optimizers may narrow a range-proven value, but the callee still
        // owns the original two-byte ABI slot.
        g.load_hl(ic.left);
        g.emit_line("push\thl");
    } else if (sz > 2) {
        if (sz & 1)
            push_byte(sz - 1);
        for (int byte = (sz / 2) * 2 - 2; byte >= 0; byte -= 2) {
            g.load_hl_word(ic.left, byte / 2);
            g.emit_line("push\thl");
        }
    } else {
        if (ic.left.byte_offset == 0 && ic.left.is_temp() &&
            g.operand_home_in_bc(ic.left)) {
            const icode *def =
                g.find_temp_def_before(ic.left.temp_id, g.cur_ic_index_);
            if (def && (def->op == icode_op::EQ ||
                        def->op == icode_op::NE ||
                        def->op == icode_op::LT ||
                        def->op == icode_op::LE ||
                        def->op == icode_op::GT ||
                        def->op == icode_op::GE)) {
                g.emit_line("push\tbc");
                return;
            }
        }
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
        return sz < 1 ? 1 : sz;
    }

    void emit_prologue(z80_gen &g, const ir_function &fn) override {
        std::string lbl = g.mangle(fn.name);
        if (g.debug_) g.debug_->begin_function(fn, lbl);
        g.asm_.label(lbl, fn.is_global);
        g.emit_comment("%s prologue: %s (locals=%d, temp_frame=%d, stack_params=%d)",
                       name(), fn.name.c_str(), fn.local_bytes, g.temp_frame_bytes_,
                       fn.stack_param_bytes);
        std_prologue_frame(g, fn);
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        bool use_shared_leave =
            !fn.is_noreturn &&
            !g.can_omit_frame_pointer(fn) &&
            g.shared_ix_helpers_enabled() &&
            !(callee_repairs_stack() && fn.stack_param_bytes > 0);
        if (use_shared_leave) {
            g.emit_label(g.fn_end_lbl_, false);
            g.emit_comment("epilogue: %s", fn.name.c_str());
            g.asm_.global_decl("__sdcc_leave_ix");
            g.emit_line("jp\t__sdcc_leave_ix");
            g.clear_known_sp_ix_delta();
        } else {
            std_epilogue_frame(g, fn);
        }
        if (!fn.is_noreturn) {
            if (callee_repairs_stack() && fn.stack_param_bytes > 0)
                callee_stack_return(
                    g, fn.stack_param_bytes,
                    fn.ret_type && fn.ret_type->kind == type_kind::VOID);
            else if (!use_shared_leave)
                g.emit_line("ret");
        }
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        if (ic.arg_loc == abi_arg_loc::STACK &&
            ic.result.is_symbol() && ic.result.byte_offset == 0 &&
            g.symbol_home_in_iy(ic.result)) {
            g.emit_comment("keep stack parameter %s live in IY",
                           ic.result.name.c_str());
            g.load_frame_word(
                z80_gen::reg_pair{"hl", 'l', 'h', false},
                g.ix_offset_of(ic.result));
            g.emit_line("push\thl");
            g.emit_line("pop\tiy");
            return;
        }
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

struct cc_z88dk_smallc final : stack_linkage_convention {
    call_abi abi_tag() const override { return call_abi::Z88DK_SMALLC; }
    const char *name() const override { return "z88dk smallc"; }
    bool caller_sends_right_to_left() const override { return false; }

    void emit_send(z80_gen &g, const icode &ic) override {
        if (g.op_size(ic.left) == 1) {
            g.load_a(ic.left);
            g.emit_line("ld\tl, a");
            g.emit_line("ld\th, %s", g.asm_.imm(0).c_str());
            g.emit_line("push\thl");
            return;
        }
        std_send_push(g, ic);
    }

    void emit_return_value(z80_gen &g, const operand &value) const override {
        emit_modern_return_value(g, value);
    }

    void emit_store_call_result(z80_gen &g, const icode &ic) const override {
        emit_store_modern_result(g, ic);
    }
};

// ─── z88dk fastcall ──────────────────────────────────────────────────────────

struct cc_z88dk_fastcall final : abi_convention {
    call_abi abi_tag() const override { return call_abi::Z88DK_FASTCALL; }

    std::vector<abi_arg_loc>
    classify_args(const std::vector<type_ptr> &types) const override {
        std::vector<abi_arg_loc> locs(types.size(), abi_arg_loc::STACK);
        if (types.empty()) return locs;

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
        g.emit_comment("z88dk fastcall prologue: %s (locals=%d, temp_frame=%d, stack_params=%d)",
                       fn.name.c_str(), fn.local_bytes, g.temp_frame_bytes_,
                       fn.stack_param_bytes);
        if (g.can_omit_frame_pointer(fn)) {
            g.emit_comment("frameless function: no IX frame needed");
            g.clear_known_sp_ix_delta();
        } else {
            bool use_shared_enter = g.shared_ix_helpers_enabled();
            if (use_shared_enter) {
                g.asm_.global_decl("__sdcc_enter_ix");
                g.emit_line("call\t__sdcc_enter_ix");
            } else {
                g.emit_line("push\tix");
                g.emit_line("ld\tix, %s", g.asm_.imm(0).c_str());
                g.emit_line("add\tix, sp");
            }
            g.set_known_sp_ix_delta(0);
            spill_leading_receives(g, fn, spill_fastcall_receive);
            if (g.total_frame_bytes() > 0) {
                g.emit_line("ld\thl, %s", g.asm_.imm(-g.total_frame_bytes()).c_str());
                g.emit_line("add\thl, sp");
                g.emit_line("ld\tsp, hl");
                g.set_known_sp_ix_delta(-g.total_frame_bytes());
            }
        }
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        if (!fn.is_noreturn &&
            !g.can_omit_frame_pointer(fn) &&
            g.shared_ix_helpers_enabled()) {
            g.emit_label(g.fn_end_lbl_, false);
            g.emit_comment("epilogue: %s", fn.name.c_str());
            g.asm_.global_decl("__sdcc_leave_ix");
            g.emit_line("jp\t__sdcc_leave_ix");
            g.clear_known_sp_ix_delta();
        } else {
            std_epilogue_frame(g, fn);
        }
        if (!fn.is_noreturn &&
            (g.can_omit_frame_pointer(fn) || !g.shared_ix_helpers_enabled()))
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
            g.load_hl_lo32(ic.left);
            g.load_de_word(ic.left, 1);
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
        emit_bc_indirect_call(g, ic.left, false, true, true);
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
        g.emit_comment("sdcccall(1) prologue: %s (locals=%d, temp_frame=%d, stack_params=%d)",
                       fn.name.c_str(), fn.local_bytes, g.temp_frame_bytes_,
                       fn.stack_param_bytes);
        bool frameless = g.can_omit_frame_pointer(fn);
        if (frameless) {
            g.emit_comment("frameless function: no IX frame needed");
            g.clear_known_sp_ix_delta();
        } else {
            bool use_shared_enter = g.shared_ix_helpers_enabled();
            if (use_shared_enter) {
                g.asm_.global_decl("__sdcc_enter_ix");
                g.emit_line("call\t__sdcc_enter_ix");
            } else {
                g.emit_line("push\tix");
                g.emit_line("ld\tix, %s", g.asm_.imm(0).c_str());
                g.emit_line("add\tix, sp");
            }
            g.set_known_sp_ix_delta(0);
            const bool retain_incoming_regs = g.opt_settings_.regalloc;
            std::unordered_map<size_t, temp_home> retained;
            bool retain_hl_like = false;
            bool retain_de = false;
            bool retain_bc = false;
            if (retain_incoming_regs) {
                for (size_t i = 1; i < fn.icodes.size(); ++i) {
                    const auto &ic = fn.icodes[i];
                    if (ic.op != icode_op::RECEIVE)
                        break;
                    temp_home preferred_home = incoming_arg_home(ic.arg_loc);
                    if (ic.result.is_temp()) {
                        auto ti = g.temp_regs_.find(ic.result.temp_id);
                        if (ti != g.temp_regs_.end() &&
                            ti->second == temp_home::main_iy) {
                            // The early-materialization pass below must see
                            // this receive before frame allocation; retaining
                            // its ABI register here would hide the IY home.
                            continue;
                        }
                        if (ti != g.temp_regs_.end() &&
                            ti->second == temp_home::main_bc) {
                            preferred_home = temp_home::main_bc;
                        }
                    } else if (ic.result.is_symbol() &&
                               g.symbol_home_in_bc(ic.result) &&
                               ic.result.byte_offset == 0) {
                        preferred_home = temp_home::main_bc;
                    }
                    if (!should_keep_modern_receive_in_register(fn, i, ic))
                        continue;
                    retained[i] = preferred_home;
                    retain_hl_like = retain_hl_like ||
                        preferred_home == temp_home::arg_hl ||
                        preferred_home == temp_home::arg_l;
                    retain_de = retain_de || preferred_home == temp_home::arg_de;
                    retain_bc = retain_bc || preferred_home == temp_home::main_bc;
                }
                if (retain_hl_like && (retain_de || retain_bc)) {
                    for (auto it = retained.begin(); it != retained.end();) {
                        if (it->second == temp_home::arg_hl ||
                            it->second == temp_home::arg_l) {
                            it = retained.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    retain_hl_like = false;
                }
            }
            // Materializing an incoming pointer directly in IY uses
            // push rr/pop iy.  Do that before spilling any other incoming
            // register to an IX-relative slot: the frame has not been
            // allocated yet, so a later push would overwrite slots just
            // written below SP (notably HL followed by DE->IY).
            std::unordered_set<size_t> early_materialized;
            for (size_t i = 1; i < fn.icodes.size(); ++i) {
                const auto &ic = fn.icodes[i];
                if (ic.op != icode_op::RECEIVE)
                    break;
                if (!ic.result.is_temp() || retained.count(i) != 0)
                    continue;
                auto home = g.temp_regs_.find(ic.result.temp_id);
                if (home == g.temp_regs_.end() ||
                    home->second != temp_home::main_iy)
                    continue;
                materialize_modern_receive(g, ic);
                early_materialized.insert(i);
            }
            for (size_t i = 1; i < fn.icodes.size(); ++i) {
                const auto &ic = fn.icodes[i];
                if (ic.op != icode_op::RECEIVE)
                    break;
                if (early_materialized.count(i) != 0)
                    continue;
                auto keep_it = retained.find(i);
                if (keep_it != retained.end()) {
                    if (ic.result.is_temp()) {
                        if (keep_it->second == temp_home::main_bc) {
                            g.emit_comment("keep incoming register arg t%d live in BC for first use",
                                           ic.result.temp_id);
                            switch (ic.arg_loc) {
                            case abi_arg_loc::REG_HL:
                                g.emit_line("ld\tb, h");
                                g.emit_line("ld\tc, l");
                                break;
                            case abi_arg_loc::REG_DE:
                                g.emit_line("ld\tb, d");
                                g.emit_line("ld\tc, e");
                                break;
                            default:
                                materialize_modern_receive(g, ic);
                                continue;
                            }
                            g.temp_regs_[ic.result.temp_id] = temp_home::main_bc;
                        } else {
                            g.emit_comment("keep incoming register arg t%d live in register for first use",
                                           ic.result.temp_id);
                            g.temp_regs_[ic.result.temp_id] = keep_it->second;
                        }
                    } else {
                        if (keep_it->second == temp_home::main_bc) {
                            g.emit_comment("keep incoming register arg %s live in BC for first use",
                                           ic.result.name.c_str());
                            switch (ic.arg_loc) {
                            case abi_arg_loc::REG_HL:
                                g.emit_line("ld\tb, h");
                                g.emit_line("ld\tc, l");
                                break;
                            case abi_arg_loc::REG_DE:
                                g.emit_line("ld\tb, d");
                                g.emit_line("ld\tc, e");
                                break;
                            default:
                                materialize_modern_receive(g, ic);
                                continue;
                            }
                        } else {
                            g.emit_comment("keep incoming register arg %s live in register for first use",
                                           ic.result.name.c_str());
                            g.incoming_symbol_homes_[ic.result.stack_offset] = keep_it->second;
                        }
                    }
                    continue;
                }
                materialize_modern_receive(g, ic);
            }
            if (g.total_frame_bytes() > 0) {
                if (retain_hl_like) {
                    g.emit_line("ex\tde, hl");
                    g.emit_line("ld\thl, %s", g.asm_.imm(-g.total_frame_bytes()).c_str());
                    g.emit_line("add\thl, sp");
                    g.emit_line("ld\tsp, hl");
                    g.set_known_sp_ix_delta(-g.total_frame_bytes());
                    g.emit_line("ex\tde, hl");
                } else {
                    g.emit_line("ld\thl, %s", g.asm_.imm(-g.total_frame_bytes()).c_str());
                    g.emit_line("add\thl, sp");
                    g.emit_line("ld\tsp, hl");
                    g.set_known_sp_ix_delta(-g.total_frame_bytes());
                }
            }
        }
    }

    void emit_epilogue(z80_gen &g, const ir_function &fn) override {
        bool use_shared_leave =
            !fn.is_noreturn &&
            !g.can_omit_frame_pointer(fn) &&
            g.shared_ix_helpers_enabled() &&
            !(fn.callee_cleans_stack && fn.stack_param_bytes > 0);
        if (use_shared_leave) {
            g.emit_label(g.fn_end_lbl_, false);
            g.emit_comment("epilogue: %s", fn.name.c_str());
            g.asm_.global_decl("__sdcc_leave_ix");
            g.emit_line("jp\t__sdcc_leave_ix");
            g.clear_known_sp_ix_delta();
        } else {
            std_epilogue_frame(g, fn);
        }
        if (!fn.is_noreturn) {
            if (fn.callee_cleans_stack && fn.stack_param_bytes > 0)
                callee_stack_return(
                    g, fn.stack_param_bytes,
                    !fn.ret_type || fn.ret_type->kind == type_kind::VOID ||
                        fn.ret_type->size() <= 2);
            else if (!use_shared_leave)
                g.emit_line("ret");
        }
        if (g.debug_) g.debug_->end_function(fn);
    }

    void emit_receive(z80_gen &g, const icode &ic) override {
        if (ic.arg_loc != abi_arg_loc::STACK && ic.result.is_temp()) {
            if (g.cur_fn_ && !g.can_omit_frame_pointer(*g.cur_fn_)) {
                g.emit_comment("receive (sdcccall1) register param handled by prologue");
                return;
            }
            bool safe_frameless_single_use = false;
            if (g.cur_fn_) {
                size_t first_use =
                    first_temp_use_after(*g.cur_fn_, g.cur_ic_index_,
                                         ic.result.temp_id);
                if (first_use < g.cur_fn_->icodes.size() &&
                    !g.temp_value_used_after(*g.cur_fn_, first_use + 1,
                                             ic.result.temp_id)) {
                    safe_frameless_single_use = true;
                    for (size_t i = g.cur_ic_index_ + 1; i < first_use; ++i) {
                        if (g.cur_fn_->icodes[i].op == icode_op::LABEL) {
                            safe_frameless_single_use = false;
                            break;
                        }
                    }
                }
            }
            if (safe_frameless_single_use) {
                temp_home preferred_home = incoming_arg_home(ic.arg_loc);
                auto ti = g.temp_regs_.find(ic.result.temp_id);
                if (ti != g.temp_regs_.end() &&
                    ti->second == temp_home::main_bc) {
                    preferred_home = temp_home::main_bc;
                }

                if (preferred_home == temp_home::main_bc) {
                    g.emit_comment("keep incoming register arg t%d live in BC for first use",
                                   ic.result.temp_id);
                    switch (ic.arg_loc) {
                    case abi_arg_loc::REG_HL:
                        g.emit_line("ld\tb, h");
                        g.emit_line("ld\tc, l");
                        g.temp_regs_[ic.result.temp_id] = temp_home::main_bc;
                        return;
                    case abi_arg_loc::REG_DE:
                        g.emit_line("ld\tb, d");
                        g.emit_line("ld\tc, e");
                        g.temp_regs_[ic.result.temp_id] = temp_home::main_bc;
                        return;
                    default:
                        break;
                    }
                } else if (preferred_home != temp_home::stack) {
                    g.emit_comment("keep incoming register arg t%d live in register for first use",
                                   ic.result.temp_id);
                    g.temp_regs_[ic.result.temp_id] = preferred_home;
                    return;
                }
            }
            materialize_modern_receive(g, ic);
            return;
        }
        g.emit_comment("receive (sdcccall1) param %s at %s",
                       ic.result.name.c_str(), g.addr_of(ic.result).c_str());
    }

    void emit_send(z80_gen &g, const icode &ic) override {
        switch (ic.arg_loc) {
        case abi_arg_loc::REG_A:
            // SENDs are emitted right-to-left. For sdcccall(1), a 2nd 8-bit
            // argument will already be live in L when we materialize arg0
            // into A. Deep stack/temp byte loads use HL as an address scratch,
            // so preserve HL here to avoid clobbering that earlier byte arg.
            if (ic.argreg == 0) {
                g.emit_line("push\thl");
                g.load_a(ic.left);
                g.emit_line("pop\thl");
            } else {
                g.load_a(ic.left);
            }
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
            g.load_de_word(ic.left, 0);
            g.load_hl_hi32(ic.left);
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
        if (!ic.callee_cleans_stack)
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
    static cc_z88dk_smallc   z88dk_smallc;
    static cc_z88dk_fastcall z88dk_fastcall;
    static cc_z88dk_callee   z88dk_callee;
    static cc_naked          naked;
    static cc_interrupt      interrupt;
    static cc_critical       critical;

    switch (effective_call_abi(abi)) {
    case call_abi::SDCCCALL0:      return sdcccall0;
    case call_abi::SDCCCALL1:      return sdcccall1;
    case call_abi::Z88DK_SMALLC:   return z88dk_smallc;
    case call_abi::Z88DK_FASTCALL: return z88dk_fastcall;
    case call_abi::Z88DK_CALLEE:   return z88dk_callee;
    case call_abi::NAKED:          return naked;
    case call_abi::INTERRUPT:      return interrupt;
    case call_abi::CRITICAL:       return critical;
    default:                       return sdcccall1;
    }
}

} // namespace xcc
