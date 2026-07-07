//
// z80gen_mem.cpp — Memory icode handlers: assign, address_of,
//                  get_value_at, set_value_at.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"

namespace xcc {

static bool is_direct_abs_ptr(const operand &op) {
    return op.kind == operand_kind::INT_CONST;
}

static bool is_direct_data_symbol(const operand &op) {
    return op.kind == operand_kind::SYMBOL &&
           op.is_global &&
           !op.is_tls &&
           !op.is_func &&
           !op.is_param;
}

static bool is_direct_array_symbol(const operand &op) {
    return is_direct_data_symbol(op) &&
           op.type &&
           op.type->unqual() &&
           op.type->unqual()->kind == type_kind::ARRAY;
}

static bool load_word_preserves_hl(const operand &op) {
    return op.kind == operand_kind::INT_CONST ||
           op.kind == operand_kind::LABEL_REF;
}

static bool load_byte_preserves_hl(const operand &op,
                                   const std::unordered_map<int, temp_home> &temp_regs) {
    if (op.is_sfr && op.sfr_port >= 0)
        return true;
    if (op.kind == operand_kind::INT_CONST)
        return true;
    if (op.kind == operand_kind::SYMBOL && op.is_global && !op.is_tls)
        return true;
    if (op.kind == operand_kind::TEMP) {
        auto it = temp_regs.find(op.temp_id);
        return it != temp_regs.end() &&
               (it->second == temp_home::alt_a ||
                it->second == temp_home::main_a ||
                it->second == temp_home::main_b ||
                it->second == temp_home::main_c ||
                it->second == temp_home::arg_a ||
                it->second == temp_home::arg_l);
    }
    return false;
}

void z80_gen::gen_assign(const icode &ic) {
    int sz = op_size(ic.result);
    if (sz <= 0)
        sz = op_size(ic.left);
    if (direct_word_value_pending_ &&
        ic.left.is_temp() &&
        direct_word_value_.is_temp() &&
        ic.left.temp_id == direct_word_value_.temp_id &&
        sz == 2) {
        store_de(ic.result);
        direct_word_value_pending_ = false;
        direct_word_value_ = operand{};
        return;
    }
    if (sz == 1) {
        load_a(ic.left);
        store_a(ic.result);
    } else if (sz > 2) {
        for (int w = 0; w < sz / 2; ++w) {
            load_hl_word(ic.left, w);
            store_hl_word(ic.result, w);
        }
        if (sz & 1) {
            operand src_tail = ic.left;
            operand dst_tail = ic.result;
            src_tail.byte_offset += (sz - 1);
            dst_tail.byte_offset += (sz - 1);
            src_tail.type = type::make_char();
            dst_tail.type = type::make_char();
            load_a(src_tail);
            store_a(dst_tail);
        }
    } else {
        load_hl(ic.left);
        store_hl(ic.result);
    }
}

void z80_gen::gen_address_of(const icode &ic) {
    if (ic.result.is_temp()) {
        auto ri = temp_regs_.find(ic.result.temp_id);
        if (ri != temp_regs_.end() && ri->second == temp_home::remat_hl)
            return;
    }
    if (ic.left.is_global) {
        emit_line("ld\thl, %s", asm_.imm_sym(mangle(ic.left.name)).c_str());
    } else {
        int off = ic.left.is_param ? param_ix_offset(ic.left) : ic.left.stack_offset;
        if (ic.left.kind == operand_kind::TEMP)
            off = ix_offset_of(ic.left);
        load_ix_addr_hl(off);
    }
    store_hl(ic.result);
}

// ----- far (24-bit banked) pointer support ---------------------------
//
// Representation: 3 bytes — bytes 0..1 = 16-bit address, byte 2 = bank.
// Dereference goes through the per-target trampoline:
//   __far_getb : in  HL = address, C = bank        -> out A = byte
//   __far_putb : in  HL = address, C = bank, A = byte
// Both trampolines MUST preserve BC, DE and HL (only A/flags change),
// so the codegen below can keep the pointer live across calls.  The
// default (none/cpm3) implementations ignore the bank and do a plain
// (hl) access — see lib/runtime/far/.

void z80_gen::load_far_bank(const operand &ptr) {
    // Bank byte lives at offset 2; load_a/store_a now honour byte_offset
    // for globals (sym+2) as well as frame/param slots.
    operand bank = ptr;
    bank.byte_offset += 2;
    bank.type = type::make_uchar();
    load_a(bank);
}

void z80_gen::store_far_bank(const operand &dst) {
    operand bank = dst;
    bank.byte_offset += 2;
    bank.type = type::make_uchar();
    store_a(bank);
}

void z80_gen::emit_load_far_ptr(const operand &ptr) {
    // HL = address (low 16 bits).  Load first, then protect across the
    // bank load (load_a may clobber HL/BC for deep frame or TLS sources).
    load_hl_word(ptr, 0);
    emit_line("push\thl");
    load_far_bank(ptr);
    emit_line("ld\tc, a");   // C = bank
    emit_line("pop\thl");    // HL = address
}

bool z80_gen::gen_far_get_value_at(const icode &ic) {
    if (!ic.left.type || !ic.left.type->is_far_ptr())
        return false;

    invalidate_a_cache();
    invalidate_pair_cache();

    int n = op_size(ic.result);
    emit_load_far_ptr(ic.left);   // HL = addr, C = bank
    asm_.global_decl("__far_getb");

    if (n == 1) {
        emit_line("call\t__far_getb");
        store_a(ic.result);
    } else if (n == 2) {
        emit_line("call\t__far_getb");  // low byte
        emit_line("ld\te, a");
        emit_line("inc\thl");
        emit_line("call\t__far_getb");  // high byte
        emit_line("ld\td, a");
        store_de(ic.result);
    } else {
        // Wider pointee (long / struct / far-pointer-to-far-pointer): the
        // result is a multi-byte temp with a frame slot.  Store byte by byte.
        int base = ix_offset_of(ic.result);
        for (int i = 0; i < n; ++i) {
            emit_line("call\t__far_getb");
            store_frame_byte(base + i, 'a');
            if (i < n - 1)
                emit_line("inc\thl");
        }
    }
    invalidate_a_cache();
    invalidate_pair_cache();
    return true;
}

bool z80_gen::gen_far_set_value_at(const icode &ic) {
    if (!ic.result.type || !ic.result.type->is_far_ptr())
        return false;

    invalidate_a_cache();
    invalidate_pair_cache();

    int n = op_size(ic.left);
    emit_load_far_ptr(ic.result);  // HL = addr, C = bank
    asm_.global_decl("__far_putb");

    for (int i = 0; i < n; ++i) {
        // Preserve pointer (HL) and bank (BC) across the value-byte load,
        // which may clobber both.
        emit_line("push\tbc");
        emit_line("push\thl");
        operand vbyte = ic.left;
        vbyte.byte_offset += i;
        vbyte.type = type::make_uchar();
        load_a(vbyte);
        emit_line("pop\thl");
        emit_line("pop\tbc");
        emit_line("call\t__far_putb");
        if (i < n - 1)
            emit_line("inc\thl");
    }
    invalidate_a_cache();
    invalidate_pair_cache();
    return true;
}

void z80_gen::gen_get_value_at(const icode &ic) {
    if (gen_far_get_value_at(ic))
        return;
    auto clear_direct_postinc_load = [&]() {
        direct_postinc_load_pending_ = false;
        direct_postinc_load_cursor_ = operand{};
        direct_postinc_load_old_ptr_ = operand{};
        direct_postinc_load_step_ = 0;
        direct_postinc_load_get_index_ = 0;
    };
    const bool use_pending_word_ptr =
        direct_word_value_pending_ &&
        ic.left.is_temp() &&
        direct_word_value_.is_temp() &&
        ic.left.temp_id == direct_word_value_.temp_id;
    operand direct_word_load_ifx_value;
    int direct_word_result_size = op_size(ic.result);
    if (direct_word_result_size <= 0 &&
        ic.left.type &&
        ic.left.type->base) {
        direct_word_result_size = ic.left.type->base->size();
    }
    if (direct_word_result_size <= 0)
        direct_word_result_size = 2;
    const bool direct_word_forward =
        cur_fn_ &&
        ic.result.is_temp() &&
        direct_word_result_size == 2 &&
        cur_ic_index_ + 1 < cur_fn_->icodes.size() &&
        [&, this]() {
            const auto &next = cur_fn_->icodes[cur_ic_index_ + 1];
            const bool uses_left_temp =
                next.left.is_temp() &&
                next.left.temp_id == ic.result.temp_id;

            switch (next.op) {
            case icode_op::ASSIGN:
            case icode_op::SEND:
            case icode_op::GET_VALUE_AT:
                if (!uses_left_temp)
                    return false;
                break;
            default:
                return false;
            }

            return !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                          ic.result.temp_id);
        }();
    const bool direct_word_load_ifx =
        cur_fn_ &&
        ic.result.is_temp() &&
        direct_word_result_size == 2 &&
        find_direct_word_truth_ifx(ic.result, cur_ic_index_,
                                   direct_word_load_ifx_value);
    if (direct_postinc_load_pending_ &&
        cur_ic_index_ == direct_postinc_load_get_index_) {
        if (ic.left.is_temp() &&
            direct_postinc_load_old_ptr_.is_temp() &&
            ic.left.temp_id == direct_postinc_load_old_ptr_.temp_id &&
            ic.right.is_none()) {
            operand load_target = ic.result;
            operand byte_target;
            bool narrow_to_byte = false;
            auto same_local_symbol = [](const operand &a, const operand &b) {
                return a.kind == operand_kind::SYMBOL &&
                       b.kind == operand_kind::SYMBOL &&
                       !a.is_global &&
                       !b.is_global &&
                       a.stack_offset == b.stack_offset &&
                       a.byte_offset == b.byte_offset &&
                       a.name == b.name;
            };
            int load_size = op_size(ic.result);
            if (load_size <= 0 &&
                ic.left.type &&
                ic.left.type->base) {
                load_size = ic.left.type->base->size();
            }
            if (load_size <= 0)
                load_size = 2;

            if (cur_fn_ && cur_ic_index_ + 1 < cur_fn_->icodes.size()) {
                const auto &next = cur_fn_->icodes[cur_ic_index_ + 1];
                if (next.op == icode_op::ASSIGN &&
                    next.left.is_temp() &&
                    ic.result.is_temp() &&
                    next.left.temp_id == ic.result.temp_id &&
                    !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                           ic.result.temp_id)) {
                    load_target = next.result;
                    skipped_icodes_.insert(cur_ic_index_ + 1);
                } else if (next.op == icode_op::CAST &&
                           next.left.is_temp() &&
                           ic.result.is_temp() &&
                           next.left.temp_id == ic.result.temp_id &&
                           next.result.type &&
                           next.result.type->size() == 1 &&
                           next.result.type->is_integer() &&
                           next.result.type->kind != type_kind::BOOL &&
                           !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                                  ic.result.temp_id)) {
                    narrow_to_byte = true;
                    byte_target = next.result;
                    skipped_icodes_.insert(cur_ic_index_ + 1);

                    if (cur_ic_index_ + 2 < cur_fn_->icodes.size()) {
                        const auto &assign_ic = cur_fn_->icodes[cur_ic_index_ + 2];
                        if (assign_ic.op == icode_op::ASSIGN &&
                            assign_ic.left.is_temp() &&
                            next.result.is_temp() &&
                            assign_ic.left.temp_id == next.result.temp_id &&
                            !temp_value_used_after(*cur_fn_, cur_ic_index_ + 3,
                                                   next.result.temp_id)) {
                            byte_target = assign_ic.result;
                            skipped_icodes_.insert(cur_ic_index_ + 2);
                        }
                    }
                }
            }

            if (narrow_to_byte && cur_fn_) {
                size_t next_idx = cur_ic_index_ + 1;
                while (next_idx < cur_fn_->icodes.size() &&
                       skipped_icodes_.count(next_idx))
                    ++next_idx;
                if (next_idx < cur_fn_->icodes.size()) {
                    const auto &next = cur_fn_->icodes[next_idx];
                    const bool matches_target =
                        (byte_target.is_temp() &&
                         next.left.is_temp() &&
                         next.left.temp_id == byte_target.temp_id) ||
                        same_local_symbol(next.left, byte_target);
                    const bool target_dead_after =
                        byte_target.is_temp()
                            ? !temp_value_used_after(*cur_fn_, next_idx + 1,
                                                     byte_target.temp_id)
                            : !symbol_value_used_after(*cur_fn_, next_idx + 1,
                                                       byte_target);
                    if (next.op == icode_op::ASSIGN &&
                        matches_target &&
                        op_size(next.result) == 1 &&
                        target_dead_after) {
                        byte_target = next.result;
                        skipped_icodes_.insert(next_idx);
                    }
                }
            }

            invalidate_pair_cache();
            invalidate_a_cache();
            load_hl(direct_postinc_load_cursor_);

            if (narrow_to_byte) {
                emit_line("ld\ta, (hl)");
                for (int i = 0; i < direct_postinc_load_step_; ++i)
                    emit_line("inc\thl");
                store_hl(direct_postinc_load_cursor_);
                store_a(byte_target);
            } else if (op_size(load_target) == 1) {
                emit_line("ld\ta, (hl)");
                for (int i = 0; i < direct_postinc_load_step_; ++i)
                    emit_line("inc\thl");
                store_hl(direct_postinc_load_cursor_);
                store_a(load_target);
            } else if (op_size(load_target) == 2 || load_size == 2) {
                emit_line("ld\te, (hl)");
                emit_line("inc\thl");
                emit_line("ld\td, (hl)");
                for (int i = 1; i < direct_postinc_load_step_; ++i)
                    emit_line("inc\thl");
                store_hl(direct_postinc_load_cursor_);
                if (direct_word_load_ifx) {
                    emit_line("ld\ta, d");
                    emit_line("or\ta, e");
                    direct_word_load_ifx_pending_ = true;
                    direct_word_load_ifx_value_ = direct_word_load_ifx_value;
                    direct_word_value_pending_ = true;
                    direct_word_value_ = ic.result;
                } else {
                    store_de(load_target);
                }
            } else {
                clear_direct_postinc_load();
                return;
            }

            clear_direct_postinc_load();
            return;
        }
        clear_direct_postinc_load();
    }
    operand direct_byte_load_ifx_value;
    const bool direct_byte_load_ifx =
        cur_fn_ &&
        ic.result.is_temp() &&
        op_size(ic.result) == 1 &&
        find_direct_byte_truth_ifx(ic.result, cur_ic_index_,
                                   direct_byte_load_ifx_value);

    if (direct_byte_load_ifx) {
        direct_byte_load_ifx_pending_ = true;
        direct_byte_load_ifx_value_ = direct_byte_load_ifx_value;
    }

    const bool direct_mem_copy_shape =
        cur_fn_ &&
        ic.result.is_temp() &&
        (op_size(ic.result) == 1 || op_size(ic.result) == 2) &&
        cur_ic_index_ + 1 < cur_fn_->icodes.size() &&
        cur_fn_->icodes[cur_ic_index_ + 1].op == icode_op::SET_VALUE_AT &&
        cur_fn_->icodes[cur_ic_index_ + 1].left.is_temp() &&
        cur_fn_->icodes[cur_ic_index_ + 1].left.temp_id == ic.result.temp_id;
    const bool direct_mem_copy =
        direct_mem_copy_shape &&
        !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2, ic.result.temp_id);

    if (direct_mem_copy) {
        direct_mem_copy_pending_ = true;
        direct_mem_copy_value_ = ic.result;
        direct_mem_copy_src_ptr_ = ic.left;
        direct_mem_copy_src_index_ = ic.right;
        return;
    }

    auto is_bc_pointer_home = [&](const operand &op) {
        if (op.kind == operand_kind::TEMP) {
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() && it->second == temp_home::main_bc;
        }
        if (op.kind == operand_kind::SYMBOL && !op.is_global) {
            auto it = symbol_regs_.find(symbol_reg_key(op));
            return it != symbol_regs_.end() && it->second == temp_home::main_bc;
        }
        return false;
    };
    if (ic.left.kind == operand_kind::LABEL_REF &&
        !ic.right.is_none() &&
        op_size(ic.result) == 1) {
        const std::string label_sym = asm_label_ref_name(ic.left.name);
        operand byte_src;
        if (get_zero_extended_u8_source(ic.right, byte_src)) {
            load_de_zero_extended_u8(byte_src);
            emit_line("ld\thl, %s", asm_.imm_sym(label_sym).c_str());
            emit_line("add\thl, de");
            emit_line("ld\ta, (hl)");
            if (direct_byte_load_ifx) {
                emit_line("or\ta, a");
            } else {
                store_a(ic.result);
            }
            return;
        }
        load_de(ic.right);
        emit_line("ld\thl, %s", asm_.imm_sym(label_sym).c_str());
        emit_line("add\thl, de");
        emit_line("ld\ta, (hl)");
        if (direct_byte_load_ifx) {
            emit_line("or\ta, a");
        } else {
            store_a(ic.result);
        }
        return;
    }

    auto emit_global_plus_u8_index_hl = [&](const operand &ptr) {
        if (!ptr.is_temp())
            return false;

        const icode *def = find_temp_def_before(ptr.temp_id, cur_ic_index_);
        if (!def || def->op != icode_op::ADD)
            return false;

        const operand *base = nullptr;
        const operand *index = nullptr;
        if (is_direct_array_symbol(def->left)) {
            base = &def->left;
            index = &def->right;
        } else if (is_direct_array_symbol(def->right)) {
            base = &def->right;
            index = &def->left;
        } else {
            return false;
        }

        operand byte_src;
        if (!get_zero_extended_u8_source(*index, byte_src))
            return false;

        load_de_zero_extended_u8(byte_src);
        emit_line("ld\thl, %s", asm_.imm_sym(mangle(base->name)).c_str());
        emit_line("add\thl, de");
        return true;
    };

    if (!use_pending_word_ptr && is_direct_abs_ptr(ic.left)) {
        const std::string abs = "(" + asm_.imm(ic.left.ival) + ")";
        if (op_size(ic.result) == 1) {
            emit_line("ld\ta, %s", abs.c_str());
            if (direct_byte_load_ifx) {
                emit_line("or\ta, a");
            } else {
                store_a(ic.result);
            }
        } else if (op_size(ic.result) > 2) {
            int sz = op_size(ic.result);
            for (int w = 0; w < sz / 2; ++w) {
                const std::string wadd =
                    "(" + asm_.imm(ic.left.ival + (w * 2)) + ")";
                emit_line("ld\thl, %s", wadd.c_str());
                store_hl_word(ic.result, w);
            }
            if (sz & 1) {
                operand tail = ic.result;
                tail.byte_offset += (sz - 1);
                tail.type = type::make_uchar();
                const std::string badd =
                    "(" + asm_.imm(ic.left.ival + (sz - 1)) + ")";
                emit_line("ld\ta, %s", badd.c_str());
                store_a(tail);
            }
        } else {
            emit_line("ld\thl, %s", abs.c_str());
            store_hl(ic.result);
        }
        return;
    }

    if (!use_pending_word_ptr &&
        op_size(ic.result) == 1 &&
        is_bc_pointer_home(ic.left)) {
        emit_line("ld\ta, (bc)");
        if (direct_byte_load_ifx) {
            emit_line("or\ta, a");
        } else {
            store_a(ic.result);
        }
        return;
    }

    if (use_pending_word_ptr) {
        invalidate_pair_cache();
        invalidate_a_cache();
        emit_line("ex\tde, hl");
        direct_word_value_pending_ = false;
        direct_word_value_ = operand{};
    } else if (!emit_global_plus_u8_index_hl(ic.left)) {
        load_hl(ic.left);
    }
    if (op_size(ic.result) == 1) {
        emit_line("ld\ta, (hl)");
        if (direct_byte_load_ifx) {
            emit_line("or\ta, a");
        } else {
            store_a(ic.result);
        }
    } else if (op_size(ic.result) > 2) {
        int sz = op_size(ic.result);
        for (int w = 0; w < sz / 2; ++w) {
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("inc\thl");
            emit_line("push\thl");
            emit_line("ex\tde, hl");
            store_hl_word(ic.result, w);
            emit_line("pop\thl");
        }
        if (sz & 1) {
            operand tail = ic.result;
            tail.byte_offset += (sz - 1);
            tail.type = type::make_uchar();
            emit_line("ld\ta, (hl)");
            store_a(tail);
        }
    } else {
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        if (direct_word_load_ifx) {
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            direct_word_load_ifx_pending_ = true;
            direct_word_load_ifx_value_ = direct_word_load_ifx_value;
            direct_word_value_pending_ = true;
            direct_word_value_ = ic.result;
        } else if (direct_word_forward) {
            direct_word_value_pending_ = true;
            direct_word_value_ = ic.result;
        } else {
            store_de(ic.result);
        }
    }
}

void z80_gen::gen_set_value_at(const icode &ic) {
    if (gen_far_set_value_at(ic))
        return;
    auto is_bc_pointer_home = [&](const operand &op) {
        if (op.kind == operand_kind::TEMP) {
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() && it->second == temp_home::main_bc;
        }
        if (op.kind == operand_kind::SYMBOL && !op.is_global) {
            auto it = symbol_regs_.find(symbol_reg_key(op));
            return it != symbol_regs_.end() && it->second == temp_home::main_bc;
        }
        return false;
    };
    auto byte_load_preserves_hl_here = [&](const operand &op) {
        if (load_byte_preserves_hl(op, temp_regs_))
            return true;
        if (op.kind == operand_kind::SYMBOL && !op.is_global) {
            auto it = incoming_symbol_homes_.find(op.stack_offset);
            if (it != incoming_symbol_homes_.end() &&
                !symbol_value_used_after(*cur_fn_, cur_ic_index_ + 1, op))
                return true;
            int off = ix_offset_of(op);
            return fits_ix_disp(off);
        }
        if (op.kind == operand_kind::TEMP) {
            auto it = temp_regs_.find(op.temp_id);
            if (it != temp_regs_.end()) {
                switch (it->second) {
                case temp_home::main_a:
                case temp_home::main_b:
                case temp_home::main_c:
                case temp_home::alt_a:
                    return true;
                case temp_home::arg_a:
                case temp_home::arg_l:
                    return !temp_value_used_after(*cur_fn_, cur_ic_index_ + 1,
                                                  op.temp_id);
                default:
                    break;
                }
            }
            int off = ix_offset_of(op);
            return fits_ix_disp(off);
        }
        return false;
    };
    auto word_load_preserves_hl_here = [&](const operand &op) {
        if (load_word_preserves_hl(op))
            return true;
        if (op.kind == operand_kind::SYMBOL && !op.is_global) {
            if (symbol_home_in_bc(op))
                return true;
            auto it = incoming_symbol_homes_.find(op.stack_offset);
            if (it != incoming_symbol_homes_.end() &&
                !symbol_value_used_after(*cur_fn_, cur_ic_index_ + 1, op))
                return true;
            int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        }
        if (op.kind == operand_kind::TEMP) {
            auto it = temp_regs_.find(op.temp_id);
            if (it != temp_regs_.end()) {
                switch (it->second) {
                case temp_home::main_bc:
                case temp_home::remat_hl:
                    return true;
                case temp_home::main_hl:
                    return false;
                case temp_home::arg_hl:
                case temp_home::arg_de:
                    return !temp_value_used_after(*cur_fn_, cur_ic_index_ + 1,
                                                  op.temp_id);
                default:
                    break;
                }
            }
            int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        }
        return false;
    };
    if (ic.result.kind == operand_kind::LABEL_REF &&
        !ic.right.is_none() &&
        op_size(ic.left) == 1) {
        const std::string label_sym = asm_label_ref_name(ic.result.name);
        operand byte_src;
        if (get_zero_extended_u8_source(ic.right, byte_src)) {
            load_de_zero_extended_u8(byte_src);
            emit_line("ld\thl, %s", asm_.imm_sym(label_sym).c_str());
            emit_line("add\thl, de");
            if (!byte_load_preserves_hl_here(ic.left))
                emit_line("push\thl");
            load_a(ic.left);
            if (!byte_load_preserves_hl_here(ic.left))
                emit_line("pop\thl");
            emit_line("ld\t(hl), a");
            return;
        }
        load_de(ic.right);
        emit_line("ld\thl, %s", asm_.imm_sym(label_sym).c_str());
        emit_line("add\thl, de");
        if (!byte_load_preserves_hl_here(ic.left))
            emit_line("push\thl");
        load_a(ic.left);
        if (!byte_load_preserves_hl_here(ic.left))
            emit_line("pop\thl");
        emit_line("ld\t(hl), a");
        return;
    }

    auto emit_global_plus_u8_index_hl = [&](const operand &ptr) {
        if (!ptr.is_temp())
            return false;

        const icode *def = find_temp_def_before(ptr.temp_id, cur_ic_index_);
        if (!def || def->op != icode_op::ADD)
            return false;

        const operand *base = nullptr;
        const operand *index = nullptr;
        if (is_direct_array_symbol(def->left)) {
            base = &def->left;
            index = &def->right;
        } else if (is_direct_array_symbol(def->right)) {
            base = &def->right;
            index = &def->left;
        } else {
            return false;
        }

        operand byte_src;
        if (!get_zero_extended_u8_source(*index, byte_src))
            return false;

        load_de_zero_extended_u8(byte_src);
        emit_line("ld\thl, %s", asm_.imm_sym(mangle(base->name)).c_str());
        emit_line("add\thl, de");
        return true;
    };

    const bool direct_mem_copy =
        direct_mem_copy_pending_ &&
        ic.left.is_temp() &&
        direct_mem_copy_value_.is_temp() &&
        ic.left.temp_id == direct_mem_copy_value_.temp_id;
    operand direct_mem_copy_src = direct_mem_copy_src_ptr_;
    operand direct_mem_copy_idx = direct_mem_copy_src_index_;
    direct_mem_copy_pending_ = false;
    direct_mem_copy_value_ = operand{};
    direct_mem_copy_src_ptr_ = operand{};
    direct_mem_copy_src_index_ = operand{};

    if (direct_mem_copy) {
        auto emit_direct_copy_src_hl = [&]() {
            if (!direct_mem_copy_idx.is_none()) {
                if (!emit_global_plus_u8_index_hl(direct_mem_copy_src))
                    load_hl(direct_mem_copy_src);
                emit_line("push\thl");
                operand byte_src;
                if (get_zero_extended_u8_source(direct_mem_copy_idx, byte_src))
                    load_de_zero_extended_u8(byte_src);
                else
                    load_de(direct_mem_copy_idx);
                emit_line("pop\thl");
                emit_line("add\thl, de");
                return;
            }
            if (!emit_global_plus_u8_index_hl(direct_mem_copy_src))
                load_hl(direct_mem_copy_src);
        };

        if (op_size(ic.left) == 1) {
            emit_direct_copy_src_hl();
            emit_line("ld\ta, (hl)");
            if (!emit_global_plus_u8_index_hl(ic.result))
                load_hl(ic.result);
            emit_line("ld\t(hl), a");
            return;
        }
        if (op_size(ic.left) == 2) {
            emit_direct_copy_src_hl();
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            if (!emit_global_plus_u8_index_hl(ic.result))
                load_hl(ic.result);
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            return;
        }
    }

    if (is_direct_abs_ptr(ic.result)) {
        const std::string abs = "(" + asm_.imm(ic.result.ival) + ")";
        if (op_size(ic.left) == 1) {
            load_a(ic.left);
            emit_line("ld\t%s, a", abs.c_str());
        } else if (op_size(ic.left) > 2) {
            int sz = op_size(ic.left);
            for (int w = 0; w < sz / 2; ++w) {
                const std::string wadd =
                    "(" + asm_.imm(ic.result.ival + (w * 2)) + ")";
                load_hl_word(ic.left, w);
                emit_line("ld\t%s, hl", wadd.c_str());
            }
            if (sz & 1) {
                operand tail = ic.left;
                tail.byte_offset += (sz - 1);
                tail.type = type::make_uchar();
                const std::string badd =
                    "(" + asm_.imm(ic.result.ival + (sz - 1)) + ")";
                load_a(tail);
                emit_line("ld\t%s, a", badd.c_str());
            }
        } else {
            load_hl(ic.left);
            emit_line("ld\t%s, hl", abs.c_str());
        }
        return;
    }

    if (op_size(ic.left) == 1 && is_bc_pointer_home(ic.result)) {
        load_a(ic.left);
        emit_line("ld\t(bc), a");
        return;
    }

    if (!emit_global_plus_u8_index_hl(ic.result))
        load_hl(ic.result);
    if (op_size(ic.left) == 1) {
        if (!byte_load_preserves_hl_here(ic.left))
            emit_line("push\thl");
        load_a(ic.left);
        if (!byte_load_preserves_hl_here(ic.left))
            emit_line("pop\thl");
        emit_line("ld\t(hl), a");
    } else if (op_size(ic.left) > 2) {
        int sz = op_size(ic.left);
        int words = sz / 2;
        for (int w = 0; w < words; ++w) {
            emit_line("push\thl");
            load_de_word(ic.left, w);
            emit_line("pop\thl");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            if (w != words - 1 || (sz & 1))
                emit_line("inc\thl");
        }
        if (sz & 1) {
            operand tail = ic.left;
            tail.byte_offset += (sz - 1);
            tail.type = type::make_uchar();
            if (!byte_load_preserves_hl_here(tail))
                emit_line("push\thl");
            load_a(tail);
            if (!byte_load_preserves_hl_here(tail))
                emit_line("pop\thl");
            emit_line("ld\t(hl), a");
        }
    } else {
        if (!word_load_preserves_hl_here(ic.left))
            emit_line("push\thl");
        load_de(ic.left);
        if (!word_load_preserves_hl_here(ic.left))
            emit_line("pop\thl");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
    }
}

} // namespace xcc
