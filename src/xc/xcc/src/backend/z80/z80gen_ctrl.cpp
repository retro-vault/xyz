//
// z80gen_ctrl.cpp — Control-flow icode handlers: label, goto, ifx,
//                   function/endfunction, return, send, receive, call.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"

namespace xcc {

void z80_gen::gen_label(const icode &ic) {
    asm_.label(ic.label_name, false);
}

void z80_gen::gen_goto(const icode &ic) {
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
    if (cur_fn_) emit_prologue(*cur_fn_);
}

void z80_gen::gen_endfunction(const icode &) {
    if (cur_fn_) emit_epilogue(*cur_fn_);
}

void z80_gen::gen_return(const icode &ic) {
    cur_convention_->emit_return_value(*this, ic.left);
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
    auto &conv = get_abi_convention(ic.callee_abi);

    // Emit the CALL instruction.
    if (!ic.func_name.empty()) {
        std::string callee = mangle(ic.func_name);
        asm_.global_decl(callee);
        emit_line("call\t%s", callee.c_str());
    } else {
        conv.emit_indirect_call(*this, ic);
    }

    // Stack cleanup via the callee's convention (only pops stack-passed args).
    conv.emit_call_cleanup(*this, ic);

    // Collect return value.
    conv.emit_store_call_result(*this, ic);
}

} // namespace xcc
