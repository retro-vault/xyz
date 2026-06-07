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
        return it != temp_regs.end() && it->second == temp_home::alt_a;
    }
    return false;
}

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
            off = ix_offset_of(ic.left);
        load_ix_addr_hl(off);
    }
    store_hl(ic.result);
}

void z80_gen::gen_get_value_at(const icode &ic) {
    if (ic.left.kind == operand_kind::LABEL_REF &&
        !ic.right.is_none() &&
        op_size(ic.result) == 1) {
        operand byte_src;
        if (get_zero_extended_u8_source(ic.right, byte_src)) {
            load_a(byte_src);
            emit_line("ld\te, a");
            emit_line("ld\td, %s", asm_.imm(0).c_str());
            emit_line("ld\thl, %s", asm_.imm_sym(ic.left.name).c_str());
            emit_line("add\thl, de");
            emit_line("ld\ta, (hl)");
            store_a(ic.result);
            return;
        }
        load_de(ic.right);
        emit_line("ld\thl, %s", asm_.imm_sym(ic.left.name).c_str());
        emit_line("add\thl, de");
        emit_line("ld\ta, (hl)");
        store_a(ic.result);
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
        if (is_direct_data_symbol(def->left)) {
            base = &def->left;
            index = &def->right;
        } else if (is_direct_data_symbol(def->right)) {
            base = &def->right;
            index = &def->left;
        } else {
            return false;
        }

        operand byte_src;
        if (!get_zero_extended_u8_source(*index, byte_src))
            return false;

        load_a(byte_src);
        emit_line("ld\te, a");
        emit_line("ld\td, %s", asm_.imm(0).c_str());
        emit_line("ld\thl, %s", asm_.imm_sym(mangle(base->name)).c_str());
        emit_line("add\thl, de");
        return true;
    };

    if (is_direct_abs_ptr(ic.left)) {
        const std::string abs = "(" + asm_.imm(ic.left.ival) + ")";
        if (op_size(ic.result) == 1) {
            emit_line("ld\ta, %s", abs.c_str());
            store_a(ic.result);
        } else {
            emit_line("ld\thl, %s", abs.c_str());
            store_hl(ic.result);
        }
        return;
    }

    if (!emit_global_plus_u8_index_hl(ic.left))
        load_hl(ic.left);
    if (op_size(ic.result) == 1) {
        emit_line("ld\ta, (hl)");
        store_a(ic.result);
    } else {
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        store_de(ic.result);
    }
}

void z80_gen::gen_set_value_at(const icode &ic) {
    if (ic.result.kind == operand_kind::LABEL_REF &&
        !ic.right.is_none() &&
        op_size(ic.left) == 1) {
        operand byte_src;
        if (get_zero_extended_u8_source(ic.right, byte_src)) {
            load_a(byte_src);
            emit_line("ld\te, a");
            emit_line("ld\td, %s", asm_.imm(0).c_str());
            emit_line("ld\thl, %s", asm_.imm_sym(ic.result.name).c_str());
            emit_line("add\thl, de");
            if (!load_byte_preserves_hl(ic.left, temp_regs_))
                emit_line("push\thl");
            load_a(ic.left);
            if (!load_byte_preserves_hl(ic.left, temp_regs_))
                emit_line("pop\thl");
            emit_line("ld\t(hl), a");
            return;
        }
        load_de(ic.right);
        emit_line("ld\thl, %s", asm_.imm_sym(ic.result.name).c_str());
        emit_line("add\thl, de");
        if (!load_byte_preserves_hl(ic.left, temp_regs_))
            emit_line("push\thl");
        load_a(ic.left);
        if (!load_byte_preserves_hl(ic.left, temp_regs_))
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
        if (is_direct_data_symbol(def->left)) {
            base = &def->left;
            index = &def->right;
        } else if (is_direct_data_symbol(def->right)) {
            base = &def->right;
            index = &def->left;
        } else {
            return false;
        }

        operand byte_src;
        if (!get_zero_extended_u8_source(*index, byte_src))
            return false;

        load_a(byte_src);
        emit_line("ld\te, a");
        emit_line("ld\td, %s", asm_.imm(0).c_str());
        emit_line("ld\thl, %s", asm_.imm_sym(mangle(base->name)).c_str());
        emit_line("add\thl, de");
        return true;
    };

    if (is_direct_abs_ptr(ic.result)) {
        const std::string abs = "(" + asm_.imm(ic.result.ival) + ")";
        if (op_size(ic.left) == 1) {
            load_a(ic.left);
            emit_line("ld\t%s, a", abs.c_str());
        } else {
            load_hl(ic.left);
            emit_line("ld\t%s, hl", abs.c_str());
        }
        return;
    }

    if (!emit_global_plus_u8_index_hl(ic.result))
        load_hl(ic.result);
    if (op_size(ic.left) == 1) {
        if (!load_byte_preserves_hl(ic.left, temp_regs_))
            emit_line("push\thl");
        load_a(ic.left);
        if (!load_byte_preserves_hl(ic.left, temp_regs_))
            emit_line("pop\thl");
        emit_line("ld\t(hl), a");
    } else {
        if (!load_word_preserves_hl(ic.left))
            emit_line("push\thl");
        load_de(ic.left);
        if (!load_word_preserves_hl(ic.left))
            emit_line("pop\thl");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
    }
}

} // namespace xcc
