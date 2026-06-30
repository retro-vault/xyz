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

} // namespace

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
    if (op_size(ic.left) == 1) {
        load_a(ic.left);
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
    if (cur_fn_) emit_prologue(*cur_fn_);
}

void z80_gen::gen_endfunction(const icode &) {
    if (cur_fn_) emit_epilogue(*cur_fn_);
}

void z80_gen::gen_return(const icode &ic) {
    if (direct_call_return_pending_ &&
        same_call_result_operand(ic.left, direct_call_return_value_)) {
        direct_call_return_pending_ = false;
        direct_call_return_value_ = operand{};
    } else {
        direct_call_return_pending_ = false;
        direct_call_return_value_ = operand{};
        cur_convention_->emit_return_value(*this, ic.left);
    }
    emit_line("jp\t%s", fn_end_lbl_.c_str());
}

void z80_gen::gen_send(const icode &ic) {
    // Delegate to the CALLEE's convention (not the current function's).
    get_abi_convention(ic.callee_abi).emit_send(*this, ic);
}

void z80_gen::gen_receive(const icode &ic) {
    // Delegate to the current function's convention.
    cur_convention_->emit_receive(*this, ic);
}

void z80_gen::gen_call(const icode &ic) {
    direct_call_return_pending_ = false;
    direct_call_return_value_ = operand{};
    auto &conv = get_abi_convention(ic.callee_abi);
    const bool large_indirect_result =
        !ic.result.is_none() && op_size(ic.result) > 8;

    // Emit the CALL instruction.
    if (!ic.func_name.empty()) {
        std::string callee = mangle(ic.func_name);
        asm_.global_decl(callee);
        emit_line("call\t%s", callee.c_str());
    } else {
        conv.emit_indirect_call(*this, ic);
    }

    bool direct_return = false;
    if (cur_fn_ && !ic.result.is_none() &&
        cur_ic_index_ + 1 < cur_fn_->icodes.size()) {
        const auto &next = cur_fn_->icodes[cur_ic_index_ + 1];
        direct_return = next.op == icode_op::RETURN &&
                        same_call_result_operand(next.left, ic.result);
    }
    if (large_indirect_result)
        direct_return = false;

    if (large_indirect_result) {
        // Large aggregate results currently come back as a pointer into
        // stack-backed storage. Copy them out before caller-side stack cleanup
        // so argument popping cannot trample the pointed result image.
        conv.emit_store_call_result(*this, ic);
        conv.emit_call_cleanup(*this, ic);
    } else {
        // Stack cleanup via the callee's convention (only pops stack-passed
        // args).
        conv.emit_call_cleanup(*this, ic);
        if (direct_return) {
            direct_call_return_pending_ = true;
            direct_call_return_value_ = ic.result;
        } else {
            conv.emit_store_call_result(*this, ic);
        }
    }
}

} // namespace xcc
