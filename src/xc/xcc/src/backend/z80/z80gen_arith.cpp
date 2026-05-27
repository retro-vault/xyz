//
// z80gen_arith.cpp — arithmetic, bitwise, compare, cast, and float
//                    instruction generation for the Z80 backend.
//
// All methods are members of z80_gen and use the shared helpers
// (emit_line, load_hl, store_hl, etc.) defined in z80gen.cpp.
//
// Runtime helpers called here (defined in lib/runtime/):
//   __mul16, __mul32, __mulll
//   __div16, __div32, __divll
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

void z80_gen::gen_add(const icode &ic) {
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
        emit_line("push\thl");
        load_hl_lo32(ic.right);
        emit_line("pop\tde");
        emit_line("add\thl, de");
        store_hl_lo32(ic.result);
        load_hl_hi32(ic.left);
        emit_line("push\thl");
        load_hl_hi32(ic.right);
        emit_line("pop\tde");
        emit_line("adc\thl, de");
        store_hl_hi32(ic.result);
    } else {
        // inc/dec for ±1 saves 1 byte and 4 cycles vs add hl,de.
        if (ic.right.kind == operand_kind::INT_CONST) {
            int64_t v = ic.right.ival;
            if (v == 1 || v == -1) {
                load_hl(ic.left);
                emit_line(v == 1 ? "inc\thl" : "dec\thl");
                store_hl(ic.result);
                return;
            }
        }
        load_hl(ic.left);
        load_de(ic.right);
        emit_line("add\thl, de");
        store_hl(ic.result);
    }
}

void z80_gen::gen_sub(const icode &ic) {
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
        emit_line("push\thl");
        load_hl_lo32(ic.right);
        emit_line("pop\tde");
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        store_hl_lo32(ic.result);
        load_hl_hi32(ic.left);
        emit_line("push\thl");
        load_hl_hi32(ic.right);
        emit_line("pop\tde");
        emit_line("ex\tde, hl");
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
        load_de(ic.right);
        // Z80 has no SUB hl,de; use SBC after clearing carry.
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        store_hl(ic.result);
    }
}

void z80_gen::gen_mul(const icode &ic) {
    if (is_llong_op(ic.left)) {
        asm_.global_decl("__mulll");
        for (int w = 3; w >= 0; --w) { load_hl_word(ic.right, w); emit_line("push\thl"); }
        for (int w = 3; w >= 0; --w) { load_hl_word(ic.left,  w); emit_line("push\thl"); }
        emit_line("call\t__mulll");
        for (int n = 0; n < 8; ++n) emit_line("pop\tbc");
        return;
    }
    if (op_size(ic.left) == 4) {
        asm_.global_decl("__mul32");
        load_hl_hi32(ic.right); emit_line("push\thl");
        load_hl_lo32(ic.right); emit_line("push\thl");
        load_hl_hi32(ic.left);  emit_line("push\thl");
        load_hl_lo32(ic.left);  emit_line("push\thl");
        emit_line("call\t__mul32");
        for (int n = 0; n < 4; ++n) emit_line("pop\tbc");
        store_hl_lo32(ic.result);
        emit_line("ex\tde, hl");
        emit_line("push\tde");
        emit_line("pop\thl");
        store_hl_hi32(ic.result);
    } else {
        asm_.global_decl("__mul16");
        load_hl(ic.right);
        emit_line("push\thl");
        load_hl(ic.left);
        emit_line("push\thl");
        emit_line("call\t__mul16");
        emit_line("pop\tbc");
        emit_line("pop\tbc");
        store_hl(ic.result);
    }
}

void z80_gen::gen_div_mod(const icode &ic, bool want_mod) {
    if (is_llong_op(ic.left)) {
        const char *helper = want_mod ? "__modll" : "__divll";
        asm_.global_decl(helper);
        for (int w = 3; w >= 0; --w) { load_hl_word(ic.right, w); emit_line("push\thl"); }
        for (int w = 3; w >= 0; --w) { load_hl_word(ic.left,  w); emit_line("push\thl"); }
        emit_line("call\t%s", helper);
        for (int n = 0; n < 8; ++n) emit_line("pop\tbc");
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
        load_hl_hi32(ic.left);  emit_line("push\thl");
        load_hl_lo32(ic.left);  emit_line("push\thl");
        emit_line("call\t%s", helper);
        for (int n = 0; n < 4; ++n) emit_line("pop\tbc");
        store_hl_lo32(ic.result);
        emit_line("push\tde"); emit_line("pop\thl");
        store_hl_hi32(ic.result);
    } else {
        // Load operands into SDCC register ABI: HL=dividend, DE=divisor.
        // Use push/pop to avoid clobbering HL when loading the second operand.
        load_hl(ic.right);
        emit_line("push\thl");
        load_hl(ic.left);
        emit_line("pop\tde");
        if (is_signed) {
            if (want_mod) {
                asm_.global_decl("__smod16");
                emit_line("call\t__smod16");     // HL = remainder
            } else {
                asm_.global_decl("__divsint");
                emit_line("call\t__divsint");    // DE = quotient
                emit_line("ex\tde, hl");         // HL = quotient
            }
        } else {
            asm_.global_decl("__divuint");
            emit_line("call\t__divuint");        // HL = remainder, DE = quotient
            if (!want_mod) emit_line("ex\tde, hl"); // HL = quotient for div
        }
        store_hl(ic.result);
    }
}

void z80_gen::gen_neg(const icode &ic) {
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
    load_hl(ic.left);
    emit_line("push\thl");
    load_hl(ic.right);
    emit_line("pop\tde");
    emit_line("ld\ta, l"); emit_line("and\ta, e"); emit_line("ld\tl, a");
    emit_line("ld\ta, h"); emit_line("and\ta, d"); emit_line("ld\th, a");
    store_hl(ic.result);
}

void z80_gen::gen_bor(const icode &ic) {
    load_hl(ic.left);
    emit_line("push\thl");
    load_hl(ic.right);
    emit_line("pop\tde");
    emit_line("ld\ta, l"); emit_line("or\ta, e");  emit_line("ld\tl, a");
    emit_line("ld\ta, h"); emit_line("or\ta, d");  emit_line("ld\th, a");
    store_hl(ic.result);
}

void z80_gen::gen_bxor(const icode &ic) {
    load_hl(ic.left);
    emit_line("push\thl");
    load_hl(ic.right);
    emit_line("pop\tde");
    emit_line("ld\ta, l"); emit_line("xor\ta, e"); emit_line("ld\tl, a");
    emit_line("ld\ta, h"); emit_line("xor\ta, d"); emit_line("ld\th, a");
    store_hl(ic.result);
}

void z80_gen::gen_bnot(const icode &ic) {
    load_hl(ic.left);
    emit_line("ld\ta, l"); emit_line("cpl"); emit_line("ld\tl, a");
    emit_line("ld\ta, h"); emit_line("cpl"); emit_line("ld\th, a");
    store_hl(ic.result);
}

void z80_gen::gen_shift(const icode &ic, bool right, bool arithmetic) {
    load_hl(ic.left);

    if (ic.right.kind == operand_kind::INT_CONST) {
        int count = (int)(ic.right.ival & 0xFF);
        if (count == 0) { store_hl(ic.result); return; }

        // Shift by 8: byte-swap trick.
        if (count == 8) {
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

        // Shift by 1 or 2: fully inline.
        if (count <= 2) {
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
    }

    // Variable or large constant shift: B-register loop.
    emit_line("push\thl");
    load_hl(ic.right);
    emit_line("ld\tb, l");
    emit_line("pop\thl");

    std::string shift_lbl = "__shift_" + std::to_string(rand() % 10000);
    std::string done_lbl  = "__sdone_" + std::to_string(rand() % 10000);

    asm_.label(shift_lbl, false);
    emit_line("ld\ta, b");
    emit_line("or\ta, a");
    emit_line("jp\tz, %s", done_lbl.c_str());

    if (!right)
        emit_line("add\thl, hl");
    else if (arithmetic) {
        emit_line("sra\th"); emit_line("rr\tl");
    } else {
        emit_line("srl\th"); emit_line("rr\tl");
    }
    emit_line("djnz\t%s", shift_lbl.c_str());
    asm_.label(done_lbl, false);
    store_hl(ic.result);
}

void z80_gen::gen_compare(const icode &ic, icode_op cmp) {
    load_hl(ic.left);
    emit_line("push\thl");
    load_hl(ic.right);
    emit_line("pop\tde");  // DE = left, HL = right

    std::string true_lbl = "__cmp_t_" + std::to_string(rand() % 100000);
    std::string end_lbl  = "__cmp_e_" + std::to_string(rand() % 100000);

    switch (cmp) {
    case icode_op::EQ:
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("jp\tz, %s", true_lbl.c_str());
        break;
    case icode_op::NE:
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("jp\tnz, %s", true_lbl.c_str());
        break;
    case icode_op::LT:
        emit_line("ex\tde, hl");  // HL=left, DE=right
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("jp\tm, %s", true_lbl.c_str());
        break;
    case icode_op::LE:
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("jp\tz, %s", true_lbl.c_str());
        emit_line("jp\tm, %s", true_lbl.c_str());
        break;
    case icode_op::GT:
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("jp\tz, %s", end_lbl.c_str());
        emit_line("jp\tp, %s", true_lbl.c_str());
        break;
    case icode_op::GE:
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("jp\tp, %s", true_lbl.c_str());
        break;
    default:
        break;
    }

    emit_line("ld\thl, %s", asm_.imm(0).c_str());
    emit_line("jp\t%s", end_lbl.c_str());
    asm_.label(true_lbl, false);
    emit_line("ld\thl, %s", asm_.imm(1).c_str());
    asm_.label(end_lbl, false);
    store_hl(ic.result);
}

void z80_gen::gen_cast(const icode &ic) {
    if (!ic.result.type) {
        load_hl(ic.left);
        store_hl(ic.result);
        return;
    }

    int src_sz = op_size(ic.left);
    int dst_sz = op_size(ic.result);

    if (dst_sz == src_sz) {
        load_hl(ic.left);
        store_hl(ic.result);
    } else if (dst_sz > src_sz) {
        if (src_sz == 1) {
            load_a(ic.left);
            if (ic.left.type && ic.left.type->is_unsigned()) {
                emit_line("ld\tl, a");
                emit_line("ld\th, %s", asm_.imm(0).c_str());
            } else {
                // Sign-extend A into HL.
                emit_line("ld\tl, a");
                emit_line("rlca");
                emit_line("sbc\ta, a");  // A = 0x00 or 0xFF
                emit_line("ld\th, a");
            }
            store_hl(ic.result);
        } else {
            load_hl(ic.left);
            store_hl(ic.result);
        }
    } else {
        // Narrowing.
        load_hl(ic.left);
        if (dst_sz == 1) {
            emit_line("ld\ta, l");
            store_a(ic.result);
        } else {
            store_hl(ic.result);
        }
    }
}

// ----- Soft-float helpers in lib/runtime/ ----------------------------
//
// IEEE 754 single (4 bytes). Each operand pushed as 32-bit value
// (lo word first); result returned in DE:HL (DE=hi, HL=lo).
//
void z80_gen::gen_float_arith(const icode &ic) {
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
        load_hl_hi32(ic.left);  emit_line("push\thl");
        load_hl_lo32(ic.left);  emit_line("push\thl");
        emit_line("call\t%s", helper);
        for (int n = 0; n < 4; ++n) emit_line("pop\tbc");
    } else {
        asm_.global_decl(helper);
        load_hl_hi32(ic.left); emit_line("push\thl");
        load_hl_lo32(ic.left); emit_line("push\thl");
        emit_line("call\t%s", helper);
        emit_line("pop\tbc"); emit_line("pop\tbc");
    }
    store_hl_lo32(ic.result);
    emit_line("push\tde"); emit_line("pop\thl");
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
    asm_.raw(ic.asm_text + "\n");
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
