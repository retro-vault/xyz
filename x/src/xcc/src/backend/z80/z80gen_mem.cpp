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
                it->second == temp_home::main_d ||
                it->second == temp_home::main_e ||
                it->second == temp_home::arg_a ||
                it->second == temp_home::arg_l);
    }
    return false;
}

void z80_gen::gen_assign(const icode &ic) {
    if (ic.result.is_temp()) {
        auto home = temp_regs_.find(ic.result.temp_id);
        if (home != temp_regs_.end() &&
            home->second == temp_home::remat_hl) {
            return;
        }
    }

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

    // A pair load followed by two indexed stores costs one byte and ten
    // cycles more than storing the constant bytes directly.  Only use this
    // when the following call kills the scratch-pair value: otherwise the
    // ordinary store deliberately leaves the value cached in HL for a later
    // reload.  Keep the size profile's longer form until after sequence
    // outlining, where its common shape may enable a larger module saving.
    bool frame_backed_result = false;
    if (ic.result.kind == operand_kind::SYMBOL &&
        !ic.result.is_global && !ic.result.is_tls &&
        !ic.result.is_sfr && !ic.result.is_func &&
        !ic.result.is_param && !symbol_home_in_bc(ic.result) &&
        symbol_regs_.find(symbol_reg_key(ic.result)) == symbol_regs_.end()) {
        frame_backed_result = true;
    } else if (ic.result.kind == operand_kind::TEMP) {
        const auto home = temp_regs_.find(ic.result.temp_id);
        frame_backed_result = home == temp_regs_.end() ||
                              home->second == temp_home::stack;
    }
    const bool speed_profile = opt_settings_.level == opt_level::Of ||
                               opt_settings_.level == opt_level::O3;
    const bool next_call_clobbers_pair =
        cur_fn_ && cur_ic_index_ + 1 < cur_fn_->icodes.size() &&
        cur_fn_->icodes[cur_ic_index_ + 1].op == icode_op::CALL;
    if (speed_profile && sz == 2 && frame_backed_result &&
        next_call_clobbers_pair &&
        ic.left.kind == operand_kind::INT_CONST) {
        const int off = ix_offset_of(ic.result);
        if (fits_ix_disp(off) && fits_ix_disp(off + 1)) {
            const uint16_t value = static_cast<uint16_t>(ic.left.ival);
            invalidate_pair_cache();
            invalidate_a_cache();
            emit_line("ld\t%s, %s", asm_.ix_rel(off).c_str(),
                      asm_.imm(value & 0xffu).c_str());
            emit_line("ld\t%s, %s", asm_.ix_rel(off + 1).c_str(),
                      asm_.imm(value >> 8).c_str());
            return;
        }
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
        std::string address = mangle(ic.left.name);
        if (ic.left.byte_offset > 0)
            address += " + " + std::to_string(ic.left.byte_offset);
        else if (ic.left.byte_offset < 0)
            address += " - " + std::to_string(-ic.left.byte_offset);
        emit_line("ld\thl, %s", asm_.imm_sym(address).c_str());
    } else {
        int off = (ic.left.is_param
                       ? param_ix_offset(ic.left)
                       : ic.left.stack_offset) +
                  ic.left.byte_offset;
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
    if (ic.result.is_temp()) {
        auto ri = temp_regs_.find(ic.result.temp_id);
        if (ri != temp_regs_.end() &&
            ri->second == temp_home::remat_hl &&
            op_size(ic.result) == 2 &&
            ic.right.is_none() &&
            ic.left.type &&
            ic.left.type->is_ptr() &&
            !ic.left.type->is_far_ptr() &&
            !(ic.result.type && ic.result.type->is_volatile) &&
            !(ic.left.type->base && ic.left.type->base->is_volatile)) {
            return;
        }
    }
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

    // Keep a freshly loaded word in DE when its only consumer masks it with
    // a constant.  The normal lowering stores the load temporary and reloads
    // it for BAND, even though both operations are adjacent.  Folding this
    // general load/use pair avoids that round trip without relying on alias
    // assumptions: volatile reads are excluded and the loaded temporary must
    // be dead immediately after the mask.
    if (cur_fn_ && ic.result.is_temp() && ic.right.is_none() &&
        direct_word_result_size == 2 &&
        !(ic.result.type && ic.result.type->is_volatile) &&
        !(ic.left.type && ic.left.type->base &&
          ic.left.type->base->is_volatile) &&
        cur_ic_index_ + 1 < cur_fn_->icodes.size()) {
        const icode &mask_ic = cur_fn_->icodes[cur_ic_index_ + 1];
        const operand *mask_op = nullptr;
        const bool value_on_left =
            mask_ic.left.is_temp() &&
            mask_ic.left.temp_id == ic.result.temp_id;
        const bool value_on_right =
            mask_ic.right.is_temp() &&
            mask_ic.right.temp_id == ic.result.temp_id;
        if (mask_ic.op == icode_op::BAND && op_size(mask_ic.result) == 2) {
            if (value_on_left && mask_ic.right.kind == operand_kind::INT_CONST)
                mask_op = &mask_ic.right;
            else if (value_on_right &&
                     mask_ic.left.kind == operand_kind::INT_CONST)
                mask_op = &mask_ic.left;
        }

        if (mask_op &&
            !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                   ic.result.temp_id)) {
            const uint16_t mask = static_cast<uint16_t>(mask_op->ival);
            const unsigned low_mask = mask & 0xffu;
            const unsigned high_mask = mask >> 8;

            invalidate_pair_cache();
            invalidate_a_cache();
            load_hl(ic.left);
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");

            auto mask_byte = [&](const char *reg, unsigned byte_mask) {
                if (byte_mask == 0xffu)
                    return;
                if (byte_mask == 0u) {
                    emit_line("ld\t%s, #0", reg);
                    return;
                }
                emit_line("ld\ta, %s", reg);
                emit_line("and\t#%u", byte_mask);
                emit_line("ld\t%s, a", reg);
            };
            mask_byte("e", low_mask);
            mask_byte("d", high_mask);
            store_de(mask_ic.result);
            skipped_icodes_.insert(cur_ic_index_ + 1);
            return;
        }
    }

    // Fold a local read/modify/write chain while the computed address is
    // still in HL:
    //
    //     value = *address;
    //     updated = value +/- 1;
    //     *address = updated;
    //
    // Re-materializing the same data-dependent address for the store is both
    // slower and larger.  Require exact pointer identity, no indexed operand,
    // single-use temporaries, and non-volatile memory before consuming the
    // following two IR instructions.
    if (cur_fn_ && ic.result.is_temp() && ic.left.is_temp() &&
        ic.right.is_none() &&
        (direct_word_result_size == 1 || direct_word_result_size == 2) &&
        !(ic.result.type && ic.result.type->is_volatile) &&
        !(ic.left.type && ic.left.type->base &&
          ic.left.type->base->is_volatile) &&
        cur_ic_index_ + 2 < cur_fn_->icodes.size()) {
        const icode &update = cur_fn_->icodes[cur_ic_index_ + 1];
        const icode &store = cur_fn_->icodes[cur_ic_index_ + 2];
        const bool value_on_left =
            update.left.is_temp() &&
            update.left.temp_id == ic.result.temp_id &&
            update.right.kind == operand_kind::INT_CONST;
        const bool value_on_right =
            update.op == icode_op::ADD && update.right.is_temp() &&
            update.right.temp_id == ic.result.temp_id &&
            update.left.kind == operand_kind::INT_CONST;
        const int64_t delta =
            value_on_left ? update.right.ival
                          : (value_on_right ? update.left.ival : 0);
        const bool small_update =
            ((update.op == icode_op::ADD) ||
             (update.op == icode_op::SUB && value_on_left)) &&
            delta >= 1 && delta <= 3;
        const bool same_store_address =
            store.op == icode_op::SET_VALUE_AT && store.result.is_temp() &&
            store.result.temp_id == ic.left.temp_id && store.right.is_none();
        const bool stores_update =
            update.result.is_temp() && store.left.is_temp() &&
            store.left.temp_id == update.result.temp_id;
        if (small_update && same_store_address && stores_update &&
            !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                   ic.result.temp_id) &&
            !temp_value_used_after(*cur_fn_, cur_ic_index_ + 3,
                                   update.result.temp_id)) {
            invalidate_pair_cache();
            invalidate_a_cache();
            load_hl(ic.left);
            if (direct_word_result_size == 1) {
                for (int64_t step = 0; step < delta; ++step)
                    emit_line(update.op == icode_op::ADD ? "inc\t(hl)"
                                                         : "dec\t(hl)");
            } else {
                emit_line("ld\te, (hl)");
                emit_line("inc\thl");
                emit_line("ld\td, (hl)");
                emit_line("dec\thl");
                for (int64_t step = 0; step < delta; ++step)
                    emit_line(update.op == icode_op::ADD ? "inc\tde"
                                                         : "dec\tde");
                emit_line("ld\t(hl), e");
                emit_line("inc\thl");
                emit_line("ld\t(hl), d");
            }
            skipped_icodes_.insert(cur_ic_index_ + 1);
            skipped_icodes_.insert(cur_ic_index_ + 2);
            return;
        }
    }

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
    auto is_iy_pointer_home = [&](const operand &op) {
        if (!op.is_temp())
            return false;
        auto it = temp_regs_.find(op.temp_id);
        return it != temp_regs_.end() && it->second == temp_home::main_iy;
    };
    auto iy_pointer_displacement = [&](const operand &op, int size,
                                       int64_t &disp) {
        if (is_iy_pointer_home(op)) {
            disp = 0;
            return true;
        }
        if (!op.is_temp())
            return false;
        const icode *def = find_temp_def_before(op.temp_id, cur_ic_index_);
        if (!def || def->op != icode_op::ADD)
            return false;
        const operand *base = &def->left;
        const operand *offset = &def->right;
        if (base->kind == operand_kind::INT_CONST)
            std::swap(base, offset);
        if (!is_iy_pointer_home(*base) ||
            offset->kind != operand_kind::INT_CONST ||
            offset->ival < -128 || offset->ival + size - 1 > 127)
            return false;
        disp = offset->ival;
        return true;
    };
    if (ic.left.kind == operand_kind::LABEL_REF &&
        !ic.right.is_none() &&
        (op_size(ic.result) == 1 || op_size(ic.result) == 2)) {
        const std::string label_sym = asm_label_ref_name(ic.left.name);
        operand byte_src;
        if (get_zero_extended_u8_source(ic.right, byte_src)) {
            load_de_zero_extended_u8(byte_src);
        } else {
            load_de(ic.right);
        }
        emit_line("ld\thl, %s", asm_.imm_sym(label_sym).c_str());
        emit_line("add\thl, de");
        if (op_size(ic.result) == 1) {
            emit_line("ld\ta, (hl)");
            if (direct_byte_load_ifx) {
                emit_line("or\ta, a");
            } else {
                store_a(ic.result);
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

    int64_t iy_disp = 0;
    if (!use_pending_word_ptr &&
        op_size(ic.result) == 1 &&
        iy_pointer_displacement(ic.left, 1, iy_disp)) {
        emit_line("ld\ta, %lld(iy)", static_cast<long long>(iy_disp));
        if (direct_byte_load_ifx) {
            emit_line("or\ta, a");
        } else {
            store_a(ic.result);
        }
        return;
    }

    if (!use_pending_word_ptr &&
        op_size(ic.result) == 2 &&
        iy_pointer_displacement(ic.left, 2, iy_disp)) {
        const auto result_home = ic.result.is_temp()
                                     ? temp_regs_.find(ic.result.temp_id)
                                     : temp_regs_.end();
        const bool direct_to_bc =
            result_home != temp_regs_.end() &&
            result_home->second == temp_home::main_bc &&
            !direct_word_load_ifx && !direct_word_forward;
        if (direct_to_bc) {
            // Honour the allocator's destination pair instead of loading
            // through DE and copying the value afterwards.  Besides saving
            // two moves, this leaves DE available for an independent byte or
            // word live range in pointer-walking loops.
            emit_line("ld\tc, %lld(iy)", static_cast<long long>(iy_disp));
            emit_line("ld\tb, %lld(iy)", static_cast<long long>(iy_disp + 1));
        } else {
            emit_line("ld\te, %lld(iy)", static_cast<long long>(iy_disp));
            emit_line("ld\td, %lld(iy)", static_cast<long long>(iy_disp + 1));
        }
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
        } else if (!direct_to_bc) {
            store_de(ic.result);
        }
        return;
    }

    if (!use_pending_word_ptr &&
        op_size(ic.result) == 4 &&
        iy_pointer_displacement(ic.left, 4, iy_disp)) {
        emit_line("ld\te, %lld(iy)", static_cast<long long>(iy_disp));
        emit_line("ld\td, %lld(iy)", static_cast<long long>(iy_disp + 1));
        store_de_word(ic.result, 0);
        emit_line("ld\te, %lld(iy)", static_cast<long long>(iy_disp + 2));
        emit_line("ld\td, %lld(iy)", static_cast<long long>(iy_disp + 3));
        store_de_word(ic.result, 1);
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

void z80_gen::gen_block_fill(const icode &ic) {
    if (ic.right.kind != operand_kind::INT_CONST || ic.right.ival <= 0)
        return;

    const int64_t count = ic.right.ival;
    load_hl(ic.result);
    if (ic.left.kind == operand_kind::INT_CONST) {
        load_a(ic.left);
    } else {
        emit_line("push\thl");
        load_a(ic.left);
        emit_line("pop\thl");
    }
    emit_line("ld\t(hl), a");

    if (count > 1) {
        emit_line("ld\td, h");
        emit_line("ld\te, l");
        emit_line("inc\tde");
        emit_line("ld\tbc, %s",
                  asm_.imm(count - 1).c_str());
        emit_line("ldir");
    }
    invalidate_pair_cache();
    invalidate_a_cache();
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
    auto is_iy_pointer_home = [&](const operand &op) {
        if (!op.is_temp())
            return false;
        auto it = temp_regs_.find(op.temp_id);
        return it != temp_regs_.end() && it->second == temp_home::main_iy;
    };
    auto iy_pointer_displacement = [&](const operand &op, int size,
                                       int64_t &disp) {
        if (is_iy_pointer_home(op)) {
            disp = 0;
            return true;
        }
        if (!op.is_temp())
            return false;
        const icode *def = find_temp_def_before(op.temp_id, cur_ic_index_);
        if (!def || def->op != icode_op::ADD)
            return false;
        const operand *base = &def->left;
        const operand *offset = &def->right;
        if (base->kind == operand_kind::INT_CONST)
            std::swap(base, offset);
        if (!is_iy_pointer_home(*base) ||
            offset->kind != operand_kind::INT_CONST ||
            offset->ival < -128 || offset->ival + size - 1 > 127)
            return false;
        disp = offset->ival;
        return true;
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
                case temp_home::main_d:
                case temp_home::main_e:
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
        (op_size(ic.left) == 1 || op_size(ic.left) == 2)) {
        const std::string label_sym = asm_label_ref_name(ic.result.name);
        operand byte_src;
        if (get_zero_extended_u8_source(ic.right, byte_src)) {
            load_de_zero_extended_u8(byte_src);
        } else {
            load_de(ic.right);
        }
        emit_line("ld\thl, %s", asm_.imm_sym(label_sym).c_str());
        emit_line("add\thl, de");
        if (op_size(ic.left) == 1) {
            const bool preserves_hl = byte_load_preserves_hl_here(ic.left);
            if (!preserves_hl)
                emit_line("push\thl");
            load_a(ic.left);
            if (!preserves_hl)
                emit_line("pop\thl");
            emit_line("ld\t(hl), a");
        } else {
            const bool preserves_hl = word_load_preserves_hl_here(ic.left);
            if (!preserves_hl)
                emit_line("push\thl");
            load_de(ic.left);
            if (!preserves_hl)
                emit_line("pop\thl");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
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
        auto emit_direct_copy_dst_hl = [&]() {
            if (!emit_global_plus_u8_index_hl(ic.result))
                load_hl(ic.result);
        };

        if (op_size(ic.left) == 1) {
            int64_t src_disp = 0;
            const bool src_in_iy =
                direct_mem_copy_idx.is_none() &&
                iy_pointer_displacement(direct_mem_copy_src, 1, src_disp);
            int64_t dst_disp = 0;
            const bool dst_in_iy =
                iy_pointer_displacement(ic.result, 1, dst_disp);

            // Form an unrelated destination before reading an IY cursor.
            // Rematerializing base[byte_index] commonly uses A as scratch,
            // so loading the source first would silently replace it with the
            // index before the store.
            if (src_in_iy && !dst_in_iy) {
                emit_direct_copy_dst_hl();
                emit_line("ld\ta, %lld(iy)",
                          static_cast<long long>(src_disp));
                emit_line("ld\t(hl), a");
                return;
            }

            if (src_in_iy) {
                emit_line("ld\ta, %lld(iy)",
                          static_cast<long long>(src_disp));
            } else {
                emit_direct_copy_src_hl();
                emit_line("ld\ta, (hl)");
            }
            if (dst_in_iy) {
                emit_line("ld\t%lld(iy), a",
                          static_cast<long long>(dst_disp));
            } else {
                emit_line("push\taf");
                emit_direct_copy_dst_hl();
                emit_line("pop\taf");
                emit_line("ld\t(hl), a");
            }
            return;
        }
        if (op_size(ic.left) == 2) {
            int64_t src_disp = 0;
            const bool src_in_iy =
                direct_mem_copy_idx.is_none() &&
                iy_pointer_displacement(direct_mem_copy_src, 2, src_disp);
            int64_t dst_disp = 0;
            const bool dst_in_iy =
                iy_pointer_displacement(ic.result, 2, dst_disp);

            if (src_in_iy && !dst_in_iy) {
                emit_direct_copy_dst_hl();
                emit_line("ld\te, %lld(iy)",
                          static_cast<long long>(src_disp));
                emit_line("ld\td, %lld(iy)",
                          static_cast<long long>(src_disp + 1));
                emit_line("ld\t(hl), e");
                emit_line("inc\thl");
                emit_line("ld\t(hl), d");
                return;
            }

            if (src_in_iy) {
                emit_line("ld\te, %lld(iy)",
                          static_cast<long long>(src_disp));
                emit_line("ld\td, %lld(iy)",
                          static_cast<long long>(src_disp + 1));
            } else {
                emit_direct_copy_src_hl();
                emit_line("ld\te, (hl)");
                emit_line("inc\thl");
                emit_line("ld\td, (hl)");
            }
            if (dst_in_iy) {
                emit_line("ld\t%lld(iy), e",
                          static_cast<long long>(dst_disp));
                emit_line("ld\t%lld(iy), d",
                          static_cast<long long>(dst_disp + 1));
            } else {
                emit_line("push\tde");
                emit_direct_copy_dst_hl();
                emit_line("pop\tde");
                emit_line("ld\t(hl), e");
                emit_line("inc\thl");
                emit_line("ld\t(hl), d");
            }
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

    int64_t iy_disp = 0;
    if (op_size(ic.left) == 1 &&
        iy_pointer_displacement(ic.result, 1, iy_disp)) {
        load_a(ic.left);
        emit_line("ld\t%lld(iy), a", static_cast<long long>(iy_disp));
        return;
    }

    if (op_size(ic.left) == 2 &&
        iy_pointer_displacement(ic.result, 2, iy_disp)) {
        load_de(ic.left);
        emit_line("ld\t%lld(iy), e", static_cast<long long>(iy_disp));
        emit_line("ld\t%lld(iy), d", static_cast<long long>(iy_disp + 1));
        return;
    }

    if (op_size(ic.left) == 4 &&
        iy_pointer_displacement(ic.result, 4, iy_disp)) {
        load_de_word(ic.left, 0);
        emit_line("ld\t%lld(iy), e", static_cast<long long>(iy_disp));
        emit_line("ld\t%lld(iy), d", static_cast<long long>(iy_disp + 1));
        load_de_word(ic.left, 1);
        emit_line("ld\t%lld(iy), e", static_cast<long long>(iy_disp + 2));
        emit_line("ld\t%lld(iy), d", static_cast<long long>(iy_disp + 3));
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
