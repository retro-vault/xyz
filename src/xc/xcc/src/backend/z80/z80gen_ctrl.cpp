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
    if (!ic.left.is_none()) {
        if (op_size(ic.left) == 1) {
            load_a(ic.left);
            emit_line("ld\tl, a");
        } else if (op_size(ic.left) == 8) {
            load_hl_word(ic.left, 0);
            emit_line("push\thl");
            load_hl_word(ic.left, 1);
            emit_line("ex\tde, hl");
            emit_line("pop\thl");
        } else if (op_size(ic.left) == 4) {
            load_hl_lo32(ic.left);
            emit_line("push\thl");
            load_hl_hi32(ic.left);
            emit_line("ex\tde, hl");
            emit_line("pop\thl");
        } else {
            load_hl(ic.left);
        }
    }
    emit_line("jp\t%s", fn_end_lbl_.c_str());
}

void z80_gen::gen_send(const icode &ic) {
    int sz = op_size(ic.left);
    if (sz == 1) {
        load_a(ic.left);
        emit_line("ld\tl, a");
        emit_line("ld\th, %s", asm_.imm(0).c_str());
        emit_line("push\thl");
    } else if (sz == 8) {
        for (int w = 3; w >= 0; --w) {
            load_hl_word(ic.left, w);
            emit_line("push\thl");
        }
    } else if (sz == 4) {
        load_hl_hi32(ic.left); emit_line("push\thl");
        load_hl_lo32(ic.left); emit_line("push\thl");
    } else {
        load_hl(ic.left);
        emit_line("push\thl");
    }
}

void z80_gen::gen_receive(const icode &ic) {
    emit_comment("receive param %s at %s",
                 ic.result.name.c_str(), addr_of(ic.result).c_str());
}

void z80_gen::gen_call(const icode &ic) {
    if (!ic.func_name.empty()) {
        std::string callee = mangle(ic.func_name);
        asm_.global_decl(callee);
        emit_line("call\t%s", callee.c_str());
    } else {
        asm_.global_decl("__call_hl");
        load_hl(ic.left);
        emit_line("call\t__call_hl");
    }

    if (ic.num_params > 0) {
        int bytes = ic.arg_bytes > 0 ? ic.arg_bytes : ic.num_params * 2;
        for (int n = 0; n < bytes / 2; ++n)
            emit_line("pop\tbc");
    }

    if (!ic.result.is_none()) {
        int sz = op_size(ic.result);
        if (sz == 1) {
            emit_line("ld\ta, l");
            store_a(ic.result);
        } else if (sz == 4) {
            store_hl_lo32(ic.result);
            emit_line("push\tde");
            emit_line("pop\thl");
            store_hl_hi32(ic.result);
        } else {
            store_hl(ic.result);
        }
    }
}

} // namespace xcc
