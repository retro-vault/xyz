//
// z80gen_mem.cpp — Memory icode handlers: assign, address_of,
//                  get_value_at, set_value_at.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"

namespace xcc {

void z80_gen::gen_assign(const icode &ic) {
    if (op_size(ic.left) == 1) {
        load_a(ic.left);
        store_a(ic.result);
    } else if (op_size(ic.left) == 8) {
        for (int w = 0; w < 4; ++w) {
            load_hl_word(ic.left, w);
            store_hl_word(ic.result, w);
        }
    } else if (op_size(ic.left) == 4) {
        load_hl_lo32(ic.left);
        store_hl_lo32(ic.result);
        load_hl_hi32(ic.left);
        store_hl_hi32(ic.result);
    } else {
        load_hl(ic.left);
        store_hl(ic.result);
    }
}

void z80_gen::gen_address_of(const icode &ic) {
    if (ic.left.is_global) {
        emit_line("ld\thl, %s", asm_.imm_sym(mangle(ic.left.name)).c_str());
    } else {
        int off = ic.left.is_param ? param_ix_offset(ic.left) : ic.left.stack_offset;
        if (ic.left.kind == operand_kind::TEMP)
            off = temp_ix_offset(ic.left.temp_id);
        emit_line("push\tix");
        emit_line("pop\thl");
        emit_line("ld\tde, %s", asm_.imm(off).c_str());
        emit_line("add\thl, de");
    }
    store_hl(ic.result);
}

void z80_gen::gen_get_value_at(const icode &ic) {
    load_hl(ic.left);
    if (op_size(ic.result) == 1) {
        emit_line("ld\ta, (hl)");
        store_a(ic.result);
    } else {
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("ex\tde, hl");
        store_hl(ic.result);
    }
}

void z80_gen::gen_set_value_at(const icode &ic) {
    load_hl(ic.result);
    emit_line("push\thl");
    if (op_size(ic.left) == 1) {
        load_a(ic.left);
        emit_line("pop\thl");
        emit_line("ld\t(hl), a");
    } else {
        load_de(ic.left);
        emit_line("pop\thl");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
    }
}

} // namespace xcc
