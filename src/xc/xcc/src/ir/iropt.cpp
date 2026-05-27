//
// iropt.cpp — IR-level optimizer for xcc.
//
// Three passes run to fixed-point:
//
//   constant_fold:  binary/unary ops with all-constant operands → ASSIGN(result, folded_const)
//   copy_prop:      ASSIGN(t_dst, t_src) where t_src is TEMP → replace t_dst uses with t_src
//   dead_code_elim: pure instructions with result TEMP, zero uses → deleted
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/iropt.h"
#include <unordered_map>
#include <algorithm>

namespace xcc {

// ── Helpers ───────────────────────────────────────────────────────────────────

bool ir_optimizer::is_pure(icode_op op) {
    switch (op) {
    case icode_op::ASSIGN:
    case icode_op::ADD:  case icode_op::SUB:  case icode_op::NEG:
    case icode_op::MUL:  case icode_op::DIV:  case icode_op::MOD:
    case icode_op::BAND: case icode_op::BOR:  case icode_op::BXOR: case icode_op::BNOT:
    case icode_op::SHL:  case icode_op::SHR:
    case icode_op::EQ:   case icode_op::NE:
    case icode_op::LT:   case icode_op::LE:
    case icode_op::GT:   case icode_op::GE:
    case icode_op::CAST:
    case icode_op::ADDRESS_OF:
        return true;
    default:
        return false;
    }
}

// ── constant_fold ─────────────────────────────────────────────────────────────

static int64_t fold_binary(icode_op op, int64_t l, int64_t r) {
    switch (op) {
    case icode_op::ADD:  return l + r;
    case icode_op::SUB:  return l - r;
    case icode_op::MUL:  return l * r;
    case icode_op::DIV:  return r ? l / r : 0;
    case icode_op::MOD:  return r ? l % r : 0;
    case icode_op::BAND: return l & r;
    case icode_op::BOR:  return l | r;
    case icode_op::BXOR: return l ^ r;
    case icode_op::SHL:  return l << (r & 63);
    case icode_op::SHR:  return l >> (r & 63);
    case icode_op::EQ:   return l == r ? 1 : 0;
    case icode_op::NE:   return l != r ? 1 : 0;
    case icode_op::LT:   return l <  r ? 1 : 0;
    case icode_op::LE:   return l <= r ? 1 : 0;
    case icode_op::GT:   return l >  r ? 1 : 0;
    case icode_op::GE:   return l >= r ? 1 : 0;
    default:             return 0;
    }
}

static bool is_binary_foldable(icode_op op) {
    switch (op) {
    case icode_op::ADD: case icode_op::SUB: case icode_op::MUL:
    case icode_op::DIV: case icode_op::MOD:
    case icode_op::BAND: case icode_op::BOR: case icode_op::BXOR:
    case icode_op::SHL: case icode_op::SHR:
    case icode_op::EQ: case icode_op::NE:
    case icode_op::LT: case icode_op::LE: case icode_op::GT: case icode_op::GE:
        return true;
    default: return false;
    }
}

bool ir_optimizer::constant_fold(ir_function &fn) {
    bool changed = false;
    for (auto &ic : fn.icodes) {
        // Binary: both operands are INT_CONST
        if (is_binary_foldable(ic.op) &&
            ic.left.kind  == operand_kind::INT_CONST &&
            ic.right.kind == operand_kind::INT_CONST) {
            int64_t v = fold_binary(ic.op, ic.left.ival, ic.right.ival);
            ic.op    = icode_op::ASSIGN;
            ic.left  = operand::make_int(v, ic.result.type);
            ic.right = operand::make_none();
            changed  = true;
        }
        // Unary NEG on constant
        else if (ic.op == icode_op::NEG &&
                 ic.left.kind == operand_kind::INT_CONST) {
            ic.op   = icode_op::ASSIGN;
            ic.left = operand::make_int(-ic.left.ival, ic.result.type);
            changed = true;
        }
        // Unary BNOT on constant
        else if (ic.op == icode_op::BNOT &&
                 ic.left.kind == operand_kind::INT_CONST) {
            ic.op   = icode_op::ASSIGN;
            ic.left = operand::make_int(~ic.left.ival, ic.result.type);
            changed = true;
        }
        // CAST of constant: truncate to target size
        else if (ic.op == icode_op::CAST &&
                 ic.left.kind == operand_kind::INT_CONST &&
                 ic.result.type) {
            int64_t v = ic.left.ival;
            switch (ic.result.type->size()) {
            case 1: v = (int8_t) (v & 0xFF);       break;
            case 2: v = (int16_t)(v & 0xFFFF);     break;
            case 4: v = (int32_t)(v & 0xFFFFFFFFLL); break;
            default: break;
            }
            ic.op   = icode_op::ASSIGN;
            ic.left = operand::make_int(v, ic.result.type);
            changed = true;
        }
    }
    return changed;
}

// ── copy_prop ─────────────────────────────────────────────────────────────────

bool ir_optimizer::copy_prop(ir_function &fn) {
    bool changed = false;
    for (size_t i = 0; i < fn.icodes.size(); ++i) {
        icode &def = fn.icodes[i];
        // Look for ASSIGN(t_dst, src) where src is a TEMP or INT_CONST.
        // Propagating INT_CONST lets constant-folded values bypass the temp
        // entirely, enabling downstream DCE to remove the defining instruction.
        if (def.op != icode_op::ASSIGN) continue;
        if (!def.result.is_temp())      continue;
        if (!def.left.is_temp() && def.left.kind != operand_kind::INT_CONST) continue;

        int t_dst = def.result.temp_id;
        int t_src = def.left.is_temp() ? def.left.temp_id : -1;
        if (t_src == t_dst) continue;

        // Propagate forward until t_src or t_dst is redefined
        for (size_t j = i + 1; j < fn.icodes.size(); ++j) {
            icode &ic = fn.icodes[j];

            // Stop if t_src (when TEMP) or t_dst is redefined
            if (ic.result.is_temp() &&
                (ic.result.temp_id == t_dst || (t_src >= 0 && ic.result.temp_id == t_src)))
                break;

            // Replace uses of t_dst with the propagated source.
            // For INT_CONST sources, preserve the use operand's type so that
            // sizing decisions (1-byte vs 2-byte) remain correct.
            auto replace = [&](operand &op) {
                if (op.is_temp() && op.temp_id == t_dst) {
                    type_ptr orig_type = op.type;
                    op = def.left;
                    if (orig_type) op.type = orig_type;
                    changed = true;
                }
            };
            replace(ic.left);
            replace(ic.right);
        }
    }
    return changed;
}

// ── dead_code_elim ────────────────────────────────────────────────────────────

bool ir_optimizer::dead_code_elim(ir_function &fn) {
    // Count uses of each temp_id across the entire function
    std::unordered_map<int, int> uses;
    for (auto &ic : fn.icodes) {
        auto count = [&](const operand &op) {
            if (op.is_temp()) uses[op.temp_id]++;
        };
        count(ic.left);
        count(ic.right);
    }

    bool changed = false;
    for (auto it = fn.icodes.begin(); it != fn.icodes.end(); ) {
        const icode &ic = *it;
        if (ic.result.is_temp() &&
            is_pure(ic.op) &&
            uses[ic.result.temp_id] == 0) {
            // This instruction's output is never read and has no side effects.
            // Removing it may expose new dead temps (their uses drop to 0),
            // but we rely on the outer fixed-point loop for that.
            it      = fn.icodes.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    return changed;
}

// ── algebraic_simplify ────────────────────────────────────────────────────────

bool ir_optimizer::algebraic_simplify(ir_function &fn) {
    bool changed = false;
    for (auto &ic : fn.icodes) {
        // Only handle binary ops with at least one INT_CONST operand.
        const bool left_const  = (ic.left.kind  == operand_kind::INT_CONST);
        const bool right_const = (ic.right.kind == operand_kind::INT_CONST);
        if (!left_const && !right_const) continue;

        // Helpers to access the constant side and the variable side.
        int64_t cval = left_const ? ic.left.ival : ic.right.ival;

        auto replace_with = [&](operand src) {
            ic.op    = icode_op::ASSIGN;
            ic.left  = src;
            ic.right = operand::make_none();
            changed  = true;
        };
        auto replace_with_zero = [&]() {
            replace_with(operand::make_int(0, ic.result.type));
        };
        auto replace_with_var = [&]() {
            replace_with(left_const ? ic.right : ic.left);
        };

        switch (ic.op) {
        case icode_op::ADD:
            if (cval == 0) replace_with_var();  // x + 0, 0 + x → x
            break;
        case icode_op::SUB:
            if (right_const && cval == 0) replace_with_var();  // x - 0 → x
            break;
        case icode_op::MUL:
            if (cval == 0) replace_with_zero();                // x * 0 → 0
            else if (cval == 1) replace_with_var();            // x * 1 → x
            break;
        case icode_op::DIV:
            if (right_const && cval == 1) replace_with_var();  // x / 1 → x
            break;
        case icode_op::BAND:
            if (cval == 0) replace_with_zero();                // x & 0 → 0
            else if (cval == -1 || cval == 0xFFFF || cval == 0xFF)
                replace_with_var();                            // x & all-ones → x
            break;
        case icode_op::BOR:
            if (cval == 0) replace_with_var();                 // x | 0 → x
            break;
        case icode_op::BXOR:
            if (cval == 0) replace_with_var();                 // x ^ 0 → x
            break;
        case icode_op::SHL:
        case icode_op::SHR:
            if (right_const && cval == 0) replace_with_var(); // x <</>></> 0 → x
            break;
        default:
            break;
        }
    }
    return changed;
}

// ── strength_reduce ───────────────────────────────────────────────────────────

// Return log2(v) if v is a power of two > 0, else -1.
static int log2_exact(int64_t v) {
    if (v <= 0 || (v & (v - 1)) != 0) return -1;
    int n = 0;
    while (v > 1) { v >>= 1; ++n; }
    return n;
}

bool ir_optimizer::strength_reduce(ir_function &fn) {
    bool changed = false;
    for (auto &ic : fn.icodes) {
        if (ic.right.kind != operand_kind::INT_CONST) continue;
        int64_t rval = ic.right.ival;
        int shift = log2_exact(rval);

        if (ic.op == icode_op::MUL && shift >= 0) {
            // x * 2^n  →  x << n
            ic.op    = icode_op::SHL;
            ic.right = operand::make_int(shift, ic.right.type);
            changed  = true;
        } else if (ic.op == icode_op::DIV && shift >= 0 &&
                   ic.left.type && ic.left.type->is_unsigned()) {
            // unsigned x / 2^n  →  x >> n
            ic.op    = icode_op::SHR;
            ic.right = operand::make_int(shift, ic.right.type);
            changed  = true;
        } else if (ic.op == icode_op::MOD && shift >= 0 &&
                   ic.left.type && ic.left.type->is_unsigned()) {
            // unsigned x % 2^n  →  x & (2^n - 1)
            ic.op    = icode_op::BAND;
            ic.right = operand::make_int(rval - 1, ic.right.type);
            changed  = true;
        }
    }
    return changed;
}

// ── optimize ─────────────────────────────────────────────────────────────────

void ir_optimizer::optimize(ir_function &fn) {
    bool changed;
    do {
        changed  = constant_fold(fn);
        changed |= algebraic_simplify(fn);
        changed |= copy_prop(fn);
        changed |= dead_code_elim(fn);
        changed |= strength_reduce(fn);
    } while (changed);
}

} // namespace xcc
