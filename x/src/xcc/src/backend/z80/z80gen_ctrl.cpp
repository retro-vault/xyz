//
// z80gen_ctrl.cpp — Control-flow icode handlers: label, goto, ifx,
//                   function/endfunction, return, send, receive, call.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"

namespace xcc {

namespace {

bool same_call_result_operand(const operand &a, const operand &b) {
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

bool supports_direct_call_ifx(call_abi abi, int size) {
    if (effective_call_abi(abi) != call_abi::SDCCCALL1)
        return false;
    return size == 1 || size == 2;
}

bool compatible_direct_return_abis(call_abi caller, call_abi callee) {
    caller = effective_call_abi(caller);
    callee = effective_call_abi(callee);
    if (caller == callee)
        return true;

    auto return_family = [](call_abi abi) {
        switch (abi) {
        case call_abi::SDCCCALL0:
        case call_abi::Z88DK_CALLEE:
            return 1; // Legacy byte/word results use L/HL.
        case call_abi::SDCCCALL1:
        case call_abi::Z88DK_SMALLC:
        case call_abi::Z88DK_FASTCALL:
            return 2; // Modern byte/word results use A/DE.
        default:
            return 0;
        }
    };

    const int caller_family = return_family(caller);
    return caller_family != 0 && caller_family == return_family(callee);
}

bool is_truth_test_preserving_integer_cast(const icode &ic) {
    if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
        return false;
    if (ic.left.type->is_far_ptr() || ic.result.type->is_far_ptr())
        return false;
    const bool src_ok = ic.left.type->is_integer() || ic.left.type->is_ptr();
    const bool dst_ok = ic.result.type->is_integer() || ic.result.type->is_ptr();
    return src_ok && dst_ok;
}

} // namespace

bool z80_gen::try_finish_direct_hl_return(const operand &result) {
    if (!cur_fn_ || !result.is_temp() || !op_is_16bit(result))
        return false;
    if (cur_ic_index_ + 1 >= cur_fn_->icodes.size())
        return false;

    const icode &next = cur_fn_->icodes[cur_ic_index_ + 1];
    if (next.op != icode_op::RETURN ||
        !same_call_result_operand(next.left, result) ||
        temp_value_used_after(*cur_fn_, cur_ic_index_ + 2, result.temp_id)) {
        return false;
    }

    switch (effective_call_abi(cur_fn_->abi)) {
    case call_abi::SDCCCALL1:
    case call_abi::Z88DK_SMALLC:
    case call_abi::Z88DK_FASTCALL:
        emit_line("ex\tde, hl");
        break;
    case call_abi::SDCCCALL0:
    case call_abi::Z88DK_CALLEE:
    case call_abi::NAKED:
    case call_abi::INTERRUPT:
    case call_abi::CRITICAL:
        break;
    default:
        return false;
    }

    direct_compare_return_pending_ = false;
    direct_compare_return_value_ = operand{};
    direct_call_return_pending_ = true;
    direct_call_return_value_ = result;
    direct_word_value_pending_ = false;
    direct_word_value_ = operand{};
    return true;
}

void z80_gen::gen_label(const icode &ic) {
    emit_label(ic.label_name, false);
}

void z80_gen::gen_goto(const icode &ic) {
    bool local_target = ic.label_name == fn_end_lbl_;
    if (!local_target && cur_fn_) {
        for (const auto &fn_ic : cur_fn_->icodes) {
            if (fn_ic.op == icode_op::LABEL && fn_ic.label_name == ic.label_name) {
                local_target = true;
                break;
            }
        }
    }

    const bool tail_jumps_to_external_symbol =
        cur_fn_ &&
        !local_target &&
        cur_ic_index_ + 1 < cur_fn_->icodes.size() &&
        cur_fn_->icodes[cur_ic_index_ + 1].op == icode_op::ENDFUNCTION;

    if (tail_jumps_to_external_symbol) {
        asm_.global_decl(ic.label_name);
        emit_line("call\t%s", ic.label_name.c_str());
        return;
    }
    emit_line("jp\t%s", ic.label_name.c_str());
}

void z80_gen::gen_ifx(const icode &ic) {
    auto byte_truth_source = [&](const operand &cond) -> std::optional<operand> {
        if (cond.type && cond.type->size() == 1)
            return cond;
        if (!cur_fn_ || !cond.is_temp())
            return std::nullopt;

        const icode *def = nullptr;
        for (const auto &fn_ic : cur_fn_->icodes) {
            if (!fn_ic.result.is_temp() || fn_ic.result.temp_id != cond.temp_id)
                continue;
            if (def)
                return std::nullopt;
            def = &fn_ic;
        }
        if (!def)
            return std::nullopt;

        if (def->result.type && def->result.type->size() == 1)
            return cond;

        if (def->op != icode_op::CAST ||
            !def->left.type || def->left.type->size() != 1 ||
            !def->result.type ||
            !(def->result.type->is_integer() || def->result.type->is_ptr()) ||
            def->result.type->is_far_ptr()) {
            return std::nullopt;
        }

        operand narrowed = def->left;
        narrowed.byte_offset += cond.byte_offset;
        return narrowed;
    };

    const bool direct_byte_load_ifx =
        direct_byte_load_ifx_pending_ &&
        ic.left.is_temp() &&
        direct_byte_load_ifx_value_.is_temp() &&
        ic.left.temp_id == direct_byte_load_ifx_value_.temp_id;
    direct_byte_load_ifx_pending_ = false;
    direct_byte_load_ifx_value_ = operand{};

    const bool direct_word_load_ifx =
        direct_word_load_ifx_pending_ &&
        ic.left.is_temp() &&
        direct_word_load_ifx_value_.is_temp() &&
        ic.left.temp_id == direct_word_load_ifx_value_.temp_id;
    direct_word_load_ifx_pending_ = false;
    direct_word_load_ifx_value_ = operand{};

    const bool direct_call_ifx =
        direct_call_ifx_pending_ &&
        same_call_result_operand(ic.left, direct_call_ifx_value_);
    const call_abi direct_ifx_abi = direct_call_ifx_abi_;
    const int direct_ifx_reg_size = direct_call_ifx_reg_size_;
    const bool keep_direct_call_word = direct_call_ifx_keep_word_pending_;
    direct_call_ifx_pending_ = false;
    direct_call_ifx_value_ = operand{};
    direct_call_ifx_abi_ = call_abi::DEFAULT;
    direct_call_ifx_reg_size_ = 0;
    direct_call_ifx_keep_word_pending_ = false;

    if (direct_byte_load_ifx) {
        // Producer already loaded the byte and prepared flags.
    } else if (direct_word_load_ifx) {
        // Producer already loaded the word in DE and prepared flags.
    } else if (direct_call_ifx &&
        effective_call_abi(direct_ifx_abi) == call_abi::SDCCCALL1) {
        if (direct_ifx_reg_size == 1) {
            emit_line("or\ta, a");
        } else {
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            if (keep_direct_call_word) {
                direct_word_value_pending_ = true;
                direct_word_value_ = ic.left;
            }
        }
    } else if (auto byte_src = byte_truth_source(ic.left)) {
        load_a(*byte_src);
        emit_line("or\ta, a");
    } else {
        load_hl(ic.left);
        emit_line("ld\ta, h");
        emit_line("or\ta, l");
    }
    if (!ic.true_lbl.empty())
        emit_line("jp\tnz, %s", ic.true_lbl.c_str());
    if (!ic.false_lbl.empty())
        emit_line("jp\t%s", ic.false_lbl.c_str());
}

void z80_gen::gen_function(const icode &) {
    direct_call_return_pending_ = false;
    direct_call_return_value_ = operand{};
    sibling_tail_call_pending_ = false;
    sibling_tail_call_value_ = operand{};
    last_frameless_return_terminated_ = false;
    direct_compare_return_pending_ = false;
    direct_compare_return_value_ = operand{};
    direct_call_ifx_pending_ = false;
    direct_call_ifx_value_ = operand{};
    direct_call_ifx_abi_ = call_abi::DEFAULT;
    direct_call_ifx_reg_size_ = 0;
    direct_call_ifx_keep_word_pending_ = false;
    if (cur_fn_) emit_prologue(*cur_fn_);
}

void z80_gen::gen_endfunction(const icode &) {
    if (cur_fn_ && !last_frameless_return_terminated_)
        emit_epilogue(*cur_fn_);
}

void z80_gen::gen_return(const icode &ic) {
    if (sibling_tail_call_pending_ &&
        (ic.left.is_none() ||
         same_call_result_operand(ic.left, sibling_tail_call_value_))) {
        sibling_tail_call_pending_ = false;
        sibling_tail_call_value_ = operand{};
        last_frameless_return_terminated_ = true;
        return;
    }

    if (direct_compare_return_pending_ &&
        same_call_result_operand(ic.left, direct_compare_return_value_)) {
        direct_compare_return_pending_ = false;
        direct_compare_return_value_ = operand{};
        direct_call_return_pending_ = false;
        direct_call_return_value_ = operand{};
    } else if (direct_call_return_pending_ &&
        same_call_result_operand(ic.left, direct_call_return_value_)) {
        direct_call_return_pending_ = false;
        direct_call_return_value_ = operand{};
    } else {
        direct_compare_return_pending_ = false;
        direct_compare_return_value_ = operand{};
        direct_call_return_pending_ = false;
        direct_call_return_value_ = operand{};
        cur_convention_->emit_return_value(*this, ic.left);
    }
    direct_compare_return_pending_ = false;
    direct_compare_return_value_ = operand{};
    direct_call_ifx_pending_ = false;
    direct_call_ifx_value_ = operand{};
    direct_call_ifx_abi_ = call_abi::DEFAULT;
    direct_call_ifx_reg_size_ = 0;
    direct_call_ifx_keep_word_pending_ = false;
    if (cur_fn_ && !debug_ && can_omit_frame_pointer(*cur_fn_)) {
        emit_line("ret");
        last_frameless_return_terminated_ = true;
    } else {
        emit_line("jp\t%s", fn_end_lbl_.c_str());
    }
}

void z80_gen::gen_send(const icode &ic) {
    icode send_ic = ic;
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

    if (direct_widen_send_pending_ &&
        ic.left.is_temp() &&
        direct_widen_send_value_.is_temp() &&
        ic.left.temp_id == direct_widen_send_value_.temp_id &&
        (ic.arg_loc == abi_arg_loc::REG_HL || ic.arg_loc == abi_arg_loc::REG_DE)) {
        send_ic.left = direct_widen_send_source_;
    } else if (direct_word_value_pending_ &&
               ic.left.is_temp() &&
               direct_word_value_.is_temp() &&
               ic.left.temp_id == direct_word_value_.temp_id &&
               op_size(ic.left) == 2) {
        if (ic.arg_loc == abi_arg_loc::REG_HL) {
            invalidate_pair_cache();
            invalidate_a_cache();
            emit_line("ex\tde, hl");
            direct_word_value_pending_ = false;
            direct_word_value_ = operand{};
            return;
        }
        if (ic.arg_loc == abi_arg_loc::REG_DE) {
            direct_word_value_pending_ = false;
            direct_word_value_ = operand{};
            return;
        }
    } else if ((ic.arg_loc == abi_arg_loc::REG_HL ||
                ic.arg_loc == abi_arg_loc::REG_DE) &&
               ic.left.is_temp()) {
        if (const icode *def = find_temp_def_before(ic.left.temp_id, cur_ic_index_)) {
            if (def->op == icode_op::CAST &&
                op_size(def->left) == 1 &&
                op_size(def->result) == 2 &&
                source_safe_for_delayed_byte_remat(def->left)) {
                send_ic.left = def->left;
            }
        }
    }
    direct_widen_send_pending_ = false;
    direct_widen_send_value_ = operand{};
    direct_widen_send_source_ = operand{};
    // Delegate to the CALLEE's convention (not the current function's).
    get_abi_convention(send_ic.callee_abi).emit_send(*this, send_ic);
}

void z80_gen::gen_receive(const icode &ic) {
    // Delegate to the current function's convention.
    cur_convention_->emit_receive(*this, ic);
}

void z80_gen::gen_call(const icode &ic) {
    direct_call_return_pending_ = false;
    direct_call_return_value_ = operand{};
    direct_compare_return_pending_ = false;
    direct_compare_return_value_ = operand{};
    auto &conv = get_abi_convention(ic.callee_abi);
    const bool large_indirect_result =
        !ic.result.is_none() && op_size(ic.result) > 8;

    bool sibling_tail_call = false;
    if (cur_fn_ && !debug_ && !ic.func_name.empty() &&
        ic.arg_bytes == 0 && !large_indirect_result &&
        can_omit_frame_pointer(*cur_fn_) &&
        effective_call_abi(cur_fn_->abi) ==
            effective_call_abi(ic.callee_abi) &&
        cur_ic_index_ + 2 < cur_fn_->icodes.size()) {
        const auto &ret = cur_fn_->icodes[cur_ic_index_ + 1];
        const auto &end = cur_fn_->icodes[cur_ic_index_ + 2];
        sibling_tail_call =
            ret.op == icode_op::RETURN &&
            end.op == icode_op::ENDFUNCTION &&
            ((ic.result.is_none() && ret.left.is_none()) ||
             (!ic.result.is_none() &&
              same_call_result_operand(ret.left, ic.result)));
    }

    if (sibling_tail_call) {
        std::string callee = mangle(ic.func_name);
        asm_.global_decl(callee);
        emit_line("jp\t%s", callee.c_str());
        sibling_tail_call_pending_ = true;
        sibling_tail_call_value_ = ic.result;
        return;
    }

    // Emit the CALL instruction.
    if (!ic.func_name.empty()) {
        std::string callee = mangle(ic.func_name);
        asm_.global_decl(callee);
        emit_line("call\t%s", callee.c_str());
    } else {
        conv.emit_indirect_call(*this, ic);
    }

    // When the callee pops stack-passed arguments, the machine SP has already
    // advanced on return even though we emit no caller-side cleanup sequence.
    // Keep the cached SP-vs-IX delta in sync so deep frame accesses continue to
    // use the right sp-relative offset after optimized calls.
    if (ic.callee_cleans_stack && ic.arg_bytes > 0 && has_known_sp_ix_delta()) {
        set_known_sp_ix_delta(current_sp_ix_delta() + ic.arg_bytes);
    }

    bool direct_return = false;
    bool direct_ifx = false;
    int direct_ifx_reg_size = 0;
    bool keep_direct_ifx_word_pending = false;
    const bool drop_unused_result =
        cur_fn_ &&
        ic.result.is_temp() &&
        !temp_value_used_after(*cur_fn_, cur_ic_index_ + 1, ic.result.temp_id);
    auto has_direct_call_ifx_fallthrough_consumer =
        [&](size_t ifx_index, const operand &value) {
            if (!cur_fn_ || !value.is_temp() ||
                ifx_index + 2 >= cur_fn_->icodes.size()) {
                return false;
            }

            const auto &ifx = cur_fn_->icodes[ifx_index];
            if (ifx.op != icode_op::IFX ||
                !same_call_result_operand(ifx.left, value) ||
                ifx.true_lbl.empty()) {
                return false;
            }

            const auto &true_label = cur_fn_->icodes[ifx_index + 1];
            if (true_label.op != icode_op::LABEL ||
                true_label.label_name != ifx.true_lbl) {
                return false;
            }

            size_t consumer_idx = ifx_index + 2;
            while (consumer_idx < cur_fn_->icodes.size() &&
                   cur_fn_->icodes[consumer_idx].op == icode_op::LABEL) {
                ++consumer_idx;
            }
            if (consumer_idx >= cur_fn_->icodes.size())
                return false;

            const auto &consumer = cur_fn_->icodes[consumer_idx];
            const bool direct_return =
                consumer.op == icode_op::RETURN &&
                same_call_result_operand(consumer.left, value);
            const bool direct_send =
                consumer.op == icode_op::SEND &&
                same_call_result_operand(consumer.left, value) &&
                (consumer.arg_loc == abi_arg_loc::REG_HL ||
                 consumer.arg_loc == abi_arg_loc::REG_DE);
            if (!direct_return && !direct_send) {
                return false;
            }

            return !temp_value_used_after(*cur_fn_, consumer_idx + 1,
                                          value.temp_id);
        };
    if (cur_fn_ && !ic.result.is_none() &&
        cur_ic_index_ + 1 < cur_fn_->icodes.size()) {
        const auto &next = cur_fn_->icodes[cur_ic_index_ + 1];
        direct_return = compatible_direct_return_abis(cur_fn_->abi,
                                                       ic.callee_abi) &&
                        next.op == icode_op::RETURN &&
                        same_call_result_operand(next.left, ic.result);
        direct_ifx = ic.result.is_temp() &&
                     next.op == icode_op::IFX &&
                     same_call_result_operand(next.left, ic.result) &&
                     supports_direct_call_ifx(ic.callee_abi, op_size(ic.result)) &&
                     (!temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                             ic.result.temp_id) ||
                      (op_size(ic.result) == 2 &&
                       has_direct_call_ifx_fallthrough_consumer(
                           cur_ic_index_ + 1, ic.result)));
        if (direct_ifx)
            direct_ifx_reg_size = op_size(ic.result);
        if (direct_ifx && op_size(ic.result) == 2) {
            keep_direct_ifx_word_pending =
                has_direct_call_ifx_fallthrough_consumer(
                    cur_ic_index_ + 1, ic.result);
        }
        if (!direct_ifx &&
            ic.result.is_temp() &&
            next.op == icode_op::CAST &&
            same_call_result_operand(next.left, ic.result) &&
            is_truth_test_preserving_integer_cast(next) &&
            supports_direct_call_ifx(ic.callee_abi, op_size(ic.result)) &&
            next.result.is_temp() &&
            cur_ic_index_ + 2 < cur_fn_->icodes.size()) {
            const auto &ifx = cur_fn_->icodes[cur_ic_index_ + 2];
            direct_ifx =
                ifx.op == icode_op::IFX &&
                same_call_result_operand(ifx.left, next.result) &&
                !temp_value_used_after(*cur_fn_, cur_ic_index_ + 2,
                                       ic.result.temp_id) &&
                !temp_value_used_after(*cur_fn_, cur_ic_index_ + 3,
                                       next.result.temp_id);
            if (direct_ifx)
                direct_ifx_reg_size = op_size(ic.result);
        }
    }
    if (large_indirect_result)
        direct_return = false;
    if (large_indirect_result)
        direct_ifx = false;

    if (large_indirect_result) {
        // Large aggregate results currently come back as a pointer into
        // stack-backed storage. Copy them out before caller-side stack cleanup
        // so argument popping cannot trample the pointed result image.
        if (!drop_unused_result)
            conv.emit_store_call_result(*this, ic);
        conv.emit_call_cleanup(*this, ic);
    } else {
        // Stack cleanup via the callee's convention (only pops stack-passed
        // args).
        conv.emit_call_cleanup(*this, ic);
        if (direct_return) {
            direct_call_return_pending_ = true;
            direct_call_return_value_ = ic.result;
        } else if (direct_ifx) {
            direct_call_ifx_pending_ = true;
            direct_call_ifx_value_ = ic.result;
            direct_call_ifx_abi_ = ic.callee_abi;
            direct_call_ifx_reg_size_ = direct_ifx_reg_size;
            direct_call_ifx_keep_word_pending_ = keep_direct_ifx_word_pending;
        } else if (drop_unused_result) {
            return;
        } else {
            conv.emit_store_call_result(*this, ic);
        }
    }
}

} // namespace xcc
