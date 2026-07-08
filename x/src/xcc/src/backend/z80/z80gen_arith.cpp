//
// z80gen_arith.cpp — arithmetic, bitwise, compare, cast, and float
//                    instruction generation for the Z80 backend.
//
// All methods are members of z80_gen and use the shared helpers
// (emit_line, load_hl, store_hl, etc.) defined in z80gen.cpp.
//
// Runtime helpers called here (defined in lib/runtime/):
//   __mul16, __mul32, __mulll
//   __mulschar, __muluschar, __mulsuchar
//   __div16, __div32, __divll
//   __divschar, __divsuchar, __divuschar
//   __modschar, __modsuchar, __moduschar
//   __mod16, __mod32, __modll
//   __fsadd, __fssub, __fsmul, __fsdiv, __fitosf, __fstoi
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"

namespace xcc {

// Return true only for actual 64-bit integer types (llong / ullong).
static bool is_llong_op(const operand &op) {
    return op.type && (op.type->kind == type_kind::LLONG ||
                       op.type->kind == type_kind::ULLONG);
}

static bool is_float32_op(const operand &op) {
    return op.type && op.type->kind == type_kind::FLOAT &&
           get_float_format() == float_format::IEEE32 &&
           op.type->size() == 4;
}

static bool is_float16_op(const operand &op) {
    return op.type && op.type->kind == type_kind::FLOAT &&
           get_float_format() == float_format::IEEE16 &&
           op.type->size() == 2;
}

static bool same_call_result_operand(const operand &a, const operand &b) {
    if (a.kind != b.kind)
        return false;
    switch (a.kind) {
    case operand_kind::TEMP:
        return a.temp_id == b.temp_id;
    case operand_kind::SYMBOL:
        return a.name == b.name &&
               a.stack_offset == b.stack_offset &&
               a.byte_offset == b.byte_offset &&
               a.is_global == b.is_global;
    default:
        return false;
    }
}

static bool equivalent_operands(const operand &a, const operand &b) {
    if (a.kind != b.kind ||
        a.byte_offset != b.byte_offset ||
        a.is_global != b.is_global ||
        a.is_param != b.is_param ||
        a.is_func != b.is_func ||
        a.is_tls != b.is_tls ||
        a.is_sfr != b.is_sfr ||
        a.sfr_port != b.sfr_port ||
        a.stack_offset != b.stack_offset ||
        a.temp_id != b.temp_id ||
        a.name != b.name) {
        return false;
    }

    switch (a.kind) {
    case operand_kind::NONE:
    case operand_kind::TEMP:
    case operand_kind::SYMBOL:
    case operand_kind::LABEL_REF:
        return true;
    case operand_kind::INT_CONST:
        return a.ival == b.ival;
    case operand_kind::FLOAT_CONST:
        return a.fval == b.fval;
    }
    return false;
}

static bool is_divmod_fusion_barrier(const icode &ic) {
    switch (ic.op) {
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::CALL:
    case icode_op::RETURN:
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
        return true;
    default:
        return false;
    }
}

static bool is_truth_test_preserving_integer_cast(const icode &ic) {
    if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
        return false;
    if (ic.left.type->is_far_ptr() || ic.result.type->is_far_ptr())
        return false;
    const bool src_ok = ic.left.type->is_integer() || ic.left.type->is_ptr();
    const bool dst_ok = ic.result.type->is_integer() || ic.result.type->is_ptr();
    return src_ok && dst_ok;
}

static bool is_fixed_format(float_format format) {
    switch (format) {
    case float_format::FIXED8_8:
    case float_format::FIXED16_16:
    case float_format::FIXED24_8:
        return true;
    case float_format::IEEE16:
    case float_format::IEEE32:
        return false;
    }
    return false;
}

static bool is_fixed_float_op(const operand &op) {
    return op.type && op.type->kind == type_kind::FLOAT &&
           is_fixed_format(get_float_format());
}

static bool is_fixed_float_type(type_ptr type) {
    return type && type->kind == type_kind::FLOAT &&
           is_fixed_format(get_float_format());
}

static bool is_double64_op(const operand &op) {
    return op.type && op.type->kind == type_kind::DOUBLE && op.type->size() == 8;
}

static bool is_real_float_op(const operand &op) {
    return is_float16_op(op) || is_float32_op(op) || is_double64_op(op);
}

static const char *fixed_helper_prefix() {
    switch (get_float_format()) {
    case float_format::FIXED8_8:   return "_fixed8_8";
    case float_format::FIXED16_16: return "_fixed16_16";
    case float_format::FIXED24_8:  return "_fixed24_8";
    case float_format::IEEE16:
    case float_format::IEEE32:
        return "";
    }
    return "";
}

static std::string fixed_helper_name(const char *suffix) {
    return std::string(fixed_helper_prefix()) + suffix;
}

static bool is_matching_shift_op(const icode &ic, bool right, bool arithmetic) {
    if (ic.op == icode_op::SHL)
        return !right;
    if (ic.op != icode_op::SHR || !right)
        return false;
    const bool ic_is_arithmetic = !(ic.left.type && ic.left.type->is_unsigned());
    return ic_is_arithmetic == arithmetic;
}

static int count_variable_shift_sites(const ir_function *fn,
                                      bool right,
                                      bool arithmetic) {
    if (!fn)
        return 0;
    int count = 0;
    for (const auto &ic : fn->icodes) {
        if (!is_matching_shift_op(ic, right, arithmetic))
            continue;
        if (ic.right.kind == operand_kind::INT_CONST)
            continue;
        ++count;
    }
    return count;
}

static bool load_word_into_de_preserves_hl(
    const operand &op,
    const std::unordered_map<int, temp_home> &temp_regs) {
    if (op.kind == operand_kind::INT_CONST ||
        op.kind == operand_kind::LABEL_REF)
        return true;

    if (op.kind == operand_kind::TEMP) {
        auto it = temp_regs.find(op.temp_id);
        return it != temp_regs.end() && it->second == temp_home::main_bc;
    }

    return false;
}

static bool fits_u8_compare_operand(const operand &op) {
    if (op.kind == operand_kind::INT_CONST)
        return op.ival >= 0 && op.ival <= 0xff;
    return op.type && op.type->size() == 1 && op.type->is_unsigned();
}

static bool fits_s8_compare_operand(const operand &op) {
    if (op.kind == operand_kind::INT_CONST)
        return op.ival >= -128 && op.ival <= 127;
    return op.type && op.type->size() == 1 && !op.type->is_unsigned();
}

static bool can_use_unsigned_byte_compare(const icode &ic) {
    return fits_u8_compare_operand(ic.left) &&
           fits_u8_compare_operand(ic.right);
}

static bool is_signed_byte_value_operand(const operand &op) {
    return op.kind != operand_kind::INT_CONST &&
           op.type && op.type->size() == 1 && !op.type->is_unsigned();
}

static bool can_use_signed_byte_const_compare(const icode &ic) {
    if (ic.left.kind == operand_kind::INT_CONST &&
        ic.right.kind == operand_kind::INT_CONST)
        return false;
    if (is_signed_byte_value_operand(ic.left) &&
        ic.right.kind == operand_kind::INT_CONST)
        return fits_s8_compare_operand(ic.right);
    if (ic.left.kind == operand_kind::INT_CONST &&
        is_signed_byte_value_operand(ic.right))
        return fits_s8_compare_operand(ic.left);
    return false;
}

static bool is_byte_value_operand(const operand &op) {
    return op.kind != operand_kind::INT_CONST &&
           op.type && op.type->size() == 1;
}

static bool fits_direct_eq_ne_byte_const(const operand &value_op,
                                         const operand &const_op) {
    if (const_op.kind != operand_kind::INT_CONST || !value_op.type ||
        value_op.type->size() != 1)
        return false;

    if (value_op.type->is_unsigned())
        return const_op.ival >= 0 && const_op.ival <= 0xff;

    return const_op.ival >= -128 && const_op.ival <= 127;
}

static bool can_use_direct_byte_eq_ne_compare(const icode &ic) {
    if (ic.left.kind == operand_kind::INT_CONST &&
        ic.right.kind == operand_kind::INT_CONST)
        return false;

    if (is_byte_value_operand(ic.left) && is_byte_value_operand(ic.right)) {
        return ic.left.type && ic.right.type &&
               ic.left.type->is_unsigned() == ic.right.type->is_unsigned();
    }

    if (is_byte_value_operand(ic.left) && ic.right.kind == operand_kind::INT_CONST)
        return fits_direct_eq_ne_byte_const(ic.left, ic.right);

    if (ic.left.kind == operand_kind::INT_CONST && is_byte_value_operand(ic.right))
        return fits_direct_eq_ne_byte_const(ic.right, ic.left);

    return false;
}

static bool is_straight_line_helper_codegen_fn(const ir_function *fn) {
    if (!fn || fn->local_bytes != 0)
        return false;
    for (size_t i = 1; i + 1 < fn->icodes.size(); ++i) {
        switch (fn->icodes[i].op) {
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

// ----- far (24-bit banked) pointer arithmetic ------------------------
//
// result(far,3) = farptr(3) +/- index(int,16).  The index is treated as
// a signed 16-bit value sign-extended to 24 bits, so the bank byte gets
// the carry/borrow and crossing a 64K boundary works like a flat 24-bit
// pointer.  (The target's trampoline decides how a bank maps to memory.)
bool z80_gen::gen_far_ptr_arith(const icode &ic, bool is_add) {
    if (!ic.result.type || !ic.result.type->is_far_ptr())
        return false;

    // Identify the far-pointer operand (always present; the other is the
    // integer index already scaled by the element size in the IR).
    const operand *ptr = nullptr;
    const operand *idx = nullptr;
    if (ic.left.type && ic.left.type->is_far_ptr()) { ptr = &ic.left;  idx = &ic.right; }
    else if (ic.right.type && ic.right.type->is_far_ptr()) { ptr = &ic.right; idx = &ic.left; }
    else return false;

    invalidate_a_cache();
    invalidate_pair_cache();

    // DE = index, HL = address, A = bank (push/pop to survive helper clobbers).
    load_de(*idx);
    load_hl_word(*ptr, 0);
    emit_line("push\thl");
    emit_line("push\tde");
    load_far_bank(*ptr);
    emit_line("pop\tde");
    emit_line("pop\thl");

    std::string nofix = fresh_local_label("__far_nofix");
    if (is_add) {
        emit_line("add\thl, de");     // HL = addr + index ; CF = carry
        emit_line("adc\ta, %s", asm_.imm(0).c_str());
        emit_line("bit\t7, d");       // index negative? -> subtract 1 (sign-extend)
        emit_line("jr\tz, %s", nofix.c_str());
        emit_line("dec\ta");
    } else {
        emit_line("or\ta, a");        // clear carry
        emit_line("sbc\thl, de");     // HL = addr - index ; CF = borrow
        emit_line("sbc\ta, %s", asm_.imm(0).c_str());
        emit_line("bit\t7, d");       // index negative? subtracting it adds 1
        emit_line("jr\tz, %s", nofix.c_str());
        emit_line("inc\ta");
    }
    emit_label(nofix, false);

    // Store bank (A) then address (HL) into the 3-byte far result.
    store_far_bank(ic.result);
    store_hl_word(ic.result, 0);
    invalidate_a_cache();
    invalidate_pair_cache();
    return true;
}

void z80_gen::gen_add(const icode &ic) {
    if (gen_far_ptr_arith(ic, /*is_add=*/true))
        return;
    if (!debug_ &&
        cur_fn_ &&
        ic.result.is_temp() &&
        cur_ic_index_ + 3 < cur_fn_->icodes.size()) {
        const auto &store_ic = cur_fn_->icodes[cur_ic_index_ + 1];
        const auto &sub_ic = cur_fn_->icodes[cur_ic_index_ + 2];
        const auto &get_ic = cur_fn_->icodes[cur_ic_index_ + 3];

        const operand *cursor = nullptr;
        int step = 0;
        if ((store_ic.op == icode_op::ASSIGN ||
             store_ic.op == icode_op::CAST) &&
            store_ic.left.is_temp() &&
            store_ic.left.temp_id == ic.result.temp_id &&
            !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                   ic.result.temp_id)) {
            if (ic.right.kind == operand_kind::INT_CONST &&
                equivalent_operands(ic.left, store_ic.result)) {
                cursor = &store_ic.result;
                step = static_cast<int>(ic.right.ival);
            } else if (ic.left.kind == operand_kind::INT_CONST &&
                       equivalent_operands(ic.right, store_ic.result)) {
                cursor = &store_ic.result;
                step = static_cast<int>(ic.left.ival);
            }
        }

        if (cursor &&
            step >= 1 && step <= 4 &&
            sub_ic.op == icode_op::SUB &&
            sub_ic.result.is_temp() &&
            equivalent_operands(sub_ic.left, *cursor) &&
            sub_ic.right.kind == operand_kind::INT_CONST &&
            static_cast<int>(sub_ic.right.ival) == step &&
            !temp_value_used_after(*cur_fn_, cur_ic_index_ + 4,
                                   sub_ic.result.temp_id) &&
            get_ic.op == icode_op::GET_VALUE_AT &&
            get_ic.left.is_temp() &&
            get_ic.left.temp_id == sub_ic.result.temp_id &&
            get_ic.right.is_none()) {
            int load_size = op_size(get_ic.result);
            if (load_size <= 0 &&
                get_ic.left.type &&
                get_ic.left.type->base) {
                load_size = get_ic.left.type->base->size();
            }
            if (load_size <= 0)
                load_size = 2;
            if ((load_size == 1 && step >= 1) ||
                (load_size == 2 && step >= 2)) {
                direct_postinc_load_pending_ = true;
                direct_postinc_load_cursor_ = *cursor;
                direct_postinc_load_old_ptr_ = sub_ic.result;
                direct_postinc_load_step_ = step;
                direct_postinc_load_get_index_ = cur_ic_index_ + 3;
                skipped_icodes_.insert(cur_ic_index_ + 1);
                skipped_icodes_.insert(cur_ic_index_ + 2);
                return;
            }
        }
    }
    auto rhs_available_in_bc =
        [this](const operand &op) {
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                return it != temp_regs_.end() && it->second == temp_home::main_bc;
            }
            return op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op);
        };
    auto available_in_de =
        [this](const operand &op) {
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                return it != temp_regs_.end() && it->second == temp_home::arg_de;
            }
            if (op.kind != operand_kind::SYMBOL || op.is_global)
                return false;
            auto it = incoming_symbol_homes_.find(op.stack_offset);
            return it != incoming_symbol_homes_.end() &&
                   it->second == temp_home::arg_de &&
                   op.byte_offset == 0;
        };
    auto maybe_preserve_arg_de =
        [this](const operand &op) {
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                if (it != temp_regs_.end() && it->second == temp_home::arg_de) {
                    emit_line("push\thl");
                    maybe_materialize_incoming_arg_temp(op);
                    emit_line("pop\thl");
                }
                return;
            }
            if (op.kind == operand_kind::SYMBOL && !op.is_global) {
                auto it = incoming_symbol_homes_.find(op.stack_offset);
                if (it != incoming_symbol_homes_.end() &&
                    it->second == temp_home::arg_de &&
                    op.byte_offset == 0) {
                    emit_line("push\thl");
                    maybe_materialize_incoming_arg_symbol(op);
                    emit_line("pop\thl");
                }
            }
        };
    auto rhs_available_in_hl =
        [this](const operand &op) {
            if (op.kind != operand_kind::TEMP)
                return false;
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() &&
                   (it->second == temp_home::main_hl ||
                    it->second == temp_home::remat_hl);
        };
    auto lhs_load_preserves_bc =
        [this](const operand &op) {
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                if (it != temp_regs_.end()) {
                    return it->second == temp_home::main_bc ||
                           it->second == temp_home::arg_hl ||
                           it->second == temp_home::arg_de;
                }
                int off = ix_offset_of(op);
                return fits_ix_disp(off) && fits_ix_disp(off + 1);
            }
            if (op.kind != operand_kind::SYMBOL)
                return false;
            if (op.is_global)
                return !op.is_tls;
            if (symbol_home_in_bc(op))
                return true;
            auto si = incoming_symbol_homes_.find(op.stack_offset);
            if (si != incoming_symbol_homes_.end())
                return si->second == temp_home::arg_hl ||
                       si->second == temp_home::arg_de;
            int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };
    auto can_load_word_into_de_without_clobber_hl =
        [this](const operand &op, int word_index) {
            if (op.kind == operand_kind::INT_CONST)
                return true;
            if (op.kind == operand_kind::TEMP && temp_regs_.count(op.temp_id))
                return false;
            if (op.kind != operand_kind::SYMBOL && op.kind != operand_kind::TEMP)
                return false;
            if (op.kind == operand_kind::SYMBOL && op.is_global)
                return false;
            int off = ix_offset_of(op) + (word_index * 2);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };
    auto can_load_rhs_into_de_without_clobber_hl =
        [this](const operand &op) {
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.kind == operand_kind::TEMP) {
                if (temp_regs_.count(op.temp_id))
                    return false;
                int off = ix_offset_of(op);
                return fits_ix_disp(off) && fits_ix_disp(off + 1);
            }
            if (op.kind != operand_kind::SYMBOL)
                return false;
            if (op.is_global)
                return false;
            if (symbol_home_in_bc(op))
                return false;
            if (incoming_symbol_homes_.count(op.stack_offset))
                return false;
            int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };

    if (op_size(ic.result) == 1) {
        const operand *lhs = &ic.left;
        const operand *rhs = &ic.right;
        if (lhs->kind == operand_kind::INT_CONST &&
            rhs->kind != operand_kind::INT_CONST)
            std::swap(lhs, rhs);

        load_a(*lhs);
        if (rhs->kind == operand_kind::INT_CONST) {
            emit_line("add\ta, %s",
                      asm_.imm(static_cast<int>(rhs->ival & 0xff)).c_str());
        } else if (rhs_available_in_bc(*rhs)) {
            emit_line("add\ta, c");
        } else {
            emit_line("ld\te, a");
            load_a(*rhs);
            emit_line("ld\td, a");
            emit_line("ld\ta, e");
            emit_line("add\ta, d");
        }
        store_a(ic.result);
        return;
    }

    if (is_llong_op(ic.left)) {
        // 64-bit: 4-word carry chain.
        load_hl_word(ic.left, 0); emit_line("push\thl");
        load_hl_word(ic.right, 0); emit_line("pop\tde");
        emit_line("add\thl, de");
        store_hl_word(ic.result, 0);
        for (int w = 1; w < 4; ++w) {
            load_hl_word(ic.left, w); emit_line("push\thl");
            load_hl_word(ic.right, w); emit_line("pop\tde");
            emit_line("adc\thl, de");
            store_hl_word(ic.result, w);
        }
    } else if (op_size(ic.left) == 4) {
        // 32-bit: DE:HL = left_lo + right_lo, then ADC for high word.
        load_hl_lo32(ic.left);
        if (can_load_word_into_de_without_clobber_hl(ic.right, 0)) {
            load_de_word(ic.right, 0);
        } else {
            emit_line("push\thl");
            load_hl_lo32(ic.right);
            emit_line("pop\tde");
        }
        emit_line("add\thl, de");
        store_hl_lo32(ic.result);
        load_hl_hi32(ic.left);
        if (can_load_word_into_de_without_clobber_hl(ic.right, 1)) {
            load_de_word(ic.right, 1);
        } else {
            emit_line("push\thl");
            load_hl_hi32(ic.right);
            emit_line("pop\tde");
        }
        emit_line("adc\thl, de");
        store_hl_hi32(ic.result);
    } else {
        auto match_zero_extended_u8_plus_const =
            [this](const operand &op, operand &byte_src,
                   uint16_t &imm16) {
                imm16 = 0;
                if (get_zero_extended_u8_source(op, byte_src))
                    return true;

                if (!op.is_temp())
                    return false;

                const icode *def =
                    find_temp_def_before(op.temp_id, cur_ic_index_);
                if (!def || !def->result.is_temp() ||
                    def->result.temp_id != op.temp_id)
                    return false;
                if (def->op != icode_op::ADD)
                    return false;
                size_t def_idx =
                    static_cast<size_t>(def - &cur_fn_->icodes[0]);
                for (size_t i = def_idx + 1; i < cur_ic_index_; ++i) {
                    switch (cur_fn_->icodes[i].op) {
                    case icode_op::LABEL:
                    case icode_op::GOTO:
                    case icode_op::IFX:
                    case icode_op::CALL:
                    case icode_op::RETURN:
                        return false;
                    default:
                        break;
                    }
                }

                if (def->left.kind == operand_kind::INT_CONST &&
                    get_zero_extended_u8_source(def->right, byte_src)) {
                    imm16 = static_cast<uint16_t>(def->left.ival);
                    return true;
                }
                if (def->right.kind == operand_kind::INT_CONST &&
                    get_zero_extended_u8_source(def->left, byte_src)) {
                    imm16 = static_cast<uint16_t>(def->right.ival);
                    return true;
                }

                return false;
            };

        operand byte_src;
        uint16_t byte_imm = 0;
        const operand *word_lhs = &ic.left;
        if (match_zero_extended_u8_plus_const(ic.right, byte_src, byte_imm)) {
            word_lhs = &ic.left;
        } else if (match_zero_extended_u8_plus_const(ic.left, byte_src, byte_imm)) {
            word_lhs = &ic.right;
        } else {
            word_lhs = nullptr;
        }
        if (word_lhs &&
            is_straight_line_helper_codegen_fn(cur_fn_) &&
            byte_src.kind != operand_kind::INT_CONST) {
            load_a(byte_src);
            emit_line("ld\te, a");
            load_hl(*word_lhs);
            emit_line("ld\ta, l");
            emit_line("add\ta, e");
            if ((byte_imm & 0xff) != 0)
                emit_line("add\ta, %s", asm_.imm(byte_imm & 0xff).c_str());
            emit_line("ld\tl, a");
            emit_line("ld\ta, h");
            emit_line("adc\ta, %s", asm_.imm((byte_imm >> 8) & 0xff).c_str());
            emit_line("ld\th, a");
            store_hl(ic.result);
            return;
        }

        const bool size_biased_small_const_add =
            opt_settings_.level == opt_level::O2 ||
            opt_settings_.level == opt_level::Os;

        // Small signed constant adds are often smaller as repeated INC/DEC
        // than materializing a full 16-bit addend register pair.
        if (ic.right.kind == operand_kind::INT_CONST) {
            int64_t v = ic.right.ival;
            if (v == 0) {
                load_hl(ic.left);
                store_hl(ic.result);
                return;
            }
            if (size_biased_small_const_add && v >= -3 && v <= 3) {
                load_hl(ic.left);
                for (int64_t i = 0; i < (v > 0 ? v : -v); ++i)
                    emit_line(v > 0 ? "inc\thl" : "dec\thl");
                store_hl(ic.result);
                return;
            }
        }
        if (ic.left.kind == operand_kind::INT_CONST &&
            rhs_available_in_bc(ic.right)) {
            load_hl(ic.left);
            emit_line("add\thl, bc");
            store_hl(ic.result);
            return;
        }
        if (ic.left.kind == operand_kind::INT_CONST &&
            available_in_de(ic.right)) {
            load_hl(ic.left);
            emit_line("add\thl, de");
            maybe_preserve_arg_de(ic.right);
            store_hl(ic.result);
            return;
        }
        if (ic.right.kind == operand_kind::INT_CONST &&
            rhs_available_in_bc(ic.left)) {
            load_hl(ic.right);
            emit_line("add\thl, bc");
            store_hl(ic.result);
            return;
        }
        if (ic.right.kind == operand_kind::INT_CONST &&
            available_in_de(ic.left)) {
            load_hl(ic.right);
            emit_line("add\thl, de");
            maybe_preserve_arg_de(ic.left);
            store_hl(ic.result);
            return;
        }
        if (rhs_available_in_bc(ic.left) && rhs_available_in_hl(ic.right)) {
            load_hl(ic.right);
            emit_line("add\thl, bc");
            store_hl(ic.result);
            return;
        }
        if (rhs_available_in_hl(ic.right)) {
            emit_line("push\thl");
            load_hl(ic.left);
            emit_line("pop\tde");
            emit_line("add\thl, de");
            store_hl(ic.result);
            return;
        }
        load_hl(ic.left);
        if (rhs_available_in_bc(ic.right) && lhs_load_preserves_bc(ic.left)) {
            emit_line("add\thl, bc");
            store_hl(ic.result);
            return;
        } else if (!can_load_rhs_into_de_without_clobber_hl(ic.right)) {
            emit_line("push\thl");
            load_de(ic.right);
            emit_line("pop\thl");
            emit_line("add\thl, de");
            store_hl(ic.result);
            return;
        }
        load_de(ic.right);
        emit_line("add\thl, de");
        store_hl(ic.result);
    }
}

void z80_gen::gen_sub(const icode &ic) {
    if (gen_far_ptr_arith(ic, /*is_add=*/false))
        return;
    auto can_load_word_into_de_without_clobber_hl =
        [this](const operand &op, int word_index) {
            if (op.kind == operand_kind::INT_CONST)
                return true;
            if (op.kind == operand_kind::TEMP && temp_regs_.count(op.temp_id))
                return false;
            if (op.kind != operand_kind::SYMBOL && op.kind != operand_kind::TEMP)
                return false;
            if (op.kind == operand_kind::SYMBOL && op.is_global)
                return false;
            int off = ix_offset_of(op) + (word_index * 2);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };
    auto can_load_rhs_into_de_without_clobber_hl =
        [this](const operand &op) {
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.kind == operand_kind::TEMP) {
                if (temp_regs_.count(op.temp_id))
                    return false;
                int off = ix_offset_of(op);
                return fits_ix_disp(off) && fits_ix_disp(off + 1);
            }
            if (op.kind != operand_kind::SYMBOL)
                return false;
            if (op.is_global)
                return false;
            if (symbol_home_in_bc(op))
                return false;
            if (incoming_symbol_homes_.count(op.stack_offset))
                return false;
            int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };

    if (op_size(ic.result) == 1) {
        load_a(ic.left);
        if (ic.right.kind == operand_kind::INT_CONST) {
            emit_line("sub\t%s",
                      asm_.imm(static_cast<int>(ic.right.ival & 0xff)).c_str());
        } else {
            if (ic.right.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(ic.right.temp_id);
                if (it != temp_regs_.end() && it->second == temp_home::main_bc) {
                    emit_line("sub\ta, c");
                    store_a(ic.result);
                    return;
                }
            }
            if (ic.right.kind == operand_kind::SYMBOL && symbol_home_in_bc(ic.right)) {
                emit_line("sub\ta, c");
            } else {
                emit_line("ld\te, a");
                load_a(ic.right);
                emit_line("ld\td, a");
                emit_line("ld\ta, e");
                emit_line("sub\ta, d");
            }
        }
        store_a(ic.result);
        return;
    }

    if (is_llong_op(ic.left)) {
        // 64-bit: 4-word borrow chain.
        load_hl_word(ic.left, 0); emit_line("push\thl");
        load_hl_word(ic.right, 0); emit_line("pop\tde");
        emit_line("ex\tde, hl"); emit_line("or\ta, a"); emit_line("sbc\thl, de");
        store_hl_word(ic.result, 0);
        for (int w = 1; w < 4; ++w) {
            load_hl_word(ic.left, w); emit_line("push\thl");
            load_hl_word(ic.right, w); emit_line("pop\tde");
            emit_line("ex\tde, hl"); emit_line("sbc\thl, de");
            store_hl_word(ic.result, w);
        }
    } else if (op_size(ic.left) == 4) {
        // 32-bit: SBC with carry chain.
        load_hl_lo32(ic.left);
        bool direct_rhs_lo = can_load_word_into_de_without_clobber_hl(ic.right, 0);
        if (direct_rhs_lo) {
            load_de_word(ic.right, 0);
        } else {
            emit_line("push\thl");
            load_hl_lo32(ic.right);
            emit_line("pop\tde");
            emit_line("ex\tde, hl");
        }
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        store_hl_lo32(ic.result);
        load_hl_hi32(ic.left);
        bool direct_rhs_hi = can_load_word_into_de_without_clobber_hl(ic.right, 1);
        if (direct_rhs_hi) {
            load_de_word(ic.right, 1);
        } else {
            emit_line("push\thl");
            load_hl_hi32(ic.right);
            emit_line("pop\tde");
            emit_line("ex\tde, hl");
        }
        emit_line("sbc\thl, de");
        store_hl_hi32(ic.result);
    } else {
        if (ic.right.kind == operand_kind::INT_CONST) {
            int64_t v = ic.right.ival;
            if (v == 1 || v == -1) {
                load_hl(ic.left);
                emit_line(v == 1 ? "dec\thl" : "inc\thl");
                store_hl(ic.result);
                return;
            }
        }
        load_hl(ic.left);
        if (!can_load_rhs_into_de_without_clobber_hl(ic.right))
            emit_line("push\thl");
        load_de(ic.right);
        if (!can_load_rhs_into_de_without_clobber_hl(ic.right))
            emit_line("pop\thl");
        // Z80 has no SUB hl,de; use SBC after clearing carry.
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        store_hl(ic.result);
    }
}

void z80_gen::gen_mul(const icode &ic) {
    if (op_size(ic.left) == 1 && op_size(ic.right) == 1) {
        const bool left_unsigned =
            ic.left.type && ic.left.type->is_unsigned();
        const bool right_unsigned =
            ic.right.type && ic.right.type->is_unsigned();
        auto store_result = [&]() {
            if (op_size(ic.result) == 1) {
                emit_line("ld\ta, l");
                store_a(ic.result);
            } else {
                store_hl(ic.result);
            }
        };

        if (left_unsigned && right_unsigned) {
            load_a(ic.left);
            emit_line("ld\tl, a");
            emit_line("ld\th, %s", asm_.imm(0).c_str());
            load_a(ic.right);
            emit_line("ld\te, a");
            emit_line("ld\td, %s", asm_.imm(0).c_str());
            asm_.global_decl("__mul16");
            emit_line("call\t__mul16");
        } else {
            const char *helper = !left_unsigned && !right_unsigned
                               ? "__mulschar"
                               : (!left_unsigned ? "__muluschar"
                                                 : "__mulsuchar");
            load_a(ic.left);
            emit_line("push\taf");
            load_a(ic.right);
            emit_line("ld\tl, a");
            emit_line("pop\taf");
            asm_.global_decl(helper);
            emit_line("call\t%s", helper);
        }

        emit_line("ex\tde, hl");
        store_result();
        return;
    }

    if (op_size(ic.result) == 2 &&
        op_size(ic.left) == 2 &&
        op_size(ic.right) == 2) {
        const operand *value = nullptr;
        const operand *constant = nullptr;
        if (ic.left.kind == operand_kind::INT_CONST &&
            ic.right.kind != operand_kind::INT_CONST) {
            value = &ic.right;
            constant = &ic.left;
        } else if (ic.right.kind == operand_kind::INT_CONST &&
                   ic.left.kind != operand_kind::INT_CONST) {
            value = &ic.left;
            constant = &ic.right;
        }

        if (value && constant) {
            const uint16_t k = static_cast<uint16_t>(constant->ival);
            int msb = -1;
            for (int bit = 15; bit >= 0; --bit) {
                if ((k >> bit) & 1u) {
                    msb = bit;
                    break;
                }
            }
            int op_count = 0;
            if (msb >= 0) {
                op_count = msb;
                for (int bit = msb - 1; bit >= 0; --bit)
                    if ((k >> bit) & 1u)
                        ++op_count;
            }

            const int max_inline_ops = size_opt_enabled() ? 20 : 30;
            if (msb < 0) {
                emit_line("ld\thl, %s", asm_.imm(0).c_str());
                store_hl(ic.result);
                return;
            }
            if (op_count <= max_inline_ops) {
                load_de(*value);
                emit_line("ld\th, d");
                emit_line("ld\tl, e");
                for (int bit = msb - 1; bit >= 0; --bit) {
                    emit_line("add\thl, hl");
                    if ((k >> bit) & 1u)
                        emit_line("add\thl, de");
                }
                store_hl(ic.result);
                return;
            }
        }
    }

    if (is_llong_op(ic.left)) {
        asm_.global_decl("__mulll");
        for (int w = 3; w >= 0; --w) {
            load_hl_word(ic.right, w);
            emit_line("push\thl");
        }
        load_reg64(ic.left);
        emit_line("call\t__mulll");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        store_reg64(ic.result);
        return;
    }
    if (op_size(ic.left) == 4) {
        asm_.global_decl("__mul32");
        load_hl_hi32(ic.right); emit_line("push\thl");
        load_hl_lo32(ic.right); emit_line("push\thl");
        load_hl_lo32(ic.left);  emit_line("push\thl");
        load_hl_hi32(ic.left);  emit_line("pop\tde");
        emit_line("call\t__mul32");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        emit_line("ld\tb, h");
        emit_line("ld\tc, l");
        emit_line("ex\tde, hl");
        store_hl_lo32(ic.result);
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        store_hl_hi32(ic.result);
    } else {
        asm_.global_decl("__mul16");
        load_hl(ic.left);
        emit_line("push\thl");
        load_hl(ic.right);
        emit_line("pop\tde");
        emit_line("call\t__mul16");
        emit_line("ex\tde, hl");
        store_hl(ic.result);
    }
}

void z80_gen::gen_div_mod(const icode &ic, bool want_mod) {
    if (is_llong_op(ic.left)) {
        const bool is_signed = ic.left.type && !ic.left.type->is_unsigned();
        const char *helper =
            want_mod ? (is_signed ? "__modsll" : "__modull")
                     : (is_signed ? "__divsll" : "__divull");
        asm_.global_decl(helper);
        for (int w = 3; w >= 0; --w) {
            load_hl_word(ic.right, w);
            emit_line("push\thl");
        }
        load_reg64(ic.left);
        emit_line("call\t%s", helper);
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        store_reg64(ic.result);
        return;
    }

    size_t fused_div_idx = 0;
    size_t fused_assign_idx = 0;
    operand fused_quotient_target;
    const bool try_divmod_writeback_fusion =
        want_mod &&
        cur_fn_ &&
        cur_ic_index_ + 2 < cur_fn_->icodes.size() &&
        (op_size(ic.left) == 1 || op_size(ic.left) == 2);

    if (try_divmod_writeback_fusion) {
        auto find_temp_def = [&](const operand &op) -> const icode * {
            if (!op.is_temp())
                return nullptr;
            for (size_t p = cur_ic_index_; p > 0; --p) {
                const auto &def_ic = cur_fn_->icodes[p - 1];
                if (def_ic.result.is_temp() && def_ic.result.temp_id == op.temp_id)
                    return &def_ic;
            }
            return nullptr;
        };

        auto same_unsigned_divisor_value = [&](const operand &a,
                                               const operand &b) {
            if (equivalent_operands(a, b))
                return true;

            auto is_unsigned_widen_of = [&](const operand &widened,
                                            const operand &src) {
                const icode *def_ic = find_temp_def(widened);
                if (!def_ic || def_ic->op != icode_op::CAST ||
                    !def_ic->left.type || !def_ic->result.type)
                    return false;
                if (!equivalent_operands(def_ic->left, src))
                    return false;
                return def_ic->result.type->size() > def_ic->left.type->size();
            };

            return is_unsigned_widen_of(a, b) || is_unsigned_widen_of(b, a);
        };

        auto value_uses_operand = [&](const icode &use_ic,
                                      const operand &op) {
            if (equivalent_operands(use_ic.left, op) ||
                equivalent_operands(use_ic.right, op))
                return true;
            // SET_VALUE_AT stores through result, so the result operand is a
            // value use (the destination address), not a definition.
            return use_ic.op == icode_op::SET_VALUE_AT &&
                   equivalent_operands(use_ic.result, op);
        };

        auto defines_operand = [&](const icode &def_ic,
                                   const operand &op) {
            if (def_ic.op == icode_op::SET_VALUE_AT)
                return false;
            return equivalent_operands(def_ic.result, op);
        };

        std::vector<std::pair<std::string, size_t>> branch_targets;
        auto remember_branch_target = [&](const std::string &label) {
            if (!label.empty())
                branch_targets.emplace_back(label, cur_ic_index_);
        };

        auto has_target = [&](const icode &branch_ic,
                              const std::string &label) {
            if (label.empty())
                return false;
            switch (branch_ic.op) {
            case icode_op::GOTO:
                return branch_ic.label_name == label;
            case icode_op::IFX:
                return branch_ic.true_lbl == label ||
                       branch_ic.false_lbl == label;
            default:
                return false;
            }
        };

        auto branch_window_is_closed = [&](size_t div_idx) {
            std::unordered_set<std::string> expected;
            for (const auto &target : branch_targets) {
                expected.insert(target.first);
            }

            std::unordered_set<std::string> labels_in_window;
            for (size_t i = cur_ic_index_ + 1; i < div_idx; ++i) {
                const auto &scan_ic = cur_fn_->icodes[i];
                if (scan_ic.op != icode_op::LABEL)
                    continue;
                if (expected.find(scan_ic.label_name) == expected.end())
                    return false;
                labels_in_window.insert(scan_ic.label_name);
            }

            for (const auto &target : expected) {
                if (labels_in_window.find(target) == labels_in_window.end())
                    return false;
            }

            for (size_t i = 0; i < cur_fn_->icodes.size(); ++i) {
                if (i >= cur_ic_index_ + 1 && i < div_idx)
                    continue;
                const auto &scan_ic = cur_fn_->icodes[i];
                for (const auto &label : labels_in_window) {
                    if (has_target(scan_ic, label))
                        return false;
                }
            }
            return true;
        };

        for (size_t scan = cur_ic_index_ + 1; scan < cur_fn_->icodes.size(); ++scan) {
            const auto &mid = cur_fn_->icodes[scan];
            if (mid.op == icode_op::LABEL) {
                continue;
            }
            if (mid.op == icode_op::IFX) {
                if (value_uses_operand(mid, ic.left) ||
                    value_uses_operand(mid, ic.right))
                    break;
                remember_branch_target(mid.true_lbl);
                remember_branch_target(mid.false_lbl);
                continue;
            }
            if (is_divmod_fusion_barrier(mid))
                break;

            if (defines_operand(mid, ic.left) ||
                defines_operand(mid, ic.right))
                break;

            if (mid.op == icode_op::DIV &&
                equivalent_operands(mid.left, ic.left) &&
                same_unsigned_divisor_value(mid.right, ic.right)) {
                if (scan + 1 >= cur_fn_->icodes.size())
                    break;

                const auto &assign_ic = cur_fn_->icodes[scan + 1];
                if (assign_ic.op != icode_op::ASSIGN ||
                    !equivalent_operands(assign_ic.left, mid.result) ||
                    !equivalent_operands(assign_ic.result, ic.left) ||
                    equivalent_operands(assign_ic.result, ic.result))
                    break;

                if (!branch_window_is_closed(scan))
                    break;

                if (mid.result.is_temp() &&
                    temp_value_used_after(*cur_fn_, scan + 2, mid.result.temp_id))
                    break;

                fused_div_idx = scan;
                fused_assign_idx = scan + 1;
                fused_quotient_target = assign_ic.result;
                break;
            }

            if (value_uses_operand(mid, ic.left))
                break;
        }
    }

    auto demote_target_to_stack = [&](const operand &target) {
        if (target.is_temp()) {
            temp_regs_.erase(target.temp_id);
            return;
        }
        if (target.is_symbol() && !target.is_global)
            symbol_regs_.erase(symbol_reg_key(target));
    };

    auto record_fused_div_writeback = [&](const operand &target, size_t div_idx,
                                          size_t assign_idx, bool byte_result) {
        demote_target_to_stack(target);
        if (byte_result) {
            emit_line("ld\ta, e");
            store_a(target);
        } else {
            store_de_word(target, 0);
        }
        skipped_icodes_.insert(div_idx);
        skipped_icodes_.insert(assign_idx);
    };

    if (op_size(ic.left) == 1 && op_size(ic.right) == 1) {
        const bool left_unsigned =
            ic.left.type && ic.left.type->is_unsigned();
        const bool right_unsigned =
            ic.right.type && ic.right.type->is_unsigned();
        auto store_result = [&]() {
            if (op_size(ic.result) == 1) {
                emit_line("ld\ta, l");
                store_a(ic.result);
            } else {
                store_hl(ic.result);
            }
        };

        if (left_unsigned && right_unsigned) {
            load_a(ic.left);
            emit_line("ld\tl, a");
            emit_line("ld\th, %s", asm_.imm(0).c_str());
            load_a(ic.right);
            emit_line("ld\te, a");
            emit_line("ld\td, %s", asm_.imm(0).c_str());
            asm_.global_decl("__divuint");
            emit_line("call\t__divuint");
        } else {
            const char *helper =
                want_mod
                    ? (!left_unsigned && !right_unsigned
                           ? "__modschar"
                           : (!left_unsigned ? "__modsuchar"
                                             : "__moduschar"))
                    : (!left_unsigned && !right_unsigned
                           ? "__divschar"
                           : (!left_unsigned ? "__divsuchar"
                                             : "__divuschar"));
            load_a(ic.left);
            emit_line("push\taf");
            load_a(ic.right);
            emit_line("ld\tl, a");
            emit_line("pop\taf");
            asm_.global_decl(helper);
            emit_line("call\t%s", helper);
        }

        if (!want_mod || !left_unsigned || !right_unsigned)
            emit_line("ex\tde, hl");
        store_result();
        if (want_mod && left_unsigned && right_unsigned &&
            fused_div_idx > cur_ic_index_)
            record_fused_div_writeback(fused_quotient_target, fused_div_idx,
                                       fused_assign_idx, true);
        return;
    }

    bool is_signed = ic.left.type && !ic.left.type->is_unsigned();
    if (op_size(ic.left) == 4) {
        const char *helper;
        if (is_signed)
            helper = want_mod ? "__smod32" : "__sdiv32";
        else
            helper = want_mod ? "__mod32" : "__div32";
        asm_.global_decl(helper);
        load_hl_hi32(ic.right); emit_line("push\thl");
        load_hl_lo32(ic.right); emit_line("push\thl");
        load_hl_lo32(ic.left);  emit_line("push\thl");
        load_hl_hi32(ic.left);  emit_line("pop\tde");
        emit_line("call\t%s", helper);
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        emit_line("ld\tb, h");
        emit_line("ld\tc, l");
        emit_line("ex\tde, hl");
        store_hl_lo32(ic.result);
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        store_hl_hi32(ic.result);
    } else {
        load_hl(ic.left);
        if (!load_word_into_de_preserves_hl(ic.right, temp_regs_))
            emit_line("push\thl");
        load_de(ic.right);
        if (!load_word_into_de_preserves_hl(ic.right, temp_regs_))
            emit_line("pop\thl");
        if (is_signed) {
            if (want_mod) {
                asm_.global_decl("__smod16");
                emit_line("call\t__smod16");     // DE = remainder
                emit_line("ex\tde, hl");
                store_hl(ic.result);
            } else {
                asm_.global_decl("__divsint");
                emit_line("call\t__divsint");    // DE = quotient
                emit_line("ex\tde, hl");
                store_hl(ic.result);
            }
        } else {
            asm_.global_decl("__divuint");
            emit_line("call\t__divuint");        // HL = remainder, DE = quotient
            if (want_mod) {
                store_hl(ic.result);
                if (fused_div_idx > cur_ic_index_) {
                    record_fused_div_writeback(fused_quotient_target,
                                               fused_div_idx,
                                               fused_assign_idx,
                                               false);
                }
            } else {
                emit_line("ex\tde, hl");
                store_hl(ic.result);
            }
        }
    }
}

void z80_gen::gen_neg(const icode &ic) {
    if (is_double64_op(ic.result)) {
        load_hl_word(ic.left, 0);
        store_hl_word(ic.result, 0);
        load_hl_word(ic.left, 1);
        store_hl_word(ic.result, 1);
        load_hl_word(ic.left, 2);
        store_hl_word(ic.result, 2);
        load_hl_word(ic.left, 3);
        emit_line("ld\ta, h");
        emit_line("xor\t%s", asm_.imm(0x80).c_str());
        emit_line("ld\th, a");
        store_hl_word(ic.result, 3);
        return;
    }

    if (is_float32_op(ic.result)) {
        load_hl_lo32(ic.left);
        store_hl_lo32(ic.result);
        load_hl_hi32(ic.left);
        emit_line("ld\ta, h");
        emit_line("xor\t%s", asm_.imm(0x80).c_str());
        emit_line("ld\th, a");
        store_hl_hi32(ic.result);
        return;
    }

    if (op_size(ic.result) == 4) {
        // 32-bit integer negate: subtract each word from zero and
        // propagate the borrow into the high word.
        load_hl_lo32(ic.left);
        emit_line("ex\tde, hl");
        emit_line("ld\thl, %s", asm_.imm(0).c_str());
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        store_hl_lo32(ic.result);

        load_hl_hi32(ic.left);
        emit_line("ex\tde, hl");
        emit_line("ld\thl, %s", asm_.imm(0).c_str());
        emit_line("sbc\thl, de");
        store_hl_hi32(ic.result);
        return;
    }

    if (is_llong_op(ic.result)) {
        for (int w = 0; w < 4; ++w) {
            load_hl_word(ic.left, w);
            emit_line("ex\tde, hl");
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            if (w == 0)
                emit_line("or\ta, a");
            emit_line("sbc\thl, de");
            store_hl_word(ic.result, w);
        }
        return;
    }

    if (op_size(ic.result) == 1) {
        load_a(ic.left);
        emit_line("cpl");
        emit_line("inc\ta");
        store_a(ic.result);
        return;
    }

    load_hl(ic.left);
    // negate HL: complement each byte then increment.
    emit_line("ld\ta, l");
    emit_line("cpl");
    emit_line("ld\tl, a");
    emit_line("ld\ta, h");
    emit_line("cpl");
    emit_line("ld\th, a");
    emit_line("inc\thl");
    store_hl(ic.result);
}

void z80_gen::gen_band(const icode &ic) {
    operand direct_byte_band_ifx_value;
    const bool direct_byte_band_ifx =
        cur_fn_ &&
        ic.result.is_temp() &&
        op_size(ic.result) == 1 &&
        find_direct_byte_truth_ifx(ic.result, cur_ic_index_,
                                   direct_byte_band_ifx_value);

    if (is_llong_op(ic.left)) {
        for (int w = 0; w < 4; ++w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ld\ta, l"); emit_line("and\ta, e"); emit_line("ld\tl, a");
            emit_line("ld\ta, h"); emit_line("and\ta, d"); emit_line("ld\th, a");
            store_hl_word(ic.result, w);
        }
        return;
    }

    if (op_size(ic.result) == 4) {
        for (int w = 0; w < 2; ++w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ld\ta, l"); emit_line("and\ta, e"); emit_line("ld\tl, a");
            emit_line("ld\ta, h"); emit_line("and\ta, d"); emit_line("ld\th, a");
            store_hl_word(ic.result, w);
        }
        return;
    }

    auto rhs_available_in_bc =
        [this](const operand &op) {
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                return it != temp_regs_.end() && it->second == temp_home::main_bc;
            }
            return op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op);
        };
    auto available_in_hl =
        [this](const operand &op) {
            if (op.kind != operand_kind::TEMP)
                return false;
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() &&
                   (it->second == temp_home::main_hl ||
                    it->second == temp_home::remat_hl);
        };
    auto lhs_load_preserves_bc =
        [this](const operand &op) {
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                if (it != temp_regs_.end()) {
                    return it->second == temp_home::main_bc ||
                           it->second == temp_home::arg_hl ||
                           it->second == temp_home::arg_de;
                }
                int off = ix_offset_of(op);
                return fits_ix_disp(off) && fits_ix_disp(off + 1);
            }
            if (op.kind != operand_kind::SYMBOL)
                return false;
            if (op.is_global)
                return !op.is_tls;
            if (symbol_home_in_bc(op))
                return true;
            auto si = incoming_symbol_homes_.find(op.stack_offset);
            if (si != incoming_symbol_homes_.end())
                return si->second == temp_home::arg_hl ||
                       si->second == temp_home::arg_de;
            int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };
    const operand *lhs = &ic.left;
    const operand *rhs = &ic.right;
    if (lhs->kind == operand_kind::INT_CONST &&
        rhs->kind != operand_kind::INT_CONST)
        std::swap(lhs, rhs);
    if (rhs_available_in_bc(*lhs) && available_in_hl(*rhs))
        std::swap(lhs, rhs);

    if (rhs->kind == operand_kind::INT_CONST) {
        const uint16_t imm = static_cast<uint16_t>(rhs->ival);
        if (op_size(ic.result) == 1) {
            load_a(*lhs);
            emit_line("and\t%s", asm_.imm(imm & 0xFF).c_str());
            if (direct_byte_band_ifx) {
                direct_byte_load_ifx_pending_ = true;
                direct_byte_load_ifx_value_ = direct_byte_band_ifx_value;
                return;
            }
            store_a(ic.result);
            return;
        }

        load_hl(*lhs);
        emit_line("ld\ta, l");
        emit_line("and\t%s", asm_.imm(imm & 0xFF).c_str());
        emit_line("ld\tl, a");
        emit_line("ld\ta, h");
        emit_line("and\t%s", asm_.imm((imm >> 8) & 0xFF).c_str());
        emit_line("ld\th, a");
        store_hl(ic.result);
        return;
    }

    if (op_size(ic.result) == 1) {
        load_a(*lhs);
        if (rhs_available_in_bc(*rhs) && lhs_load_preserves_bc(*lhs)) {
            emit_line("and\ta, c");
        } else {
            emit_line("ld\te, a");
            load_a(*rhs);
            emit_line("ld\td, a");
            emit_line("ld\ta, e");
            emit_line("and\ta, d");
        }
        if (direct_byte_band_ifx) {
            direct_byte_load_ifx_pending_ = true;
            direct_byte_load_ifx_value_ = direct_byte_band_ifx_value;
            return;
        }
        store_a(ic.result);
        return;
    }

    operand byte_src;
    if (is_straight_line_helper_codegen_fn(cur_fn_) &&
        get_zero_extended_u8_source(*rhs, byte_src)) {
        load_a(byte_src);
        emit_line("ld\te, a");
        load_hl(*lhs);
        emit_line("ld\ta, l");
        emit_line("and\ta, e");
        emit_line("ld\tl, a");
        emit_line("ld\th, %s", asm_.imm(0).c_str());
        store_hl(ic.result);
        return;
    }

    if (rhs_available_in_bc(*rhs) && lhs_load_preserves_bc(*lhs)) {
        load_hl(*lhs);
        emit_line("ld\ta, l"); emit_line("and\ta, c"); emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("and\ta, b"); emit_line("ld\th, a");
    } else if (available_in_hl(*rhs)) {
        emit_line("push\thl");
        load_hl(*lhs);
        emit_line("pop\tde");
        emit_line("ld\ta, l"); emit_line("and\ta, e"); emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("and\ta, d"); emit_line("ld\th, a");
    } else {
        load_hl(*lhs);
        emit_line("push\thl");
        load_hl(*rhs);
        emit_line("pop\tde");
        emit_line("ld\ta, l"); emit_line("and\ta, e"); emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("and\ta, d"); emit_line("ld\th, a");
    }
    store_hl(ic.result);
}

void z80_gen::gen_bor(const icode &ic) {
    if (is_llong_op(ic.left)) {
        for (int w = 0; w < 4; ++w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ld\ta, l"); emit_line("or\te"); emit_line("ld\tl, a");
            emit_line("ld\ta, h"); emit_line("or\td"); emit_line("ld\th, a");
            store_hl_word(ic.result, w);
        }
        return;
    }

    if (op_size(ic.result) == 4) {
        for (int w = 0; w < 2; ++w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ld\ta, l"); emit_line("or\ta, e"); emit_line("ld\tl, a");
            emit_line("ld\ta, h"); emit_line("or\ta, d"); emit_line("ld\th, a");
            store_hl_word(ic.result, w);
        }
        return;
    }

    auto rhs_available_in_bc =
        [this](const operand &op) {
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                return it != temp_regs_.end() && it->second == temp_home::main_bc;
            }
            return op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op);
        };
    auto available_in_hl =
        [this](const operand &op) {
            if (op.kind != operand_kind::TEMP)
                return false;
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() &&
                   (it->second == temp_home::main_hl ||
                    it->second == temp_home::remat_hl);
        };
    auto lhs_load_preserves_bc =
        [this](const operand &op) {
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                if (it != temp_regs_.end()) {
                    return it->second == temp_home::main_bc ||
                           it->second == temp_home::arg_hl ||
                           it->second == temp_home::arg_de;
                }
                int off = ix_offset_of(op);
                return fits_ix_disp(off) && fits_ix_disp(off + 1);
            }
            if (op.kind != operand_kind::SYMBOL)
                return false;
            if (op.is_global)
                return !op.is_tls;
            if (symbol_home_in_bc(op))
                return true;
            auto si = incoming_symbol_homes_.find(op.stack_offset);
            if (si != incoming_symbol_homes_.end())
                return si->second == temp_home::arg_hl ||
                       si->second == temp_home::arg_de;
            int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };
    const operand *lhs = &ic.left;
    const operand *rhs = &ic.right;
    if (lhs->kind == operand_kind::INT_CONST &&
        rhs->kind != operand_kind::INT_CONST)
        std::swap(lhs, rhs);
    if (rhs_available_in_bc(*lhs) && available_in_hl(*rhs))
        std::swap(lhs, rhs);

    if (rhs->kind == operand_kind::INT_CONST) {
        const uint16_t imm = static_cast<uint16_t>(rhs->ival);
        if (op_size(ic.result) == 1) {
            const uint8_t mask = static_cast<uint8_t>(imm & 0xffu);
            if (mask != 0 &&
                (mask & static_cast<uint8_t>(mask - 1)) == 0 &&
                same_call_result_operand(ic.result, *lhs)) {
                int bit = 0;
                uint8_t probe = mask;
                while ((probe & 1u) == 0u) {
                    probe >>= 1;
                    ++bit;
                }

                auto emit_inplace_set = [&](const operand &dst) -> bool {
                    if (dst.kind == operand_kind::SYMBOL && dst.is_global && !dst.is_tls) {
                        std::string sym = asm_.imm_sym(mangle(dst.name));
                        if (dst.byte_offset != 0)
                            sym += " + " + std::to_string(dst.byte_offset);
                        emit_line("ld\thl, %s", sym.c_str());
                        emit_line("set\t%d, (hl)", bit);
                        return true;
                    }
                    if ((dst.kind == operand_kind::SYMBOL && !dst.is_global) ||
                        dst.kind == operand_kind::TEMP) {
                        int off = ix_offset_of(dst);
                        if (fits_ix_disp(off)) {
                            emit_line("set\t%d, %s", bit, asm_.ix_rel(off).c_str());
                            return true;
                        }
                    }
                    return false;
                };

                if (emit_inplace_set(ic.result))
                    return;
            }

            load_a(*lhs);
            emit_line("or\t%s", asm_.imm(imm & 0xFF).c_str());
            store_a(ic.result);
            return;
        }

        load_hl(*lhs);
        emit_line("ld\ta, l");
        emit_line("or\t%s", asm_.imm(imm & 0xFF).c_str());
        emit_line("ld\tl, a");
        emit_line("ld\ta, h");
        emit_line("or\t%s", asm_.imm((imm >> 8) & 0xFF).c_str());
        emit_line("ld\th, a");
        store_hl(ic.result);
        return;
    }

    if (op_size(ic.result) == 1) {
        load_a(*lhs);
        if (rhs_available_in_bc(*rhs) && lhs_load_preserves_bc(*lhs)) {
            emit_line("or\ta, c");
        } else {
            emit_line("ld\te, a");
            load_a(*rhs);
            emit_line("ld\td, a");
            emit_line("ld\ta, e");
            emit_line("or\ta, d");
        }
        store_a(ic.result);
        return;
    }

    operand byte_src;
    if (is_straight_line_helper_codegen_fn(cur_fn_) &&
        get_zero_extended_u8_source(*rhs, byte_src)) {
        load_a(byte_src);
        emit_line("ld\te, a");
        load_hl(*lhs);
        emit_line("ld\ta, l");
        emit_line("or\ta, e");
        emit_line("ld\tl, a");
        store_hl(ic.result);
        return;
    }

    if (rhs_available_in_bc(*rhs) && lhs_load_preserves_bc(*lhs)) {
        load_hl(*lhs);
        emit_line("ld\ta, l"); emit_line("or\ta, c"); emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("or\ta, b"); emit_line("ld\th, a");
    } else if (available_in_hl(*rhs)) {
        emit_line("push\thl");
        load_hl(*lhs);
        emit_line("pop\tde");
        emit_line("ld\ta, l"); emit_line("or\ta, e");  emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("or\ta, d");  emit_line("ld\th, a");
    } else {
        load_hl(*lhs);
        emit_line("push\thl");
        load_hl(*rhs);
        emit_line("pop\tde");
        emit_line("ld\ta, l"); emit_line("or\ta, e");  emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("or\ta, d");  emit_line("ld\th, a");
    }
    store_hl(ic.result);
}

void z80_gen::gen_pack_bytes(const icode &ic) {
    load_a(ic.right);
    emit_line("ld\th, a");
    load_a(ic.left);
    emit_line("ld\tl, a");
    store_hl(ic.result);
}

void z80_gen::gen_bxor(const icode &ic) {
    if (is_llong_op(ic.left)) {
        for (int w = 0; w < 4; ++w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ld\ta, l"); emit_line("xor\ta, e"); emit_line("ld\tl, a");
            emit_line("ld\ta, h"); emit_line("xor\ta, d"); emit_line("ld\th, a");
            store_hl_word(ic.result, w);
        }
        return;
    }

    if (op_size(ic.result) == 4) {
        for (int w = 0; w < 2; ++w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ld\ta, l"); emit_line("xor\ta, e"); emit_line("ld\tl, a");
            emit_line("ld\ta, h"); emit_line("xor\ta, d"); emit_line("ld\th, a");
            store_hl_word(ic.result, w);
        }
        return;
    }

    auto rhs_available_in_bc =
        [this](const operand &op) {
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                return it != temp_regs_.end() && it->second == temp_home::main_bc;
            }
            return op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op);
        };
    auto available_in_hl =
        [this](const operand &op) {
            if (op.kind != operand_kind::TEMP)
                return false;
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() &&
                   (it->second == temp_home::main_hl ||
                    it->second == temp_home::remat_hl);
        };
    auto lhs_load_preserves_bc =
        [this](const operand &op) {
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.kind == operand_kind::TEMP) {
                auto it = temp_regs_.find(op.temp_id);
                if (it != temp_regs_.end()) {
                    return it->second == temp_home::main_bc ||
                           it->second == temp_home::arg_hl ||
                           it->second == temp_home::arg_de;
                }
                int off = ix_offset_of(op);
                return fits_ix_disp(off) && fits_ix_disp(off + 1);
            }
            if (op.kind != operand_kind::SYMBOL)
                return false;
            if (op.is_global)
                return !op.is_tls;
            if (symbol_home_in_bc(op))
                return true;
            auto si = incoming_symbol_homes_.find(op.stack_offset);
            if (si != incoming_symbol_homes_.end())
                return si->second == temp_home::arg_hl ||
                       si->second == temp_home::arg_de;
            int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };
    const operand *lhs = &ic.left;
    const operand *rhs = &ic.right;
    if (lhs->kind == operand_kind::INT_CONST &&
        rhs->kind != operand_kind::INT_CONST)
        std::swap(lhs, rhs);
    if (rhs_available_in_bc(*lhs) && available_in_hl(*rhs))
        std::swap(lhs, rhs);

    if (rhs->kind == operand_kind::INT_CONST) {
        const uint16_t imm = static_cast<uint16_t>(rhs->ival);
        if (op_size(ic.result) == 1) {
            load_a(*lhs);
            emit_line("xor\t%s", asm_.imm(imm & 0xFF).c_str());
            store_a(ic.result);
            return;
        }

        load_hl(*lhs);
        emit_line("ld\ta, l");
        emit_line("xor\t%s", asm_.imm(imm & 0xFF).c_str());
        emit_line("ld\tl, a");
        emit_line("ld\ta, h");
        emit_line("xor\t%s", asm_.imm((imm >> 8) & 0xFF).c_str());
        emit_line("ld\th, a");
        store_hl(ic.result);
        return;
    }

    if (op_size(ic.result) == 1) {
        load_a(*lhs);
        if (rhs_available_in_bc(*rhs) && lhs_load_preserves_bc(*lhs)) {
            emit_line("xor\ta, c");
        } else {
            emit_line("ld\te, a");
            load_a(*rhs);
            emit_line("ld\td, a");
            emit_line("ld\ta, e");
            emit_line("xor\ta, d");
        }
        store_a(ic.result);
        return;
    }

    operand byte_src;
    if (is_straight_line_helper_codegen_fn(cur_fn_) &&
        get_zero_extended_u8_source(*rhs, byte_src)) {
        load_a(byte_src);
        emit_line("ld\te, a");
        load_hl(*lhs);
        emit_line("ld\ta, l");
        emit_line("xor\ta, e");
        emit_line("ld\tl, a");
        store_hl(ic.result);
        return;
    }

    if (rhs_available_in_bc(*rhs) && lhs_load_preserves_bc(*lhs)) {
        load_hl(*lhs);
        emit_line("ld\ta, l"); emit_line("xor\ta, c"); emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("xor\ta, b"); emit_line("ld\th, a");
    } else if (available_in_hl(*rhs)) {
        emit_line("push\thl");
        load_hl(*lhs);
        emit_line("pop\tde");
        emit_line("ld\ta, l"); emit_line("xor\ta, e"); emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("xor\ta, d"); emit_line("ld\th, a");
    } else {
        load_hl(*lhs);
        emit_line("push\thl");
        load_hl(*rhs);
        emit_line("pop\tde");
        emit_line("ld\ta, l"); emit_line("xor\ta, e"); emit_line("ld\tl, a");
        emit_line("ld\ta, h"); emit_line("xor\ta, d"); emit_line("ld\th, a");
    }
    store_hl(ic.result);
}

void z80_gen::gen_bnot(const icode &ic) {
    if (is_llong_op(ic.left)) {
        for (int w = 0; w < 4; ++w) {
            load_hl_word(ic.left, w);
            emit_line("ld\ta, l"); emit_line("cpl"); emit_line("ld\tl, a");
            emit_line("ld\ta, h"); emit_line("cpl"); emit_line("ld\th, a");
            store_hl_word(ic.result, w);
        }
        return;
    }

    if (op_size(ic.result) == 4) {
        for (int w = 0; w < 2; ++w) {
            load_hl_word(ic.left, w);
            emit_line("ld\ta, l"); emit_line("cpl"); emit_line("ld\tl, a");
            emit_line("ld\ta, h"); emit_line("cpl"); emit_line("ld\th, a");
            store_hl_word(ic.result, w);
        }
        return;
    }

    if (op_size(ic.result) == 1) {
        load_a(ic.left);
        emit_line("cpl");
        store_a(ic.result);
        return;
    }

    load_hl(ic.left);
    emit_line("ld\ta, l"); emit_line("cpl"); emit_line("ld\tl, a");
    emit_line("ld\ta, h"); emit_line("cpl"); emit_line("ld\th, a");
    store_hl(ic.result);
}

void z80_gen::gen_shift(const icode &ic, bool right, bool arithmetic) {
    if (is_llong_op(ic.left)) {
        const char *helper = !right ? "__shl64"
                                    : (arithmetic ? "__shr64s" : "__shr64u");
        asm_.global_decl(helper);
        if (ic.right.kind == operand_kind::INT_CONST) {
            emit_line("ld\thl, %s",
                      asm_.imm(static_cast<int>(ic.right.ival & 0xff)).c_str());
        } else {
            load_hl(ic.right);
        }
        emit_line("push\thl");
        load_reg64(ic.left);
        emit_line("call\t%s", helper);
        emit_line("pop\tbc");
        store_reg64(ic.result);
        return;
    }

    if (op_size(ic.result) == 4 && op_size(ic.left) == 4) {
        auto load_32_to_dehl = [&]() {
            // Load the high word into DE *first*: in the deep-frame path
            // load_de_word uses HL as an address scratch, which would clobber
            // the low word if HL were loaded first.  load_hl_lo32 uses BC as
            // its scratch and preserves DE, so doing it last is safe.
            load_de_word(ic.left, 1);
            load_hl_lo32(ic.left);
        };
        auto store_dehl_32 = [&]() {
            store_hl_lo32(ic.result);
            store_de_word(ic.result, 1);
        };
        auto emit_zero_32 = [&]() {
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            store_hl_lo32(ic.result);
            store_hl_hi32(ic.result);
        };
        auto emit_sign_fill_32 = [&]() {
            load_hl_hi32(ic.left);
            emit_line("ld\ta, h");
            emit_line("rlca");
            emit_line("sbc\ta, a");
            emit_line("ld\tl, a");
            emit_line("ld\th, a");
            store_hl_lo32(ic.result);
            store_hl_hi32(ic.result);
        };
        auto emit_one = [&]() {
            if (!right) {
                emit_line("add\thl, hl");
                emit_line("rl\te");
                emit_line("rl\td");
            } else if (arithmetic) {
                emit_line("sra\td");
                emit_line("rr\te");
                emit_line("rr\th");
                emit_line("rr\tl");
            } else {
                emit_line("srl\td");
                emit_line("rr\te");
                emit_line("rr\th");
                emit_line("rr\tl");
            }
        };

        if (ic.right.kind == operand_kind::INT_CONST) {
            int count = static_cast<int>(ic.right.ival & 0xff);
            if (count == 0) {
                load_32_to_dehl();
                store_dehl_32();
                return;
            }
            if (count >= 32) {
                if (right && arithmetic)
                    emit_sign_fill_32();
                else
                    emit_zero_32();
                return;
            }
            load_32_to_dehl();
            for (int k = 0; k < count; ++k)
                emit_one();
            store_dehl_32();
            return;
        }

        load_hl_lo32(ic.left);
        emit_line("push\thl");
        load_de_word(ic.left, 1);
        load_hl(ic.right);
        emit_line("ld\tb, l");
        emit_line("pop\thl");

        std::string done_lbl = fresh_local_label("__sh32_done");
        std::string loop_lbl = fresh_local_label("__sh32");
        emit_line("ld\ta, b");
        emit_line("or\ta, a");
        emit_line("jp\tz, %s", done_lbl.c_str());
        emit_label(loop_lbl, false);
        emit_one();
        emit_line("djnz\t%s", loop_lbl.c_str());
        emit_label(done_lbl, false);
        store_dehl_32();
        return;
    }

    if (op_size(ic.result) == 1 && op_size(ic.left) == 1) {
        load_a(ic.left);

        if (ic.right.kind == operand_kind::INT_CONST) {
            int count = static_cast<int>(ic.right.ival & 0xFF);
            if (count == 0) {
                store_a(ic.result);
                return;
            }

            if (count >= 8) {
                if (!right) {
                    emit_line("xor\ta, a");
                } else if (arithmetic) {
                    emit_line("rlca");
                    emit_line("sbc\ta, a");
                } else {
                    emit_line("xor\ta, a");
                }
                store_a(ic.result);
                return;
            }

            if (count <= 5) {
                for (int k = 0; k < count; ++k) {
                    if (!right)
                        emit_line("add\ta, a");
                    else if (arithmetic)
                        emit_line("sra\ta");
                    else
                        emit_line("srl\ta");
                }
                store_a(ic.result);
                return;
            }

            emit_line("ld\tb, %s", asm_.imm(count).c_str());
            std::string shift_lbl = fresh_local_label("__shiftb");
            emit_label(shift_lbl, false);
            if (!right)
                emit_line("add\ta, a");
            else if (arithmetic)
                emit_line("sra\ta");
            else
                emit_line("srl\ta");
            emit_line("djnz\t%s", shift_lbl.c_str());
            store_a(ic.result);
            return;
        }

        emit_line("ld\te, a");
        load_a(ic.right);
        emit_line("ld\tb, a");
        emit_line("ld\ta, e");

        std::string shift_lbl = fresh_local_label("__shiftb");
        std::string done_lbl  = fresh_local_label("__sbdone");

        emit_line("ld\td, a");
        emit_line("ld\ta, b");
        emit_line("or\ta, a");
        emit_line("jp\tz, %s", done_lbl.c_str());
        emit_line("ld\ta, d");

        emit_label(shift_lbl, false);
        if (!right)
            emit_line("add\ta, a");
        else if (arithmetic)
            emit_line("sra\ta");
        else
            emit_line("srl\ta");
        emit_line("djnz\t%s", shift_lbl.c_str());
        emit_label(done_lbl, false);
        store_a(ic.result);
        return;
    }

    load_hl(ic.left);

    if (ic.right.kind == operand_kind::INT_CONST) {
        int count = (int)(ic.right.ival & 0xFF);
        if (count == 0) { store_hl(ic.result); return; }

        // Shift by 8: byte-swap trick.
        if (count == 8) {
            operand byte_src;
            if (!right && get_zero_extended_u8_source(ic.left, byte_src)) {
                load_a(byte_src);
                emit_line("ld\th, a");
                emit_line("ld\tl, %s", asm_.imm(0).c_str());
                store_hl(ic.result);
                return;
            }
            if (!right) {
                emit_line("ld\th, l");
                emit_line("ld\tl, %s", asm_.imm(0).c_str());
            } else if (arithmetic) {
                emit_line("ld\tl, h");
                for (int k = 0; k < 8; ++k) emit_line("sra\th");
            } else {
                emit_line("ld\tl, h");
                emit_line("ld\th, %s", asm_.imm(0).c_str());
            }
            store_hl(ic.result);
            return;
        }

        // Small constant shifts are smaller and faster fully unrolled.
        const int unroll_limit = 5;
        if (count <= unroll_limit) {
            for (int k = 0; k < count; ++k) {
                if (!right)
                    emit_line("add\thl, hl");
                else if (arithmetic) {
                    emit_line("sra\th"); emit_line("rr\tl");
                } else {
                    emit_line("srl\th"); emit_line("rr\tl");
                }
            }
            store_hl(ic.result);
            return;
        }

        emit_line("ld\tb, %s", asm_.imm(count).c_str());
        std::string shift_lbl = fresh_local_label("__shift");
        emit_label(shift_lbl, false);
        if (!right)
            emit_line("add\thl, hl");
        else if (arithmetic) {
            emit_line("sra\th"); emit_line("rr\tl");
        } else {
            emit_line("srl\th"); emit_line("rr\tl");
        }
        emit_line("djnz\t%s", shift_lbl.c_str());
        store_hl(ic.result);
        return;
    }

    if (size_opt_enabled() &&
        ic.right.kind != operand_kind::INT_CONST &&
        count_variable_shift_sites(cur_fn_, right, arithmetic) >= 2) {
        const char *helper = !right ? "__shl16"
                                    : (arithmetic ? "__shr16s" : "__shr16u");
        emit_line("push\thl");
        load_hl(ic.right);
        emit_line("ld\tb, l");
        emit_line("pop\thl");
        asm_.global_decl(helper);
        emit_line("call\t%s", helper);
        store_hl(ic.result);
        return;
    }

    // Variable or large constant shift: B-register loop.
    emit_line("push\thl");
    load_hl(ic.right);
    emit_line("ld\tb, l");
    emit_line("pop\thl");

    std::string shift_lbl = fresh_local_label("__shift");
    std::string done_lbl  = fresh_local_label("__sdone");

    emit_line("ld\ta, b");
    emit_line("or\ta, a");
    emit_line("jp\tz, %s", done_lbl.c_str());

    emit_label(shift_lbl, false);
    if (!right)
        emit_line("add\thl, hl");
    else if (arithmetic) {
        emit_line("sra\th"); emit_line("rr\tl");
    } else {
        emit_line("srl\th"); emit_line("rr\tl");
    }
    emit_line("djnz\t%s", shift_lbl.c_str());
    emit_label(done_lbl, false);
    store_hl(ic.result);
}

void z80_gen::gen_rotate(const icode &ic, bool right) {
    load_hl(ic.left);

    int count = 0;
    if (ic.right.kind == operand_kind::INT_CONST)
        count = static_cast<int>(ic.right.ival & 0x0F);
    if (count == 0) {
        store_hl(ic.result);
        return;
    }

    if (count > 8) {
        right = !right;
        count = 16 - count;
    }

    if (count == 8) {
        emit_line("ld\ta, l");
        emit_line("ld\tl, h");
        emit_line("ld\th, a");
        store_hl(ic.result);
        return;
    }

    if (!right) {
        emit_line("ld\ta, h");
        for (int k = 0; k < count; ++k)
            emit_line("add\thl, hl");
        for (int k = 0; k < 8 - count; ++k)
            emit_line("rrca");
        emit_line("and\t%s", asm_.imm((1 << count) - 1).c_str());
        emit_line("or\tl");
        emit_line("ld\tl, a");
    } else {
        emit_line("ld\ta, l");
        for (int k = 0; k < count; ++k) {
            emit_line("srl\th");
            emit_line("rr\tl");
        }
        for (int k = 0; k < 8 - count; ++k)
            emit_line("rlca");
        emit_line("and\t%s", asm_.imm((0xFF << (8 - count)) & 0xFF).c_str());
        emit_line("or\th");
        emit_line("ld\th, a");
    }

    store_hl(ic.result);
}

void z80_gen::emit_compare_branch(const icode &ic, icode_op cmp,
                                  const std::string &true_lbl,
                                  const std::string &false_lbl) {
    if (is_real_float_op(ic.left) || is_real_float_op(ic.right)) {
        const bool half_compare =
            is_float16_op(ic.left) || is_float16_op(ic.right);
        auto load_operand_as_double64 = [&](const operand &op) {
            if (op.kind == operand_kind::FLOAT_CONST) {
                if (op.type && op.type->kind == type_kind::FLOAT &&
                    op.type->size() == 4) {
                    asm_.global_decl("___fs2db");
                    load_hl_lo32(op);
                    emit_line("push\thl");
                    load_hl_hi32(op);
                    emit_line("pop\tde");
                    emit_line("call\t___fs2db");
                    return;
                }
                operand dbl = op;
                dbl.type = type::make_double();
                load_reg64(dbl);
                return;
            }
            if (is_double64_op(op)) {
                load_reg64(op);
                return;
            }
            if (is_float32_op(op)) {
                asm_.global_decl("___fs2db");
                load_hl_lo32(op);
                emit_line("push\thl");
                load_hl_hi32(op);
                emit_line("pop\tde");
                emit_line("call\t___fs2db");
                return;
            }
            if (is_llong_op(op)) {
                const char *helper =
                    op.type && op.type->is_unsigned()
                        ? "___ull2db"
                        : "___sll2db";
                asm_.global_decl(helper);
                load_reg64(op);
                emit_line("call\t%s", helper);
                return;
            }
            if (op_size(op) <= 2) {
                const char *helper =
                    op.type && op.type->is_unsigned()
                        ? "___uint2db"
                        : "___sint2db";
                asm_.global_decl(helper);
                load_hl(op);
                emit_line("call\t%s", helper);
                return;
            }
            const char *helper =
                op.type && op.type->is_unsigned()
                    ? "___ulong2db"
                    : "___slong2db";
            asm_.global_decl(helper);
            load_hl_lo32(op);
            emit_line("push\thl");
            load_hl_hi32(op);
            emit_line("pop\tde");
            emit_line("call\t%s", helper);
        };
        auto push_double_stack_arg = [&](const operand &op) {
            load_operand_as_double64(op);
            emit_line("exx");
            emit_line("push\thl");
            emit_line("push\tde");
            emit_line("exx");
            emit_line("push\thl");
            emit_line("push\tde");
        };
        auto emit_de_cmp_branch = [&](icode_op op) {
            std::string skip_lbl;
            switch (op) {
            case icode_op::EQ:
                emit_line("ld\ta, d");
                emit_line("or\te");
                if (!true_lbl.empty())
                    emit_line("jp\tz, %s", true_lbl.c_str());
                break;
            case icode_op::NE:
                emit_line("ld\ta, d");
                emit_line("or\te");
                if (!true_lbl.empty())
                    emit_line("jp\tnz, %s", true_lbl.c_str());
                break;
            case icode_op::LT:
                if (!true_lbl.empty()) {
                    skip_lbl = "__fcmp_skip_" + std::to_string(rand() % 100000);
                    emit_line("ld\ta, d");
                    emit_line("cp\t%s", asm_.imm(0xff).c_str());
                    emit_line("jp\tnz, %s",
                              false_lbl.empty() ? skip_lbl.c_str() : false_lbl.c_str());
                    emit_line("ld\ta, e");
                    emit_line("cp\t%s", asm_.imm(0xff).c_str());
                    emit_line("jp\tz, %s", true_lbl.c_str());
                    if (false_lbl.empty())
                        asm_.label(skip_lbl, false);
                }
                break;
            case icode_op::LE:
                if (!true_lbl.empty()) {
                    std::string le_check_lbl =
                        "__fcmp_le_check_" + std::to_string(rand() % 100000);
                    emit_line("ld\ta, d");
                    emit_line("or\te");
                    emit_line("jp\tz, %s", true_lbl.c_str());
                    emit_line("ld\ta, d");
                    emit_line("cp\t%s", asm_.imm(0xff).c_str());
                    emit_line("jp\tnz, %s",
                              false_lbl.empty() ? le_check_lbl.c_str() : false_lbl.c_str());
                    emit_line("ld\ta, e");
                    emit_line("cp\t%s", asm_.imm(0xff).c_str());
                    emit_line("jp\tz, %s", true_lbl.c_str());
                    if (false_lbl.empty())
                        asm_.label(le_check_lbl, false);
                }
                break;
            case icode_op::GT:
                skip_lbl = "__fcmp_skip_" + std::to_string(rand() % 100000);
                emit_line("bit\t7, d");
                emit_line("jp\tnz, %s",
                          false_lbl.empty() ? skip_lbl.c_str() : false_lbl.c_str());
                emit_line("ld\ta, d");
                emit_line("or\te");
                if (!true_lbl.empty())
                    emit_line("jp\tnz, %s", true_lbl.c_str());
                if (false_lbl.empty())
                    asm_.label(skip_lbl, false);
                break;
            case icode_op::GE:
                emit_line("bit\t7, d");
                if (!true_lbl.empty())
                    emit_line("jp\tz, %s", true_lbl.c_str());
                break;
            default:
                break;
            }
            if (!false_lbl.empty())
                emit_line("jp\t%s", false_lbl.c_str());
        };

        if (half_compare) {
            asm_.global_decl("___fh2fs");
            asm_.global_decl("___fscmp");
            load_hl(ic.right);
            emit_line("call\t___fh2fs");
            emit_line("push\thl");
            emit_line("push\tde");
            load_hl(ic.left);
            emit_line("call\t___fh2fs");
            emit_line("call\t___fscmp");
        } else if (is_double64_op(ic.left) || is_double64_op(ic.right)) {
            asm_.global_decl("___dbcmp");
            push_double_stack_arg(ic.right);
            load_operand_as_double64(ic.left);
            emit_line("call\t___dbcmp");
            emit_line("pop\tbc");
            emit_line("pop\tbc");
            emit_line("pop\tbc");
            emit_line("pop\tbc");
        } else {
            asm_.global_decl("___fscmp");
            load_hl_hi32(ic.right);
            emit_line("push\thl");
            load_hl_lo32(ic.right);
            emit_line("push\thl");
            load_hl_lo32(ic.left);
            emit_line("push\thl");
            load_hl_hi32(ic.left);
            emit_line("pop\tde");
            emit_line("call\t___fscmp");
        }
        emit_de_cmp_branch(cmp);
        return;
    }

    if (op_size(ic.left) == 4 && op_size(ic.right) == 4) {
        const bool is_unsigned = ic.left.type && ic.left.type->is_unsigned();
        const std::string less_lbl = "__lcmp_lt_" + std::to_string(rand() % 100000);
        const std::string greater_lbl = "__lcmp_gt_" + std::to_string(rand() % 100000);
        const std::string equal_lbl = "__lcmp_eq_" + std::to_string(rand() % 100000);
        const std::string end_lbl = "__lcmp_end_" + std::to_string(rand() % 100000);

        for (int w = 1; w >= 0; --w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ex\tde, hl");
            if (w == 1 && !is_unsigned) {
                emit_line("ld\ta, h");
                emit_line("xor\t%s", asm_.imm(0x80).c_str());
                emit_line("ld\th, a");
                emit_line("ld\ta, d");
                emit_line("xor\t%s", asm_.imm(0x80).c_str());
                emit_line("ld\td, a");
            }
            emit_line("or\ta, a");
            emit_line("sbc\thl, de");
            emit_line("jp\tc, %s", less_lbl.c_str());
            emit_line("jp\tnz, %s", greater_lbl.c_str());
        }
        emit_line("jp\t%s", equal_lbl.c_str());

        asm_.label(equal_lbl, false);
        switch (cmp) {
        case icode_op::EQ:
        case icode_op::LE:
        case icode_op::GE:
            if (!true_lbl.empty())
                emit_line("jp\t%s", true_lbl.c_str());
            break;
        default:
            break;
        }
        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        else
            emit_line("jp\t%s", end_lbl.c_str());

        asm_.label(less_lbl, false);
        switch (cmp) {
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::NE:
            if (!true_lbl.empty())
                emit_line("jp\t%s", true_lbl.c_str());
            break;
        default:
            break;
        }
        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        else
            emit_line("jp\t%s", end_lbl.c_str());

        asm_.label(greater_lbl, false);
        switch (cmp) {
        case icode_op::GT:
        case icode_op::GE:
        case icode_op::NE:
            if (!true_lbl.empty())
                emit_line("jp\t%s", true_lbl.c_str());
            break;
        default:
            break;
        }
        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        else
            emit_line("jp\t%s", end_lbl.c_str());

        if (false_lbl.empty())
            asm_.label(end_lbl, false);
        return;
    }

    if (is_llong_op(ic.left)) {
        const bool is_unsigned = ic.left.type && ic.left.type->is_unsigned();
        const std::string less_lbl = "__llcmp_lt_" + std::to_string(rand() % 100000);
        const std::string greater_lbl = "__llcmp_gt_" + std::to_string(rand() % 100000);
        const std::string end_lbl = "__llcmp_end_" + std::to_string(rand() % 100000);

        for (int w = 3; w >= 0; --w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ex\tde, hl");
            if (w == 3 && !is_unsigned) {
                emit_line("ld\ta, h");
                emit_line("xor\t%s", asm_.imm(0x80).c_str());
                emit_line("ld\th, a");
                emit_line("ld\ta, d");
                emit_line("xor\t%s", asm_.imm(0x80).c_str());
                emit_line("ld\td, a");
            }
            emit_line("or\ta, a");
            emit_line("sbc\thl, de");
            emit_line("jp\tc, %s", less_lbl.c_str());
            emit_line("jp\tnz, %s", greater_lbl.c_str());
        }

        switch (cmp) {
        case icode_op::EQ:
        case icode_op::LE:
        case icode_op::GE:
            if (!true_lbl.empty())
                emit_line("jp\t%s", true_lbl.c_str());
            break;
        default:
            break;
        }
        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        else
            emit_line("jp\t%s", end_lbl.c_str());

        asm_.label(less_lbl, false);
        switch (cmp) {
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::NE:
            if (!true_lbl.empty())
                emit_line("jp\t%s", true_lbl.c_str());
            break;
        default:
            break;
        }
        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        else
            emit_line("jp\t%s", end_lbl.c_str());

        asm_.label(greater_lbl, false);
        switch (cmp) {
        case icode_op::GT:
        case icode_op::GE:
        case icode_op::NE:
            if (!true_lbl.empty())
                emit_line("jp\t%s", true_lbl.c_str());
            break;
        default:
            break;
        }
        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        else
            asm_.label(end_lbl, false);
        return;
    }

    auto swapped_compare = [](icode_op op) {
        switch (op) {
        case icode_op::LT: return icode_op::GT;
        case icode_op::LE: return icode_op::GE;
        case icode_op::GT: return icode_op::LT;
        case icode_op::GE: return icode_op::LE;
        default: return op;
        }
    };

    if ((cmp == icode_op::EQ || cmp == icode_op::NE) &&
        can_use_direct_byte_eq_ne_compare(ic)) {
        const operand *lhs = &ic.left;
        const operand *rhs = &ic.right;
        if (lhs->kind == operand_kind::INT_CONST)
            std::swap(lhs, rhs);

        auto emit_cp_rhs_byte = [&]() {
            if (rhs->kind == operand_kind::INT_CONST) {
                emit_line("cp\t%s",
                          asm_.imm(static_cast<int>(rhs->ival & 0xff)).c_str());
                return;
            }
            if (rhs->kind == operand_kind::TEMP) {
                auto ri = temp_regs_.find(rhs->temp_id);
                if (ri != temp_regs_.end()) {
                    switch (ri->second) {
                    case temp_home::main_b:
                        if (rhs->byte_offset == 0) {
                            emit_line("cp\tb");
                            return;
                        }
                        break;
                    case temp_home::main_bc:
                        if (rhs->byte_offset == 0) {
                            emit_line("cp\tc");
                            return;
                        }
                        break;
                    case temp_home::arg_l:
                        emit_line("cp\tl");
                        maybe_materialize_incoming_arg_temp(*rhs);
                        return;
                    case temp_home::arg_hl:
                        emit_line("cp\t%c", rhs->byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_temp(*rhs);
                        return;
                    case temp_home::arg_de:
                        emit_line("cp\t%c", rhs->byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_temp(*rhs);
                        return;
                    default:
                        break;
                    }
                }
            }
            if (rhs->kind == operand_kind::SYMBOL && !rhs->is_global) {
                auto si = incoming_symbol_homes_.find(rhs->stack_offset);
                if (si != incoming_symbol_homes_.end()) {
                    switch (si->second) {
                    case temp_home::arg_l:
                        emit_line("cp\tl");
                        maybe_materialize_incoming_arg_symbol(*rhs);
                        return;
                    case temp_home::arg_hl:
                        emit_line("cp\t%c", rhs->byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_symbol(*rhs);
                        return;
                    case temp_home::arg_de:
                        emit_line("cp\t%c", rhs->byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_symbol(*rhs);
                        return;
                    default:
                        break;
                    }
                }
            }

            if (rhs->kind == operand_kind::SYMBOL && rhs->is_global &&
                !rhs->is_tls) {
                emit_line("ld\tc, (%s)", mangle(rhs->name).c_str());
            } else if (rhs->kind == operand_kind::SYMBOL ||
                       rhs->kind == operand_kind::TEMP) {
                load_frame_byte('c', ix_offset_of(*rhs));
            } else {
                emit_line("push\taf");
                load_a(*rhs);
                emit_line("ld\tc, a");
                emit_line("pop\taf");
            }
            emit_line("cp\tc");
        };

        load_a(*lhs);
        emit_cp_rhs_byte();

        if (!true_lbl.empty())
            emit_line("jp\t%s, %s",
                      cmp == icode_op::EQ ? "z" : "nz",
                      true_lbl.c_str());
        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        return;
    }

    if (can_use_signed_byte_const_compare(ic)) {
        const operand *value = &ic.left;
        const operand *constant = &ic.right;
        icode_op effective_cmp = cmp;
        if (value->kind == operand_kind::INT_CONST) {
            std::swap(value, constant);
            effective_cmp = swapped_compare(cmp);
        }

        load_a(*value);
        emit_line("xor\t%s", asm_.imm(0x80).c_str());
        emit_line("cp\t%s",
                  asm_.imm(static_cast<int>((constant->ival ^ 0x80) & 0xff)).c_str());

        switch (effective_cmp) {
        case icode_op::EQ:
            if (!true_lbl.empty())
                emit_line("jp\tz, %s", true_lbl.c_str());
            break;
        case icode_op::NE:
            if (!true_lbl.empty())
                emit_line("jp\tnz, %s", true_lbl.c_str());
            break;
        case icode_op::LT:
            if (!true_lbl.empty())
                emit_line("jp\tc, %s", true_lbl.c_str());
            break;
        case icode_op::LE:
            if (!true_lbl.empty()) {
                emit_line("jp\tz, %s", true_lbl.c_str());
                emit_line("jp\tc, %s", true_lbl.c_str());
            }
            break;
        case icode_op::GT:
            if (!false_lbl.empty()) {
                emit_line("jp\tz, %s", false_lbl.c_str());
                if (!true_lbl.empty())
                    emit_line("jp\tnc, %s", true_lbl.c_str());
            } else {
                std::string skip_lbl = "__cmp_skip_" + std::to_string(rand() % 100000);
                emit_line("jp\tz, %s", skip_lbl.c_str());
                if (!true_lbl.empty())
                    emit_line("jp\tnc, %s", true_lbl.c_str());
                asm_.label(skip_lbl, false);
            }
            break;
        case icode_op::GE:
            if (!true_lbl.empty())
                emit_line("jp\tnc, %s", true_lbl.c_str());
            break;
        default:
            break;
        }

        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        return;
    }

    if (can_use_unsigned_byte_compare(ic)) {
        const operand *value = &ic.left;
        const operand *constant = &ic.right;
        icode_op effective_cmp = cmp;
        if (value->kind == operand_kind::INT_CONST) {
            std::swap(value, constant);
            effective_cmp = swapped_compare(cmp);
        }

        if (constant->kind == operand_kind::INT_CONST &&
            static_cast<uint8_t>(constant->ival & 0xff) == 0) {
            auto emit_const_compare_result = [&](bool truth) {
                if (truth) {
                    if (!true_lbl.empty())
                        emit_line("jp\t%s", true_lbl.c_str());
                } else if (!false_lbl.empty()) {
                    emit_line("jp\t%s", false_lbl.c_str());
                }
            };

            switch (effective_cmp) {
            case icode_op::LT:
                emit_const_compare_result(false);
                return;
            case icode_op::GE:
                emit_const_compare_result(true);
                return;
            default:
                break;
            }

            load_a(*value);
            emit_line("or\ta, a");
            switch (effective_cmp) {
            case icode_op::EQ:
            case icode_op::LE:
                if (!true_lbl.empty())
                    emit_line("jp\tz, %s", true_lbl.c_str());
                break;
            case icode_op::NE:
            case icode_op::GT:
                if (!true_lbl.empty())
                    emit_line("jp\tnz, %s", true_lbl.c_str());
                break;
            default:
                break;
            }

            if (!false_lbl.empty())
                emit_line("jp\t%s", false_lbl.c_str());
            return;
        }

        auto emit_cp_rhs = [&]() {
            if (ic.right.kind == operand_kind::INT_CONST) {
                emit_line("cp\t%s",
                          asm_.imm(static_cast<int>(ic.right.ival & 0xff)).c_str());
                return;
            }
            if (ic.right.kind == operand_kind::TEMP) {
                auto ri = temp_regs_.find(ic.right.temp_id);
                if (ri != temp_regs_.end()) {
                    switch (ri->second) {
                    case temp_home::main_b:
                        if (ic.right.byte_offset == 0) {
                            emit_line("cp\tb");
                            return;
                        }
                        break;
                    case temp_home::main_bc:
                        if (ic.right.byte_offset == 0) {
                            emit_line("cp\tc");
                            return;
                        }
                        break;
                    case temp_home::arg_l:
                        emit_line("cp\tl");
                        maybe_materialize_incoming_arg_temp(ic.right);
                        return;
                    case temp_home::arg_hl:
                        emit_line("cp\t%c",
                                  ic.right.byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_temp(ic.right);
                        return;
                    case temp_home::arg_de:
                        emit_line("cp\t%c",
                                  ic.right.byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_temp(ic.right);
                        return;
                    default:
                        break;
                    }
                }
            }
            if (ic.right.kind == operand_kind::SYMBOL && !ic.right.is_global) {
                auto si = incoming_symbol_homes_.find(ic.right.stack_offset);
                if (si != incoming_symbol_homes_.end()) {
                    switch (si->second) {
                    case temp_home::arg_l:
                        emit_line("cp\tl");
                        maybe_materialize_incoming_arg_symbol(ic.right);
                        return;
                    case temp_home::arg_hl:
                        emit_line("cp\t%c", ic.right.byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_symbol(ic.right);
                        return;
                    case temp_home::arg_de:
                        emit_line("cp\t%c", ic.right.byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_symbol(ic.right);
                        return;
                    default:
                        break;
                    }
                }
            }

            if (ic.right.kind == operand_kind::SYMBOL && ic.right.is_global &&
                !ic.right.is_tls) {
                emit_line("ld\tc, (%s)", mangle(ic.right.name).c_str());
            } else if (ic.right.kind == operand_kind::SYMBOL ||
                       ic.right.kind == operand_kind::TEMP) {
                load_frame_byte('c', ix_offset_of(ic.right));
            } else {
                emit_line("push\taf");
                load_a(ic.right);
                emit_line("ld\tc, a");
                emit_line("pop\taf");
            }
            emit_line("cp\tc");
        };

        load_a(ic.left);
        emit_cp_rhs();

        switch (cmp) {
        case icode_op::EQ:
            if (!true_lbl.empty())
                emit_line("jp\tz, %s", true_lbl.c_str());
            break;
        case icode_op::NE:
            if (!true_lbl.empty())
                emit_line("jp\tnz, %s", true_lbl.c_str());
            break;
        case icode_op::LT:
            if (!true_lbl.empty())
                emit_line("jp\tc, %s", true_lbl.c_str());
            break;
        case icode_op::LE:
            if (!true_lbl.empty()) {
                emit_line("jp\tz, %s", true_lbl.c_str());
                emit_line("jp\tc, %s", true_lbl.c_str());
            }
            break;
        case icode_op::GT:
            if (!false_lbl.empty()) {
                emit_line("jp\tz, %s", false_lbl.c_str());
                if (!true_lbl.empty())
                    emit_line("jp\tnc, %s", true_lbl.c_str());
            } else {
                std::string skip_lbl = "__cmp_skip_" + std::to_string(rand() % 100000);
                emit_line("jp\tz, %s", skip_lbl.c_str());
                if (!true_lbl.empty())
                    emit_line("jp\tnc, %s", true_lbl.c_str());
                asm_.label(skip_lbl, false);
            }
            break;
        case icode_op::GE:
            if (!true_lbl.empty())
                emit_line("jp\tnc, %s", true_lbl.c_str());
            break;
        default:
            break;
        }

        if (!false_lbl.empty())
            emit_line("jp\t%s", false_lbl.c_str());
        return;
    }

    load_hl(ic.left);
    emit_line("push\thl");
    load_hl(ic.right);
    emit_line("pop\tde");  // DE = left, HL = right
    const bool is_unsigned = ic.left.type && ic.left.type->is_unsigned();

    switch (cmp) {
    case icode_op::EQ:
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        if (!true_lbl.empty())
            emit_line("jp\tz, %s", true_lbl.c_str());
        break;
    case icode_op::NE:
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        if (!true_lbl.empty())
            emit_line("jp\tnz, %s", true_lbl.c_str());
        break;
    case icode_op::LT:
        emit_line("ex\tde, hl");  // HL=left, DE=right
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        if (!true_lbl.empty()) {
            if (is_unsigned)
                emit_line("jp\tc, %s", true_lbl.c_str());
            else
                emit_line("jp\tm, %s", true_lbl.c_str());
        }
        break;
    case icode_op::LE:
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        if (!true_lbl.empty()) {
            emit_line("jp\tz, %s", true_lbl.c_str());
            emit_line("jp\t%s, %s", is_unsigned ? "c" : "m", true_lbl.c_str());
        }
        break;
    case icode_op::GT: {
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        if (!false_lbl.empty()) {
            emit_line("jp\tz, %s", false_lbl.c_str());
            if (!true_lbl.empty()) {
                if (is_unsigned)
                    emit_line("jp\tnc, %s", true_lbl.c_str());
                else
                    emit_line("jp\tp, %s", true_lbl.c_str());
            }
        } else {
            std::string skip_lbl = "__cmp_skip_" + std::to_string(rand() % 100000);
            emit_line("jp\tz, %s", skip_lbl.c_str());
            if (!true_lbl.empty()) {
                if (is_unsigned)
                    emit_line("jp\tnc, %s", true_lbl.c_str());
                else
                    emit_line("jp\tp, %s", true_lbl.c_str());
            }
            asm_.label(skip_lbl, false);
        }
        break;
    }
    case icode_op::GE:
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        if (!true_lbl.empty()) {
            if (is_unsigned)
                emit_line("jp\tnc, %s", true_lbl.c_str());
            else
                emit_line("jp\tp, %s", true_lbl.c_str());
        }
        break;
    default:
        break;
    }

    if (!false_lbl.empty())
        emit_line("jp\t%s", false_lbl.c_str());
}

void z80_gen::gen_compare(const icode &ic, icode_op cmp) {
    auto swapped_compare = [](icode_op op) {
        switch (op) {
        case icode_op::LT: return icode_op::GT;
        case icode_op::LE: return icode_op::GE;
        case icode_op::GT: return icode_op::LT;
        case icode_op::GE: return icode_op::LE;
        default: return op;
        }
    };

    bool direct_return = false;
    operand direct_return_value;

    auto return_target_supported = [&](const operand &op) {
        if (!op.type)
            return false;
        const int sz = op_size(op);
        if (sz != 1 && sz != 2)
            return false;
        switch (effective_call_abi(cur_fn_ ? cur_fn_->abi : call_abi::DEFAULT)) {
        case call_abi::SDCCCALL1:
        case call_abi::SDCCCALL0:
        case call_abi::Z88DK_SMALLC:
        case call_abi::Z88DK_FASTCALL:
        case call_abi::Z88DK_CALLEE:
        case call_abi::NAKED:
        case call_abi::INTERRUPT:
        case call_abi::CRITICAL:
            return true;
        default:
            return false;
        }
    };

    if (cur_fn_ && ic.result.is_temp() &&
        cur_ic_index_ + 1 < cur_fn_->icodes.size()) {
        const auto &next = cur_fn_->icodes[cur_ic_index_ + 1];
        if (next.op == icode_op::RETURN &&
            same_call_result_operand(next.left, ic.result) &&
            !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                   ic.result.temp_id) &&
            return_target_supported(next.left)) {
            direct_return = true;
            direct_return_value = next.left;
        } else if (cur_ic_index_ + 2 < cur_fn_->icodes.size()) {
            const auto &cast_ic = next;
            const auto &ret_ic = cur_fn_->icodes[cur_ic_index_ + 2];
            if (cast_ic.op == icode_op::CAST &&
                same_call_result_operand(cast_ic.left, ic.result) &&
                cast_ic.result.is_temp() &&
                is_truth_test_preserving_integer_cast(cast_ic) &&
                ret_ic.op == icode_op::RETURN &&
                same_call_result_operand(ret_ic.left, cast_ic.result) &&
                !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                       ic.result.temp_id) &&
                !temp_value_used_after(*cur_fn_, cur_ic_index_ + 3,
                                       cast_ic.result.temp_id) &&
                return_target_supported(ret_ic.left)) {
                direct_return = true;
                direct_return_value = cast_ic.result;
            }
        }
    }

    auto prepare_direct_compare_return = [&](const operand &target) {
        const int sz = op_size(target);
        switch (effective_call_abi(cur_fn_ ? cur_fn_->abi : call_abi::DEFAULT)) {
        case call_abi::SDCCCALL1:
        case call_abi::Z88DK_SMALLC:
        case call_abi::Z88DK_FASTCALL:
            if (sz == 1)
                emit_line("ld\ta, l");
            else
                emit_line("ex\tde, hl");
            break;
        default:
            break;
        }
        direct_compare_return_pending_ = true;
        direct_compare_return_value_ = target;
    };

    auto finish_compare_result = [&]() {
        if (direct_return) {
            prepare_direct_compare_return(direct_return_value);
            return;
        }
        store_hl(ic.result);
    };

    if (is_real_float_op(ic.left) || is_real_float_op(ic.right)) {
        const bool half_compare =
            is_float16_op(ic.left) || is_float16_op(ic.right);
        auto load_operand_as_double64 = [&](const operand &op) {
            if (op.kind == operand_kind::FLOAT_CONST) {
                if (op.type && op.type->kind == type_kind::FLOAT &&
                    op.type->size() == 4) {
                    asm_.global_decl("___fs2db");
                    load_hl_lo32(op);
                    emit_line("push\thl");
                    load_hl_hi32(op);
                    emit_line("pop\tde");
                    emit_line("call\t___fs2db");
                    return;
                }
                operand dbl = op;
                dbl.type = type::make_double();
                load_reg64(dbl);
                return;
            }
            if (is_double64_op(op)) {
                load_reg64(op);
                return;
            }
            if (is_float32_op(op)) {
                asm_.global_decl("___fs2db");
                load_hl_lo32(op);
                emit_line("push\thl");
                load_hl_hi32(op);
                emit_line("pop\tde");
                emit_line("call\t___fs2db");
                return;
            }
            if (is_llong_op(op)) {
                const char *helper =
                    op.type && op.type->is_unsigned()
                        ? "___ull2db"
                        : "___sll2db";
                asm_.global_decl(helper);
                load_reg64(op);
                emit_line("call\t%s", helper);
                return;
            }
            if (op_size(op) <= 2) {
                const char *helper =
                    op.type && op.type->is_unsigned()
                        ? "___uint2db"
                        : "___sint2db";
                asm_.global_decl(helper);
                load_hl(op);
                emit_line("call\t%s", helper);
                return;
            }
            const char *helper =
                op.type && op.type->is_unsigned()
                    ? "___ulong2db"
                    : "___slong2db";
            asm_.global_decl(helper);
            load_hl_lo32(op);
            emit_line("push\thl");
            load_hl_hi32(op);
            emit_line("pop\tde");
            emit_line("call\t%s", helper);
        };
        auto push_double_stack_arg = [&](const operand &op) {
            load_operand_as_double64(op);
            emit_line("exx");
            emit_line("push\thl");
            emit_line("push\tde");
            emit_line("exx");
            emit_line("push\thl");
            emit_line("push\tde");
        };
        auto store_bool_from_de_cmp = [&](icode_op op) {
            std::string true_lbl = "__fcmp_true_" + std::to_string(rand() % 100000);
            std::string end_lbl = "__fcmp_end_" + std::to_string(rand() % 100000);

            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            switch (op) {
            case icode_op::EQ:
                emit_line("ld\ta, d");
                emit_line("or\te");
                emit_line("jp\tz, %s", true_lbl.c_str());
                break;
            case icode_op::NE:
                emit_line("ld\ta, d");
                emit_line("or\te");
                emit_line("jp\tnz, %s", true_lbl.c_str());
                break;
            case icode_op::LT:
                emit_line("ld\ta, d");
                emit_line("cp\t%s", asm_.imm(0xff).c_str());
                emit_line("jp\tnz, %s", end_lbl.c_str());
                emit_line("ld\ta, e");
                emit_line("cp\t%s", asm_.imm(0xff).c_str());
                emit_line("jp\tz, %s", true_lbl.c_str());
                break;
            case icode_op::LE:
                emit_line("ld\ta, d");
                emit_line("or\te");
                emit_line("jp\tz, %s", true_lbl.c_str());
                emit_line("ld\ta, d");
                emit_line("cp\t%s", asm_.imm(0xff).c_str());
                emit_line("jp\tnz, %s", end_lbl.c_str());
                emit_line("ld\ta, e");
                emit_line("cp\t%s", asm_.imm(0xff).c_str());
                emit_line("jp\tz, %s", true_lbl.c_str());
                break;
            case icode_op::GT:
                emit_line("bit\t7, d");
                emit_line("jp\tnz, %s", end_lbl.c_str());
                emit_line("ld\ta, d");
                emit_line("or\te");
                emit_line("jp\tnz, %s", true_lbl.c_str());
                break;
            case icode_op::GE:
                emit_line("bit\t7, d");
                emit_line("jp\tz, %s", true_lbl.c_str());
                break;
            default:
                break;
            }
            emit_line("jp\t%s", end_lbl.c_str());
            asm_.label(true_lbl, false);
            emit_line("inc\tl");
            asm_.label(end_lbl, false);
            finish_compare_result();
        };

        if (half_compare) {
            asm_.global_decl("___fh2fs");
            asm_.global_decl("___fscmp");
            load_hl(ic.right);
            emit_line("call\t___fh2fs");
            emit_line("push\thl");
            emit_line("push\tde");
            load_hl(ic.left);
            emit_line("call\t___fh2fs");
            emit_line("call\t___fscmp");
        } else if (is_double64_op(ic.left) || is_double64_op(ic.right)) {
            asm_.global_decl("___dbcmp");
            push_double_stack_arg(ic.right);
            load_operand_as_double64(ic.left);
            emit_line("call\t___dbcmp");
            emit_line("pop\tbc");
            emit_line("pop\tbc");
            emit_line("pop\tbc");
            emit_line("pop\tbc");
        } else {
            asm_.global_decl("___fscmp");
            load_hl_hi32(ic.right);
            emit_line("push\thl");
            load_hl_lo32(ic.right);
            emit_line("push\thl");
            load_hl_lo32(ic.left);
            emit_line("push\thl");
            load_hl_hi32(ic.left);
            emit_line("pop\tde");
            emit_line("call\t___fscmp");
        }
        store_bool_from_de_cmp(cmp);
        return;
    }

    if (op_size(ic.left) == 4 && op_size(ic.right) == 4) {
        const bool is_unsigned = ic.left.type && ic.left.type->is_unsigned();
        const std::string less_lbl = "__lcmp_lt_" + std::to_string(rand() % 100000);
        const std::string greater_lbl = "__lcmp_gt_" + std::to_string(rand() % 100000);
        const std::string equal_lbl = "__lcmp_eq_" + std::to_string(rand() % 100000);
        const std::string done_lbl = "__lcmp_done_" + std::to_string(rand() % 100000);

        for (int w = 1; w >= 0; --w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ex\tde, hl");
            if (w == 1 && !is_unsigned) {
                emit_line("ld\ta, h");
                emit_line("xor\t%s", asm_.imm(0x80).c_str());
                emit_line("ld\th, a");
                emit_line("ld\ta, d");
                emit_line("xor\t%s", asm_.imm(0x80).c_str());
                emit_line("ld\td, a");
            }
            emit_line("or\ta, a");
            emit_line("sbc\thl, de");
            emit_line("jp\tc, %s", less_lbl.c_str());
            emit_line("jp\tnz, %s", greater_lbl.c_str());
        }
        emit_line("jp\t%s", equal_lbl.c_str());

        asm_.label(less_lbl, false);
        switch (cmp) {
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::NE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            break;
        default:
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            break;
        }
        emit_line("jp\t%s", done_lbl.c_str());

        asm_.label(greater_lbl, false);
        switch (cmp) {
        case icode_op::GT:
        case icode_op::GE:
        case icode_op::NE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            break;
        default:
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            break;
        }
        emit_line("jp\t%s", done_lbl.c_str());

        asm_.label(equal_lbl, false);
        switch (cmp) {
        case icode_op::EQ:
        case icode_op::LE:
        case icode_op::GE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            break;
        default:
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            break;
        }

        asm_.label(done_lbl, false);
        finish_compare_result();
        return;
    }

    if (is_llong_op(ic.left)) {
        const bool is_unsigned = ic.left.type && ic.left.type->is_unsigned();
        const std::string less_lbl = "__llcmp_lt_" + std::to_string(rand() % 100000);
        const std::string greater_lbl = "__llcmp_gt_" + std::to_string(rand() % 100000);
        const std::string equal_lbl = "__llcmp_eq_" + std::to_string(rand() % 100000);
        const std::string done_lbl = "__llcmp_done_" + std::to_string(rand() % 100000);

        for (int w = 3; w >= 0; --w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
            load_hl_word(ic.right, w);
            emit_line("pop\tde");
            emit_line("ex\tde, hl");
            if (w == 3 && !is_unsigned) {
                emit_line("ld\ta, h");
                emit_line("xor\t%s", asm_.imm(0x80).c_str());
                emit_line("ld\th, a");
                emit_line("ld\ta, d");
                emit_line("xor\t%s", asm_.imm(0x80).c_str());
                emit_line("ld\td, a");
            }
            emit_line("or\ta, a");
            emit_line("sbc\thl, de");
            emit_line("jp\tc, %s", less_lbl.c_str());
            emit_line("jp\tnz, %s", greater_lbl.c_str());
        }
        emit_line("jp\t%s", equal_lbl.c_str());

        asm_.label(less_lbl, false);
        switch (cmp) {
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::NE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            break;
        default:
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            break;
        }
        emit_line("jp\t%s", done_lbl.c_str());

        asm_.label(greater_lbl, false);
        switch (cmp) {
        case icode_op::GT:
        case icode_op::GE:
        case icode_op::NE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            break;
        default:
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            break;
        }
        emit_line("jp\t%s", done_lbl.c_str());

        asm_.label(equal_lbl, false);
        switch (cmp) {
        case icode_op::EQ:
        case icode_op::LE:
        case icode_op::GE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            break;
        default:
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            break;
        }

        asm_.label(done_lbl, false);
        finish_compare_result();
        return;
    }

    if ((cmp == icode_op::EQ || cmp == icode_op::NE) &&
        can_use_direct_byte_eq_ne_compare(ic)) {
        const operand *lhs = &ic.left;
        const operand *rhs = &ic.right;
        if (lhs->kind == operand_kind::INT_CONST)
            std::swap(lhs, rhs);

        auto emit_cp_rhs_byte = [&]() {
            if (rhs->kind == operand_kind::INT_CONST) {
                emit_line("cp\t%s",
                          asm_.imm(static_cast<int>(rhs->ival & 0xff)).c_str());
                return;
            }
            if (rhs->kind == operand_kind::TEMP) {
                auto ri = temp_regs_.find(rhs->temp_id);
                if (ri != temp_regs_.end()) {
                    switch (ri->second) {
                    case temp_home::main_b:
                        if (rhs->byte_offset == 0) {
                            emit_line("cp\tb");
                            return;
                        }
                        break;
                    case temp_home::main_bc:
                        if (rhs->byte_offset == 0) {
                            emit_line("cp\tc");
                            return;
                        }
                        break;
                    case temp_home::arg_l:
                        emit_line("cp\tl");
                        maybe_materialize_incoming_arg_temp(*rhs);
                        return;
                    case temp_home::arg_hl:
                        emit_line("cp\t%c", rhs->byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_temp(*rhs);
                        return;
                    case temp_home::arg_de:
                        emit_line("cp\t%c", rhs->byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_temp(*rhs);
                        return;
                    default:
                        break;
                    }
                }
            }
            if (rhs->kind == operand_kind::SYMBOL && !rhs->is_global) {
                auto si = incoming_symbol_homes_.find(rhs->stack_offset);
                if (si != incoming_symbol_homes_.end()) {
                    switch (si->second) {
                    case temp_home::arg_l:
                        emit_line("cp\tl");
                        maybe_materialize_incoming_arg_symbol(*rhs);
                        return;
                    case temp_home::arg_hl:
                        emit_line("cp\t%c", rhs->byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_symbol(*rhs);
                        return;
                    case temp_home::arg_de:
                        emit_line("cp\t%c", rhs->byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_symbol(*rhs);
                        return;
                    default:
                        break;
                    }
                }
            }

            if (rhs->kind == operand_kind::SYMBOL && rhs->is_global &&
                !rhs->is_tls) {
                emit_line("ld\tc, (%s)", mangle(rhs->name).c_str());
            } else if (rhs->kind == operand_kind::SYMBOL ||
                       rhs->kind == operand_kind::TEMP) {
                load_frame_byte('c', ix_offset_of(*rhs));
            } else {
                emit_line("push\taf");
                load_a(*rhs);
                emit_line("ld\tc, a");
                emit_line("pop\taf");
            }
            emit_line("cp\tc");
        };

        load_a(*lhs);
        emit_cp_rhs_byte();

        std::string end_lbl = "__cmp_e_" + std::to_string(rand() % 100000);
        emit_line("ld\thl, %s", asm_.imm(1).c_str());
        emit_line("jp\t%s, %s",
                  cmp == icode_op::EQ ? "z" : "nz",
                  end_lbl.c_str());
        emit_line("dec\thl");
        asm_.label(end_lbl, false);
        finish_compare_result();
        return;
    }

    if (can_use_signed_byte_const_compare(ic)) {
        const operand *value = &ic.left;
        const operand *constant = &ic.right;
        icode_op effective_cmp = cmp;
        if (value->kind == operand_kind::INT_CONST) {
            std::swap(value, constant);
            effective_cmp = swapped_compare(cmp);
        }

        load_a(*value);
        emit_line("xor\t%s", asm_.imm(0x80).c_str());
        emit_line("cp\t%s",
                  asm_.imm(static_cast<int>((constant->ival ^ 0x80) & 0xff)).c_str());

        std::string end_lbl = "__cmp_e_" + std::to_string(rand() % 100000);
        emit_line("ld\thl, %s", asm_.imm(1).c_str());
        switch (effective_cmp) {
        case icode_op::EQ:
            emit_line("jp\tz, %s", end_lbl.c_str());
            break;
        case icode_op::NE:
            emit_line("jp\tnz, %s", end_lbl.c_str());
            break;
        case icode_op::LT:
            emit_line("jp\tc, %s", end_lbl.c_str());
            break;
        case icode_op::LE:
            emit_line("jp\tz, %s", end_lbl.c_str());
            emit_line("jp\tc, %s", end_lbl.c_str());
            break;
        case icode_op::GT: {
            std::string false_lbl = "__cmp_f_" + std::to_string(rand() % 100000);
            emit_line("jp\tz, %s", false_lbl.c_str());
            emit_line("jp\tnc, %s", end_lbl.c_str());
            asm_.label(false_lbl, false);
            emit_line("dec\thl");
            asm_.label(end_lbl, false);
            finish_compare_result();
            return;
        }
        case icode_op::GE:
            emit_line("jp\tnc, %s", end_lbl.c_str());
            break;
        default:
            break;
        }
        emit_line("dec\thl");
        asm_.label(end_lbl, false);
        finish_compare_result();
        return;
    }

    if (can_use_unsigned_byte_compare(ic)) {
        auto emit_cp_rhs = [&]() {
            if (ic.right.kind == operand_kind::INT_CONST) {
                emit_line("cp\t%s",
                          asm_.imm(static_cast<int>(ic.right.ival & 0xff)).c_str());
                return;
            }
            if (ic.right.kind == operand_kind::TEMP) {
                auto ri = temp_regs_.find(ic.right.temp_id);
                if (ri != temp_regs_.end()) {
                    switch (ri->second) {
                    case temp_home::main_b:
                        if (ic.right.byte_offset == 0) {
                            emit_line("cp\tb");
                            return;
                        }
                        break;
                    case temp_home::main_bc:
                        if (ic.right.byte_offset == 0) {
                            emit_line("cp\tc");
                            return;
                        }
                        break;
                    case temp_home::arg_l:
                        emit_line("cp\tl");
                        maybe_materialize_incoming_arg_temp(ic.right);
                        return;
                    case temp_home::arg_hl:
                        emit_line("cp\t%c",
                                  ic.right.byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_temp(ic.right);
                        return;
                    case temp_home::arg_de:
                        emit_line("cp\t%c",
                                  ic.right.byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_temp(ic.right);
                        return;
                    default:
                        break;
                    }
                }
            }
            if (ic.right.kind == operand_kind::SYMBOL && !ic.right.is_global) {
                auto si = incoming_symbol_homes_.find(ic.right.stack_offset);
                if (si != incoming_symbol_homes_.end()) {
                    switch (si->second) {
                    case temp_home::arg_l:
                        emit_line("cp\tl");
                        maybe_materialize_incoming_arg_symbol(ic.right);
                        return;
                    case temp_home::arg_hl:
                        emit_line("cp\t%c", ic.right.byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_symbol(ic.right);
                        return;
                    case temp_home::arg_de:
                        emit_line("cp\t%c", ic.right.byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_symbol(ic.right);
                        return;
                    default:
                        break;
                    }
                }
            }

            if (ic.right.kind == operand_kind::SYMBOL && ic.right.is_global &&
                !ic.right.is_tls) {
                emit_line("ld\tc, (%s)", mangle(ic.right.name).c_str());
            } else if (ic.right.kind == operand_kind::SYMBOL ||
                       ic.right.kind == operand_kind::TEMP) {
                load_frame_byte('c', ix_offset_of(ic.right));
            } else {
                emit_line("push\taf");
                load_a(ic.right);
                emit_line("ld\tc, a");
                emit_line("pop\taf");
            }
            emit_line("cp\tc");
        };

        load_a(ic.left);
        emit_cp_rhs();

        std::string end_lbl  = "__cmp_e_" + std::to_string(rand() % 100000);
        switch (cmp) {
        case icode_op::EQ:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            emit_line("jp\tz, %s", end_lbl.c_str());
            emit_line("dec\thl");
            break;
        case icode_op::NE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            emit_line("jp\tnz, %s", end_lbl.c_str());
            emit_line("dec\thl");
            break;
        case icode_op::LT:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            emit_line("jp\tc, %s", end_lbl.c_str());
            emit_line("dec\thl");
            break;
        case icode_op::LE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            emit_line("jp\tz, %s", end_lbl.c_str());
            emit_line("jp\tc, %s", end_lbl.c_str());
            emit_line("dec\thl");
            break;
        case icode_op::GT:
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            emit_line("jp\tc, %s", end_lbl.c_str());
            emit_line("jp\tz, %s", end_lbl.c_str());
            emit_line("inc\tl");
            break;
        case icode_op::GE:
            emit_line("ld\thl, %s", asm_.imm(1).c_str());
            emit_line("jp\tnc, %s", end_lbl.c_str());
            emit_line("dec\thl");
            break;
        default:
            break;
        }

        asm_.label(end_lbl, false);
        finish_compare_result();
        return;
    }

    load_hl(ic.left);
    emit_line("push\thl");
    load_hl(ic.right);
    emit_line("pop\tde");  // DE = left, HL = right

    std::string end_lbl  = "__cmp_e_" + std::to_string(rand() % 100000);
    const bool is_unsigned = ic.left.type && ic.left.type->is_unsigned();

    switch (cmp) {
    case icode_op::EQ:
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("ld\thl, %s", asm_.imm(1).c_str());
        emit_line("jp\tz, %s", end_lbl.c_str());
        emit_line("dec\thl");
        break;
    case icode_op::NE:
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("ld\thl, %s", asm_.imm(1).c_str());
        emit_line("jp\tnz, %s", end_lbl.c_str());
        emit_line("dec\thl");
        break;
    case icode_op::LT:
        emit_line("ex\tde, hl");  // HL=left, DE=right
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("ld\thl, %s", asm_.imm(1).c_str());
        emit_line("jp\t%s, %s", is_unsigned ? "c" : "m", end_lbl.c_str());
        emit_line("dec\thl");
        break;
    case icode_op::LE:
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("ld\thl, %s", asm_.imm(1).c_str());
        emit_line("jp\tz, %s", end_lbl.c_str());
        emit_line("jp\t%s, %s", is_unsigned ? "c" : "m", end_lbl.c_str());
        emit_line("dec\thl");
        break;
    case icode_op::GT:
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("ld\thl, %s", asm_.imm(0).c_str());
        emit_line("jp\tz, %s", end_lbl.c_str());
        emit_line("jp\t%s, %s", is_unsigned ? "c" : "m", end_lbl.c_str());
        emit_line("inc\tl");
        break;
    case icode_op::GE:
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("ld\thl, %s", asm_.imm(1).c_str());
        emit_line("jp\t%s, %s", is_unsigned ? "nc" : "p", end_lbl.c_str());
        emit_line("dec\thl");
        break;
    default:
        break;
    }

    asm_.label(end_lbl, false);
    finish_compare_result();
}

void z80_gen::gen_cast(const icode &ic) {
    auto source_safe_for_delayed_byte_remat = [&](const operand &src) {
        if (src.kind == operand_kind::SYMBOL && !src.is_global) {
            return incoming_symbol_homes_.find(src.stack_offset) ==
                   incoming_symbol_homes_.end();
        }
        if (!src.is_temp())
            return true;
        auto it = temp_regs_.find(src.temp_id);
        if (it == temp_regs_.end())
            return true;
        switch (it->second) {
        case temp_home::arg_a:
        case temp_home::arg_l:
        case temp_home::arg_hl:
        case temp_home::arg_de:
            return false;
        default:
            return true;
        }
    };

    auto first_use_index = [&](int temp_id) -> size_t {
        if (!cur_fn_)
            return cur_fn_ ? cur_fn_->icodes.size() : 0;
        auto uses_temp = [&](const operand &op) {
            return op.is_temp() && op.temp_id == temp_id;
        };
        for (size_t i = cur_ic_index_ + 1; i < cur_fn_->icodes.size(); ++i) {
            const auto &next = cur_fn_->icodes[i];
            if (uses_temp(next.left) || uses_temp(next.right))
                return i;
            if (uses_temp(next.result))
                break;
        }
        return cur_fn_->icodes.size();
    };

    if (direct_compare_return_pending_ &&
        same_call_result_operand(ic.result, direct_compare_return_value_) &&
        is_truth_test_preserving_integer_cast(ic)) {
        return;
    }

    if (direct_call_ifx_pending_ &&
        same_call_result_operand(ic.left, direct_call_ifx_value_) &&
        ic.result.is_temp() &&
        is_truth_test_preserving_integer_cast(ic)) {
        direct_call_ifx_value_ = ic.result;
        return;
    }

    if (cur_fn_ && ic.result.is_temp()) {
        const size_t first_use = first_use_index(ic.result.temp_id);
        const bool delayed_remat_safe =
            first_use == cur_ic_index_ + 1 ||
            source_safe_for_delayed_byte_remat(ic.left);

        if (ic.left.type &&
            ic.left.type->size() == 1 &&
            is_truth_test_preserving_integer_cast(ic) &&
            first_use < cur_fn_->icodes.size()) {
            const auto &use_ic = cur_fn_->icodes[first_use];
            if (use_ic.op == icode_op::IFX &&
                use_ic.left.is_temp() &&
                use_ic.left.temp_id == ic.result.temp_id &&
                delayed_remat_safe &&
                !temp_value_used_after(*cur_fn_, first_use + 1,
                                       ic.result.temp_id)) {
                return;
            }
        }

        if (op_size(ic.left) == 1 &&
            op_size(ic.result) == 2 &&
            first_use < cur_fn_->icodes.size()) {
            const auto &use_ic = cur_fn_->icodes[first_use];
            if (use_ic.op == icode_op::SEND &&
                use_ic.left.is_temp() &&
                use_ic.left.temp_id == ic.result.temp_id &&
                (use_ic.arg_loc == abi_arg_loc::REG_HL ||
                 use_ic.arg_loc == abi_arg_loc::REG_DE) &&
                !temp_value_used_after(*cur_fn_, first_use + 1,
                                       ic.result.temp_id)) {
                if (delayed_remat_safe) {
                    return;
                }
                if (first_use == cur_ic_index_ + 1) {
                    direct_widen_send_pending_ = true;
                    direct_widen_send_value_ = ic.result;
                    direct_widen_send_source_ = ic.left;
                    return;
                }
            }
        }
    }

    if (!ic.result.type) {
        load_hl(ic.left);
        store_hl(ic.result);
        return;
    }

    // ----- far (24-bit banked) pointer conversions -------------------
    // A far pointer is 3 bytes (addr16 + bank).  Convert by adjusting
    // only the bank byte; the 16-bit address is preserved.
    {
        const bool dst_far = ic.result.type->is_far_ptr();
        const bool src_far = ic.left.type && ic.left.type->is_far_ptr();
        if (dst_far && !src_far) {
            // Widen near pointer / integer -> far: low 16 bits = source,
            // bank = 0 (current bank is the target's concern at deref time).
            invalidate_pair_cache();
            load_hl(ic.left);
            store_hl_word(ic.result, 0);
            emit_line("xor\ta");
            store_far_bank(ic.result);
            return;
        }
        if (src_far && !dst_far) {
            // Narrow far -> near pointer / integer: keep low 16 bits.
            invalidate_pair_cache();
            load_hl_word(ic.left, 0);
            if (op_size(ic.result) == 1) {
                emit_line("ld\ta, l");
                store_a(ic.result);
            } else {
                store_hl(ic.result);
            }
            return;
        }
        if (src_far && dst_far) {
            // far -> far: identical representation, copy all 3 bytes.
            gen_assign(ic);
            return;
        }
    }

    const bool src_is_float16 = is_float16_op(ic.left);
    const bool src_is_float32 = is_float32_op(ic.left);
    const bool src_is_fixed_float = is_fixed_float_op(ic.left);
    const bool src_is_double64 = is_double64_op(ic.left);
    const bool src_is_real_float = src_is_float16 || src_is_float32 || src_is_double64;
    const bool src_is_llong = is_llong_op(ic.left);
    const bool dst_is_float16 =
        ic.result.type->kind == type_kind::FLOAT &&
        get_float_format() == float_format::IEEE16 &&
        ic.result.type->size() == 2;
    const bool dst_is_float32 =
        ic.result.type->kind == type_kind::FLOAT &&
        get_float_format() == float_format::IEEE32 &&
        ic.result.type->size() == 4;
    const bool dst_is_fixed_float = is_fixed_float_type(ic.result.type);
    const bool dst_is_double64 =
        ic.result.type->kind == type_kind::DOUBLE && ic.result.type->size() == 8;
    const bool dst_is_real_float = dst_is_float16 || dst_is_float32 || dst_is_double64;
    const bool dst_is_llong =
        ic.result.type->kind == type_kind::LLONG ||
        ic.result.type->kind == type_kind::ULLONG;

    int src_sz = op_size(ic.left);
    int dst_sz = op_size(ic.result);

    auto store_de_as_int = [&](int size) {
        emit_line("push\tde");
        emit_line("pop\thl");
        if (size == 1) {
            emit_line("ld\ta, l");
            store_a(ic.result);
        } else {
            store_hl(ic.result);
        }
    };

    auto store_dehl32 = [&]() {
        if (dst_sz == 1) {
            emit_line("ld\ta, e");
            store_a(ic.result);
        } else if (dst_sz == 2) {
            store_de(ic.result);
        } else {
            store_de_word(ic.result, 0);
            store_hl_hi32(ic.result);
        }
    };

    auto load_fixed32_dehl = [&](const operand &op) {
        load_de_word(op, 0);
        load_hl_hi32(op);
    };

    auto store_half_from_de = [&]() {
        emit_line("push\tde");
        emit_line("pop\thl");
        store_hl(ic.result);
    };

    auto load_half_as_float32 = [&](const operand &op) {
        asm_.global_decl("___fh2fs");
        load_hl(op);
        emit_line("call\t___fh2fs");
    };

    auto emit_integer_to_float32 = [&](const operand &op) {
        if (is_llong_op(op)) {
            asm_.global_decl(op.type && op.type->is_unsigned()
                                 ? "___ull2db"
                                 : "___sll2db");
            load_reg64(op);
            emit_line("call\t%s",
                      op.type && op.type->is_unsigned()
                          ? "___ull2db"
                          : "___sll2db");
            asm_.global_decl("___db2fs");
            emit_line("call\t___db2fs");
            return;
        }

        if (op_size(op) <= 2) {
            const char *helper =
                op.type && op.type->is_unsigned()
                    ? "___uint2fs"
                    : "___sint2fs";
            asm_.global_decl(helper);
            load_hl(op);
            emit_line("call\t%s", helper);
            return;
        }

        const char *helper =
            op.type && op.type->is_unsigned()
                ? "___ulong2fs"
                : "___slong2fs";
        asm_.global_decl(helper);
        load_hl_lo32(op);
        emit_line("push\thl");
        load_hl_hi32(op);
        emit_line("pop\tde");
        emit_line("call\t%s", helper);
    };

    if (dst_is_float16 && !src_is_float16) {
        if (ic.left.kind == operand_kind::FLOAT_CONST ||
            ic.left.kind == operand_kind::INT_CONST) {
            operand packed = ic.left.kind == operand_kind::FLOAT_CONST
                ? operand::make_float(ic.left.fval, ic.result.type)
                : operand::make_float(static_cast<double>(ic.left.ival), ic.result.type);
            load_hl(packed);
            store_hl(ic.result);
            return;
        }

        if (src_is_double64) {
            asm_.global_decl("___db2fs");
            asm_.global_decl("___fs2fh");
            load_reg64(ic.left);
            emit_line("call\t___db2fs");
            emit_line("call\t___fs2fh");
            store_half_from_de();
            return;
        }

        if (src_is_float32) {
            asm_.global_decl("___fs2fh");
            load_de_word(ic.left, 0);
            load_hl_hi32(ic.left);
            emit_line("call\t___fs2fh");
            store_half_from_de();
            return;
        }

        emit_integer_to_float32(ic.left);
        asm_.global_decl("___fs2fh");
        emit_line("call\t___fs2fh");
        store_half_from_de();
        return;
    }

    if (src_is_float16 && !dst_is_float16) {
        if (dst_is_double64) {
            asm_.global_decl("___fs2db");
            load_half_as_float32(ic.left);
            emit_line("call\t___fs2db");
            store_reg64(ic.result);
            return;
        }

        if (dst_is_float32) {
            load_half_as_float32(ic.left);
            store_dehl32();
            return;
        }

        if (dst_is_llong) {
            asm_.global_decl("___fs2db");
            asm_.global_decl(ic.result.type->is_unsigned()
                                 ? "___db2ull"
                                 : "___db2sll");
            load_half_as_float32(ic.left);
            emit_line("call\t___fs2db");
            emit_line("call\t%s",
                      ic.result.type->is_unsigned()
                          ? "___db2ull"
                          : "___db2sll");
            store_reg64(ic.result);
            return;
        }

        {
            const char *helper =
                dst_sz <= 2
                    ? (ic.result.type->is_unsigned() ? "___fs2uint" : "___fs2sint")
                    : (ic.result.type->is_unsigned() ? "___fs2ulong" : "___fs2slong");
            asm_.global_decl(helper);
            load_half_as_float32(ic.left);
            emit_line("call\t%s", helper);
            if (dst_sz <= 2)
                store_de_as_int(dst_sz);
            else
                store_dehl32();
            return;
        }
    }

    if (dst_is_fixed_float && !src_is_fixed_float) {
        if (ic.left.kind == operand_kind::FLOAT_CONST ||
            ic.left.kind == operand_kind::INT_CONST) {
            operand packed = ic.left.kind == operand_kind::FLOAT_CONST
                ? operand::make_float(ic.left.fval, ic.result.type)
                : operand::make_float(static_cast<double>(ic.left.ival), ic.result.type);
            if (dst_sz == 2) {
                load_hl(packed);
                store_hl(ic.result);
            } else {
                load_hl_lo32(packed);
                store_hl_lo32(ic.result);
                load_hl_hi32(packed);
                store_hl_hi32(ic.result);
            }
            return;
        }

        if (!src_is_real_float && !src_is_llong) {
            const std::string helper = fixed_helper_name("_from_int");
            asm_.global_decl(helper);
            load_hl(ic.left);
            emit_line("call\t%s", helper.c_str());
            if (dst_sz == 2)
                store_de_as_int(2);
            else
                store_dehl32();
            return;
        }
    }

    if (src_is_fixed_float && !dst_is_fixed_float) {
        if (ic.result.type->is_integer() && !dst_is_llong) {
            const std::string helper = fixed_helper_name("_to_int");
            asm_.global_decl(helper);
            if (src_sz == 2)
                load_hl(ic.left);
            else
                load_fixed32_dehl(ic.left);
            emit_line("call\t%s", helper.c_str());
            if (dst_sz <= 2)
                store_de_as_int(dst_sz);
            else
                store_dehl32();
            return;
        }
    }

    if (dst_is_real_float) {
        if (src_is_float32 && dst_is_double64) {
            asm_.global_decl("___fs2db");
            load_de_word(ic.left, 0);
            load_hl_hi32(ic.left);
            emit_line("call\t___fs2db");
            store_reg64(ic.result);
            return;
        }

        if (src_is_double64 && dst_is_float32) {
            asm_.global_decl("___db2fs");
            load_reg64(ic.left);
            emit_line("call\t___db2fs");
            store_dehl32();
            return;
        }

        if (!src_is_real_float) {
            if (dst_is_double64) {
                if (src_is_llong) {
                    asm_.global_decl(ic.left.type && ic.left.type->is_unsigned()
                                         ? "___ull2db"
                                         : "___sll2db");
                    load_reg64(ic.left);
                    emit_line("call\t%s",
                              ic.left.type && ic.left.type->is_unsigned()
                                  ? "___ull2db"
                                  : "___sll2db");
                } else if (src_sz <= 2) {
                    const char *helper =
                        ic.left.type && ic.left.type->is_unsigned()
                            ? "___uint2db"
                            : "___sint2db";
                    asm_.global_decl(helper);
                    load_hl(ic.left);
                    emit_line("call\t%s", helper);
                } else {
                    const char *helper =
                        ic.left.type && ic.left.type->is_unsigned()
                            ? "___ulong2db"
                            : "___slong2db";
                    asm_.global_decl(helper);
                    load_hl_lo32(ic.left);
                    emit_line("push\thl");
                    load_hl_hi32(ic.left);
                    emit_line("pop\tde");
                    emit_line("call\t%s", helper);
                }
                store_reg64(ic.result);
                return;
            }

            if (dst_is_float32) {
                if (src_is_llong) {
                    asm_.global_decl(ic.left.type && ic.left.type->is_unsigned()
                                         ? "___ull2db"
                                         : "___sll2db");
                    load_reg64(ic.left);
                    emit_line("call\t%s",
                              ic.left.type && ic.left.type->is_unsigned()
                                  ? "___ull2db"
                                  : "___sll2db");
                    asm_.global_decl("___db2fs");
                    emit_line("call\t___db2fs");
                } else if (src_sz <= 2) {
                    const char *helper =
                        ic.left.type && ic.left.type->is_unsigned()
                            ? "___uint2db"
                            : "___sint2db";
                    asm_.global_decl(helper);
                    load_hl(ic.left);
                    emit_line("call\t%s", helper);
                    asm_.global_decl("___db2fs");
                    emit_line("call\t___db2fs");
                } else {
                    const char *helper =
                        ic.left.type && ic.left.type->is_unsigned()
                            ? "___ulong2fs"
                            : "___slong2fs";
                    asm_.global_decl(helper);
                    load_hl_lo32(ic.left);
                    emit_line("push\thl");
                    load_hl_hi32(ic.left);
                    emit_line("pop\tde");
                    emit_line("call\t%s", helper);
                }
                store_dehl32();
                return;
            }
        }
    }

    if (src_is_real_float && !dst_is_real_float) {
        if (src_is_double64) {
            if (dst_is_llong) {
                asm_.global_decl(ic.result.type->is_unsigned()
                                     ? "___db2ull"
                                     : "___db2sll");
                load_reg64(ic.left);
                emit_line("call\t%s",
                          ic.result.type->is_unsigned()
                              ? "___db2ull"
                              : "___db2sll");
                store_reg64(ic.result);
                return;
            }

            const char *helper =
                dst_sz <= 2
                    ? (ic.result.type->is_unsigned() ? "___db2uint" : "___db2sint")
                    : (ic.result.type->is_unsigned() ? "___db2ulong" : "___db2slong");
            asm_.global_decl(helper);
            load_reg64(ic.left);
            emit_line("call\t%s", helper);
            if (dst_sz <= 2)
                store_de_as_int(dst_sz);
            else
                store_dehl32();
            return;
        }

        if (src_is_float32) {
            if (dst_is_llong) {
                asm_.global_decl("___fs2db");
                load_hl_lo32(ic.left);
                emit_line("push\thl");
                load_hl_hi32(ic.left);
                emit_line("pop\tde");
                emit_line("call\t___fs2db");
                asm_.global_decl(ic.result.type->is_unsigned()
                                     ? "___db2ull"
                                     : "___db2sll");
                emit_line("call\t%s",
                          ic.result.type->is_unsigned()
                              ? "___db2ull"
                              : "___db2sll");
                store_reg64(ic.result);
                return;
            }

            const char *helper =
                ic.result.type->is_unsigned() ? "___fs2ulong" : "___fs2slong";
            asm_.global_decl(helper);
            load_hl_lo32(ic.left);
            emit_line("push\thl");
            load_hl_hi32(ic.left);
            emit_line("pop\tde");
            emit_line("call\t%s", helper);
            if (dst_sz <= 2)
                store_de_as_int(dst_sz);
            else
                store_dehl32();
            return;
        }
    }

    if (!src_is_real_float && !dst_is_real_float &&
        (src_is_llong || dst_is_llong)) {
        if (dst_is_llong) {
            if (src_is_llong) {
                load_reg64(ic.left);
                store_reg64(ic.result);
                return;
            }

            if (src_sz <= 2) {
                const char *helper =
                    ic.left.type && ic.left.type->is_unsigned()
                        ? "___uint2ll"
                        : "___sint2ll";
                asm_.global_decl(helper);
                load_hl(ic.left);
                emit_line("call\t%s", helper);
            } else {
                const char *helper =
                    ic.left.type && ic.left.type->is_unsigned()
                        ? "___ulong2ll"
                        : "___slong2ll";
                asm_.global_decl(helper);
                load_hl_lo32(ic.left);
                emit_line("push\thl");
                load_hl_hi32(ic.left);
                emit_line("pop\tde");
                emit_line("call\t%s", helper);
            }
            store_reg64(ic.result);
            return;
        }

        if (src_is_llong) {
            const char *helper =
                dst_sz <= 2
                    ? (ic.result.type->is_unsigned() ? "___ll2uint" : "___ll2sint")
                    : (ic.result.type->is_unsigned() ? "___ll2ulong" : "___ll2slong");
            asm_.global_decl(helper);
            load_reg64(ic.left);
            emit_line("call\t%s", helper);
            if (dst_sz <= 2)
                store_de_as_int(dst_sz);
            else
                store_dehl32();
            return;
        }
    }

    if (dst_sz == src_sz) {
        if (dst_sz == 8) {
            load_reg64(ic.left);
            store_reg64(ic.result);
        } else if (dst_sz == 4) {
            load_hl_lo32(ic.left);
            store_hl_lo32(ic.result);
            load_hl_hi32(ic.left);
            store_hl_hi32(ic.result);
        } else if (dst_sz == 1) {
            load_a(ic.left);
            store_a(ic.result);
        } else {
            load_hl(ic.left);
            store_hl(ic.result);
        }
    } else if (dst_sz > src_sz) {
        if (src_sz == 1) {
            load_a(ic.left);
            const bool src_unsigned =
                ic.left.type && ic.left.type->is_unsigned();
            if (src_unsigned) {
                emit_line("ld\tl, a");
                emit_line("ld\th, %s", asm_.imm(0).c_str());
            } else {
                // Sign-extend A into HL.
                emit_line("ld\tl, a");
                emit_line("rlca");
                emit_line("sbc\ta, a");  // A = 0x00 or 0xFF
                emit_line("ld\th, a");
            }
            if (dst_sz == 2) {
                store_hl(ic.result);
            } else if (dst_sz == 4) {
                // A holds the sign byte (0x00/0xFF) for the signed case; it
                // survives store_hl_lo32 (which preserves AF), whereas H does
                // not in the deep-frame path (HL is used as an address scratch).
                store_hl_lo32(ic.result);
                if (src_unsigned) {
                    emit_line("ld\thl, %s", asm_.imm(0).c_str());
                } else {
                    emit_line("ld\th, a");
                    emit_line("ld\tl, a");
                }
                store_hl_hi32(ic.result);
            } else {
                store_hl(ic.result);
            }
        } else if (dst_sz == 4 && src_sz == 2) {
            load_hl(ic.left);
            if (ic.left.type && ic.left.type->is_unsigned()) {
                store_hl_lo32(ic.result);
                emit_line("ld\thl, %s", asm_.imm(0).c_str());
                store_hl_hi32(ic.result);
            } else {
                // Compute the sign byte from H *before* storing the low word.
                // A large (deep) frame stores through HL as an address scratch,
                // clobbering it; reading H afterwards would pick up a stack
                // address byte and sign-extend to 0xFFFF.  store_hl_lo32
                // preserves AF, so the sign byte in A survives it.
                emit_line("ld\ta, h");
                emit_line("rlca");
                emit_line("sbc\ta, a");          // A = 0x00 / 0xFF
                store_hl_lo32(ic.result);
                emit_line("ld\th, a");
                emit_line("ld\tl, a");
                store_hl_hi32(ic.result);
            }
        } else {
            load_hl(ic.left);
            store_hl(ic.result);
        }
    } else {
        // Narrowing.
        if (src_sz == 4 && dst_sz <= 2) {
            load_hl_lo32(ic.left);
            if (dst_sz == 1) {
                emit_line("ld\ta, l");
                store_a(ic.result);
            } else {
                store_hl(ic.result);
            }
        } else {
            load_hl(ic.left);
            if (dst_sz == 1) {
                emit_line("ld\ta, l");
                store_a(ic.result);
            } else {
                store_hl(ic.result);
            }
        }
    }
}

// ----- Soft-float helpers in lib/runtime/ ----------------------------
//
// IEEE 754 single (4 bytes). Each operand pushed as 32-bit value
// (lo word first); result returned in DE:HL (DE=hi, HL=lo).
//
void z80_gen::gen_float_arith(const icode &ic) {
    if (is_fixed_float_op(ic.result) ||
        is_fixed_float_op(ic.left) ||
        is_fixed_float_op(ic.right)) {
        auto store_de16 = [&]() {
            emit_line("push\tde");
            emit_line("pop\thl");
            store_hl(ic.result);
        };
        auto store_dehl32 = [&]() {
            emit_line("ld\tb, h");
            emit_line("ld\tc, l");
            emit_line("push\tde");
            emit_line("pop\thl");
            store_hl_lo32(ic.result);
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            store_hl_hi32(ic.result);
        };
        auto load_fixed32_dehl = [&](const operand &op) {
            load_hl_hi32(op);
            emit_line("push\thl");
            load_hl_lo32(op);
            emit_line("ex\tde, hl");
            emit_line("pop\thl");
        };
        auto push_fixed32_stack_arg = [&](const operand &op) {
            load_hl_hi32(op);
            emit_line("push\thl");
            load_hl_lo32(op);
            emit_line("push\thl");
        };
        auto emit_fixed_call = [&](const std::string &helper) {
            asm_.global_decl(helper);
            if (op_size(ic.result) == 2) {
                load_de(ic.right);
                load_hl(ic.left);
                emit_line("call\t%s", helper.c_str());
                store_de16();
                return;
            }
            push_fixed32_stack_arg(ic.right);
            load_fixed32_dehl(ic.left);
            emit_line("call\t%s", helper.c_str());
            emit_line("pop\tbc");
            emit_line("pop\tbc");
            store_dehl32();
        };

        switch (ic.op) {
        case icode_op::FADD:
            emit_fixed_call(fixed_helper_name("_add"));
            return;
        case icode_op::FSUB:
            emit_fixed_call(fixed_helper_name("_sub"));
            return;
        case icode_op::FMUL:
            emit_fixed_call(fixed_helper_name("_mul"));
            return;
        case icode_op::FDIV:
            emit_fixed_call(fixed_helper_name("_div"));
            return;
        case icode_op::FITOSF: {
            const std::string helper = fixed_helper_name("_from_int");
            asm_.global_decl(helper);
            load_hl(ic.left);
            emit_line("call\t%s", helper.c_str());
            if (op_size(ic.result) == 2)
                store_de16();
            else
                store_dehl32();
            return;
        }
        case icode_op::FSTOI: {
            const std::string helper = fixed_helper_name("_to_int");
            asm_.global_decl(helper);
            if (op_size(ic.left) == 2)
                load_hl(ic.left);
            else
                load_fixed32_dehl(ic.left);
            emit_line("call\t%s", helper.c_str());
            store_de16();
            return;
        }
        default:
            break;
        }
    }

    if (is_float16_op(ic.result) ||
        is_float16_op(ic.left) ||
        is_float16_op(ic.right)) {
        auto push_half_as_float32 = [&](const operand &op) {
            asm_.global_decl("___fh2fs");
            load_hl(op);
            emit_line("call\t___fh2fs");
            emit_line("push\thl");
            emit_line("push\tde");
        };
        auto load_half_as_float32 = [&](const operand &op) {
            asm_.global_decl("___fh2fs");
            load_hl(op);
            emit_line("call\t___fh2fs");
        };
        auto store_half_result = [&]() {
            asm_.global_decl("___fs2fh");
            emit_line("call\t___fs2fh");
            emit_line("push\tde");
            emit_line("pop\thl");
            store_hl(ic.result);
        };
        auto store_de_as_int = [&](int size) {
            emit_line("push\tde");
            emit_line("pop\thl");
            if (size == 1) {
                emit_line("ld\ta, l");
                store_a(ic.result);
            } else {
                store_hl(ic.result);
            }
        };
        auto store_dehl32 = [&]() {
            if (op_size(ic.result) == 1) {
                emit_line("ld\ta, e");
                store_a(ic.result);
            } else if (op_size(ic.result) == 2) {
                store_de(ic.result);
            } else {
                store_de_word(ic.result, 0);
                store_hl_hi32(ic.result);
            }
        };
        auto emit_integer_to_float32 = [&](const operand &op) {
            if (is_llong_op(op)) {
                asm_.global_decl(op.type && op.type->is_unsigned()
                                     ? "___ull2db"
                                     : "___sll2db");
                load_reg64(op);
                emit_line("call\t%s",
                          op.type && op.type->is_unsigned()
                              ? "___ull2db"
                              : "___sll2db");
                asm_.global_decl("___db2fs");
                emit_line("call\t___db2fs");
                return;
            }
            if (op_size(op) <= 2) {
                const char *helper =
                    op.type && op.type->is_unsigned()
                        ? "___uint2fs"
                        : "___sint2fs";
                asm_.global_decl(helper);
                load_hl(op);
                emit_line("call\t%s", helper);
                return;
            }
            const char *helper =
                op.type && op.type->is_unsigned()
                    ? "___ulong2fs"
                    : "___slong2fs";
            asm_.global_decl(helper);
            load_hl_lo32(op);
            emit_line("push\thl");
            load_hl_hi32(op);
            emit_line("pop\tde");
            emit_line("call\t%s", helper);
        };

        switch (ic.op) {
        case icode_op::FADD:
        case icode_op::FSUB:
        case icode_op::FMUL:
        case icode_op::FDIV: {
            const char *helper = "__fsadd";
            switch (ic.op) {
            case icode_op::FADD: helper = "__fsadd"; break;
            case icode_op::FSUB: helper = "__fssub"; break;
            case icode_op::FMUL: helper = "__fsmul"; break;
            case icode_op::FDIV: helper = "__fsdiv"; break;
            default: break;
            }
            asm_.global_decl(helper);
            push_half_as_float32(ic.right);
            load_half_as_float32(ic.left);
            emit_line("call\t%s", helper);
            emit_line("pop\tbc");
            emit_line("pop\tbc");
            store_half_result();
            return;
        }
        case icode_op::FITOSF:
            emit_integer_to_float32(ic.left);
            store_half_result();
            return;
        case icode_op::FSTOI: {
            load_half_as_float32(ic.left);
            if (is_llong_op(ic.result)) {
                asm_.global_decl("___fs2db");
                asm_.global_decl(ic.result.type && ic.result.type->is_unsigned()
                                     ? "___db2ull"
                                     : "___db2sll");
                emit_line("call\t___fs2db");
                emit_line("call\t%s",
                          ic.result.type && ic.result.type->is_unsigned()
                              ? "___db2ull"
                              : "___db2sll");
                store_reg64(ic.result);
                return;
            }

            const char *helper =
                op_size(ic.result) <= 2
                    ? ((ic.result.type && ic.result.type->is_unsigned())
                           ? "___fs2uint"
                           : "___fs2sint")
                    : ((ic.result.type && ic.result.type->is_unsigned())
                           ? "___fs2ulong"
                           : "___fs2slong");
            asm_.global_decl(helper);
            emit_line("call\t%s", helper);
            if (op_size(ic.result) <= 2)
                store_de_as_int(op_size(ic.result));
            else
                store_dehl32();
            return;
        }
        default:
            break;
        }
    }

    if (op_size(ic.result) == 8 ||
        is_double64_op(ic.left) ||
        is_double64_op(ic.right)) {
        const char *helper = "__dbadd";
        switch (ic.op) {
        case icode_op::FADD: helper = "__dbadd";  break;
        case icode_op::FSUB: helper = "__dbsub";  break;
        case icode_op::FMUL: helper = "___dbmul"; break;
        case icode_op::FDIV: helper = "___dbdiv"; break;
        default:
            break;
        }

        asm_.global_decl(helper);
        for (int w = 3; w >= 0; --w) {
            load_hl_word(ic.right, w);
            emit_line("push\thl");
        }
        load_reg64(ic.left);
        emit_line("call\t%s", helper);
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        store_reg64(ic.result);
        return;
    }

    static const struct {
        icode_op    op;
        const char *helper;
        bool        binary;
    } tbl[] = {
        { icode_op::FADD,   "__fsadd",  true  },
        { icode_op::FSUB,   "__fssub",  true  },
        { icode_op::FMUL,   "__fsmul",  true  },
        { icode_op::FDIV,   "__fsdiv",  true  },
        { icode_op::FITOSF, "__fitosf", false },
        { icode_op::FSTOI,  "__fstoi",  false },
    };
    const char *helper = "__fsadd";
    bool binary = true;
    for (auto &entry : tbl) {
        if (entry.op == ic.op) { helper = entry.helper; binary = entry.binary; break; }
    }
    if (binary) {
        asm_.global_decl(helper);
        load_hl_hi32(ic.right); emit_line("push\thl");
        load_hl_lo32(ic.right); emit_line("push\thl");
        load_hl_lo32(ic.left);  emit_line("push\thl");
        load_hl_hi32(ic.left);  emit_line("pop\tde");
        emit_line("call\t%s", helper);
        emit_line("pop\tbc");
        emit_line("pop\tbc");
    } else {
        asm_.global_decl(helper);
        load_hl_hi32(ic.left); emit_line("push\thl");
        load_hl_lo32(ic.left); emit_line("push\thl");
        emit_line("call\t%s", helper);
        emit_line("pop\tbc"); emit_line("pop\tbc");
    }
    emit_line("ld\tb, h");
    emit_line("ld\tc, l");
    emit_line("ex\tde, hl");
    store_hl_lo32(ic.result);
    emit_line("ld\th, b");
    emit_line("ld\tl, c");
    store_hl_hi32(ic.result);
}

void z80_gen::gen_alloca(const icode &ic) {
    load_hl(ic.left);
    emit_line("ex\tde, hl");
    emit_line("ld\thl, %s", asm_.imm(0).c_str());
    emit_line("add\thl, sp");
    emit_line("or\ta, a");
    emit_line("sbc\thl, de");
    emit_line("ld\tsp, hl");
    store_hl(ic.result);
}

void z80_gen::gen_inline_asm(const icode &ic) {
    invalidate_pair_cache();
    asm_.raw(ic.asm_text + "\n");
    invalidate_pair_cache();
}

void z80_gen::gen_make_complex(const icode &ic) {
    if (ic.result.kind == operand_kind::TEMP)
        alloc_temp(ic.result.temp_id, 8);

    load_hl_word(ic.left, 0); store_hl_word(ic.result, 0);
    load_hl_word(ic.left, 1); store_hl_word(ic.result, 1);

    operand res_im = ic.result; res_im.byte_offset += 4;
    load_hl_word(ic.right, 0); store_hl_word(res_im, 0);
    load_hl_word(ic.right, 1); store_hl_word(res_im, 1);
}

} // namespace xcc
