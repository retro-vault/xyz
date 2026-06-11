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
    int sz = op_size(ic.left);
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
            load_a(byte_src);
            emit_line("ld\te, a");
            emit_line("ld\td, %s", asm_.imm(0).c_str());
            emit_line("ld\thl, %s", asm_.imm_sym(label_sym).c_str());
            emit_line("add\thl, de");
            emit_line("ld\ta, (hl)");
            store_a(ic.result);
            return;
        }
        load_de(ic.right);
        emit_line("ld\thl, %s", asm_.imm_sym(label_sym).c_str());
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
        } else if (op_size(ic.result) == 8) {
            for (int w = 0; w < 4; ++w) {
                const std::string wadd =
                    "(" + asm_.imm(ic.left.ival + (w * 2)) + ")";
                emit_line("ld\thl, %s", wadd.c_str());
                store_hl_word(ic.result, w);
            }
        } else if (op_size(ic.result) == 4) {
            for (int w = 0; w < 2; ++w) {
                const std::string wadd =
                    "(" + asm_.imm(ic.left.ival + (w * 2)) + ")";
                emit_line("ld\thl, %s", wadd.c_str());
                store_hl_word(ic.result, w);
            }
        } else {
            emit_line("ld\thl, %s", abs.c_str());
            store_hl(ic.result);
        }
        return;
    }

    if (op_size(ic.result) == 1 && is_bc_pointer_home(ic.left)) {
        emit_line("ld\ta, (bc)");
        store_a(ic.result);
        return;
    }

    if (!emit_global_plus_u8_index_hl(ic.left))
        load_hl(ic.left);
    if (op_size(ic.result) == 1) {
        emit_line("ld\ta, (hl)");
        store_a(ic.result);
    } else if (op_size(ic.result) == 8) {
        for (int w = 0; w < 4; ++w) {
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("inc\thl");
            emit_line("push\thl");
            emit_line("ex\tde, hl");
            store_hl_word(ic.result, w);
            emit_line("pop\thl");
        }
    } else if (op_size(ic.result) == 4) {
        for (int w = 0; w < 2; ++w) {
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("inc\thl");
            emit_line("push\thl");
            emit_line("ex\tde, hl");
            store_hl_word(ic.result, w);
            emit_line("pop\thl");
        }
    } else {
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        store_de(ic.result);
    }
}

void z80_gen::gen_set_value_at(const icode &ic) {
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
            load_a(byte_src);
            emit_line("ld\te, a");
            emit_line("ld\td, %s", asm_.imm(0).c_str());
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
        } else if (op_size(ic.left) == 8) {
            for (int w = 0; w < 4; ++w) {
                const std::string wadd =
                    "(" + asm_.imm(ic.result.ival + (w * 2)) + ")";
                load_hl_word(ic.left, w);
                emit_line("ld\t%s, hl", wadd.c_str());
            }
        } else if (op_size(ic.left) == 4) {
            for (int w = 0; w < 2; ++w) {
                const std::string wadd =
                    "(" + asm_.imm(ic.result.ival + (w * 2)) + ")";
                load_hl_word(ic.left, w);
                emit_line("ld\t%s, hl", wadd.c_str());
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
    } else if (op_size(ic.left) == 8) {
        for (int w = 0; w < 4; ++w) {
            emit_line("push\thl");
            load_de_word(ic.left, w);
            emit_line("pop\thl");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            if (w != 3)
                emit_line("inc\thl");
        }
    } else if (op_size(ic.left) == 4) {
        for (int w = 0; w < 2; ++w) {
            emit_line("push\thl");
            load_de_word(ic.left, w);
            emit_line("pop\thl");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            if (w != 1)
                emit_line("inc\thl");
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
