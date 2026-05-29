//
// z80gen_operand.cpp — Operand addressing and register load/store helpers.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"

namespace xcc {

int z80_gen::op_size(const operand &op) const {
    if (!op.type) return 2;
    return op.type->size();
}

bool z80_gen::op_is_16bit(const operand &op) const {
    return op_size(op) >= 2;
}

int z80_gen::alloc_temp(int temp_id, int sz) {
    auto it = temp_slots_.find(temp_id);
    if (it != temp_slots_.end()) return it->second;

    if (temp_regs_.count(temp_id)) {
        temp_slots_[temp_id] = 0;
        return 0;
    }

    next_temp_slot_ -= sz;
    int offset = next_temp_slot_ - local_bytes_;
    temp_slots_[temp_id] = offset;

    if (sz == 1)      emit_line("dec\tsp");
    else if (sz == 2) { emit_line("dec\tsp"); emit_line("dec\tsp"); }
    else if (sz == 4) {
        for (int i = 0; i < 4; ++i) emit_line("dec\tsp");
    } else if (sz == 8) {
        for (int i = 0; i < 8; ++i) emit_line("dec\tsp");
    }

    return offset;
}

int z80_gen::temp_ix_offset(int temp_id) const {
    auto it = temp_slots_.find(temp_id);
    if (it == temp_slots_.end())
        return const_cast<z80_gen*>(this)->alloc_temp(temp_id);
    return it->second;
}

int z80_gen::param_ix_offset(const operand &op) const {
    return op.stack_offset + 4;
}

std::string z80_gen::addr_of(const operand &op) {
    if (op.kind == operand_kind::SYMBOL) {
        if (op.is_global) return mangle(op.name);
        if (op.is_param)  return std::to_string(param_ix_offset(op)) + "(ix)";
        return std::to_string(op.stack_offset) + "(ix)";
    }
    if (op.kind == operand_kind::TEMP) {
        int off = temp_ix_offset(op.temp_id);
        return std::to_string(off) + "(ix)";
    }
    return "?";
}

int z80_gen::ix_offset_of(const operand &op) const {
    int base;
    if (op.kind == operand_kind::TEMP) {
        int sz = (op.type && op.type->size() > 0) ? op.type->size() : 2;
        if (sz < 2) sz = 2;
        base = const_cast<z80_gen*>(this)->alloc_temp(op.temp_id, sz);
    } else {
        base = op.is_param ? param_ix_offset(op) : op.stack_offset;
    }
    return base + op.byte_offset;
}

void z80_gen::emit_load_rr(const reg_pair &r, const operand &op) {
    switch (op.kind) {
    case operand_kind::INT_CONST:
        emit_line("ld\t%s, %s", r.name, asm_.imm(op.ival).c_str());
        break;
    case operand_kind::SYMBOL:
        if (op.is_global && op.is_tls) {
            int off = tls_offsets_.count(mangle(op.name)) ? tls_offsets_.at(mangle(op.name)) : 0;
            emit_line("call\t__tls_base");
            emit_line("ld\tbc, %s", asm_.imm(off).c_str());
            emit_line("add\thl, bc");
            if (r.via_hl) {
                emit_line("ld\te, (hl)");
                emit_line("inc\thl");
                emit_line("ld\td, (hl)");
            } else {
                emit_line("ld\tc, (hl)");
                emit_line("inc\thl");
                emit_line("ld\tb, (hl)");
                emit_line("ld\tl, c");
                emit_line("ld\th, b");
            }
        } else if (op.is_global && op.type && op.type->is_func()) {
            if (r.via_hl) {
                emit_line("ld\thl, %s", asm_.imm_sym(mangle(op.name)).c_str());
                emit_line("ex\tde, hl");
            } else {
                emit_line("ld\t%s, %s", r.name, asm_.imm_sym(mangle(op.name)).c_str());
            }
        } else if (op.is_global) {
            if (r.via_hl) {
                emit_line("ld\thl, (%s)", mangle(op.name).c_str());
                emit_line("ex\tde, hl");
            } else {
                emit_line("ld\t%s, (%s)", r.name, mangle(op.name).c_str());
            }
        } else {
            int off = ix_offset_of(op);
            emit_line("ld\t%c, %s", r.lo, asm_.ix_rel(off).c_str());
            emit_line("ld\t%c, %s", r.hi, asm_.ix_rel(off + 1).c_str());
        }
        break;
    case operand_kind::TEMP: {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end() && ri->second == temp_home::main_bc) {
            if (r.lo == 'l') { emit_line("ld\th, b"); emit_line("ld\tl, c"); }
            else             { emit_line("ld\td, b"); emit_line("ld\te, c"); }
            break;
        }
        int off = ix_offset_of(op);
        emit_line("ld\t%c, %s", r.lo, asm_.ix_rel(off).c_str());
        emit_line("ld\t%c, %s", r.hi, asm_.ix_rel(off + 1).c_str());
        break;
    }
    case operand_kind::LABEL_REF:
        emit_line("ld\t%s, %s", r.name, asm_.imm_sym(op.name).c_str());
        break;
    default:
        emit_comment("load_rr: unhandled operand_kind %d", (int)op.kind);
        emit_line("ld\t%s, %s", r.name, asm_.imm(0).c_str());
        break;
    }
}

void z80_gen::emit_store_rr(const reg_pair &r, const operand &op) {
    switch (op.kind) {
    case operand_kind::SYMBOL:
        if (op.is_global && op.is_tls) {
            int off = tls_offsets_.count(mangle(op.name)) ? tls_offsets_.at(mangle(op.name)) : 0;
            if (!r.via_hl) {
                emit_line("push\thl");
                emit_line("call\t__tls_base");
                emit_line("ld\tbc, %s", asm_.imm(off).c_str());
                emit_line("add\thl, bc");
                emit_line("pop\tde");
                emit_line("ld\t(hl), e");
                emit_line("inc\thl");
                emit_line("ld\t(hl), d");
            } else {
                emit_line("call\t__tls_base");
                emit_line("ld\tbc, %s", asm_.imm(off).c_str());
                emit_line("add\thl, bc");
                emit_line("ld\t(hl), e");
                emit_line("inc\thl");
                emit_line("ld\t(hl), d");
            }
        } else if (op.is_global) {
            emit_line("ld\t(%s), %s", mangle(op.name).c_str(), r.name);
        } else {
            int off = ix_offset_of(op);
            emit_line("ld\t%s, %c", asm_.ix_rel(off).c_str(),     r.lo);
            emit_line("ld\t%s, %c", asm_.ix_rel(off + 1).c_str(), r.hi);
        }
        break;
    case operand_kind::TEMP: {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end() && ri->second == temp_home::main_bc) {
            if (r.lo == 'l') { emit_line("ld\tb, h"); emit_line("ld\tc, l"); }
            else             { emit_line("ld\tb, d"); emit_line("ld\tc, e"); }
            break;
        }
        int off = ix_offset_of(op);
        emit_line("ld\t%s, %c", asm_.ix_rel(off).c_str(),     r.lo);
        emit_line("ld\t%s, %c", asm_.ix_rel(off + 1).c_str(), r.hi);
        break;
    }
    default:
        emit_comment("store_rr: unhandled operand_kind %d", (int)op.kind);
        break;
    }
}

void z80_gen::load_hl (const operand &op) { constexpr reg_pair HL{"hl",'l','h',false}; emit_load_rr (HL, op); }
void z80_gen::load_de (const operand &op) { constexpr reg_pair DE{"de",'e','d',true};  emit_load_rr (DE, op); }
void z80_gen::store_hl(const operand &op) { constexpr reg_pair HL{"hl",'l','h',false}; emit_store_rr(HL, op); }

void z80_gen::load_bc(const operand &op) {
    // Load op into BC via HL (no direct BC load from memory on Z80).
    load_hl(op);
    emit_line("ld\tb, h");
    emit_line("ld\tc, l");
}

void z80_gen::load_a(const operand &op) {
    // [[sdcc::sfr(N)]]: read via IN instruction
    if (op.is_sfr && op.sfr_port >= 0) {
        emit_line("in\ta, (%s)", asm_.imm(op.sfr_port).c_str());
        return;
    }
    if (op.kind == operand_kind::INT_CONST) {
        emit_line("ld\ta, %s", asm_.imm(op.ival & 0xFF).c_str());
        return;
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global && op.is_tls) {
        int off = tls_offsets_.count(mangle(op.name)) ? tls_offsets_.at(mangle(op.name)) : 0;
        emit_line("call\t__tls_base");
        emit_line("ld\tbc, %s", asm_.imm(off).c_str());
        emit_line("add\thl, bc");
        emit_line("ld\ta, (hl)");
        return;
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global) {
        emit_line("ld\ta, (%s)", mangle(op.name).c_str());
        return;
    }
    if (op.kind == operand_kind::TEMP) {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end() && ri->second == temp_home::alt_a) {
            emit_line("ex\taf, af'");
            return;
        }
    }
    emit_line("ld\ta, %s", asm_.ix_rel(ix_offset_of(op)).c_str());
}

void z80_gen::store_a(const operand &op) {
    // [[sdcc::sfr(N)]]: write via OUT instruction
    if (op.is_sfr && op.sfr_port >= 0) {
        emit_line("out\t(%s), a", asm_.imm(op.sfr_port).c_str());
        return;
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global && op.is_tls) {
        int off = tls_offsets_.count(mangle(op.name)) ? tls_offsets_.at(mangle(op.name)) : 0;
        emit_line("push\taf");
        emit_line("call\t__tls_base");
        emit_line("ld\tde, %s", asm_.imm(off).c_str());
        emit_line("add\thl, de");
        emit_line("pop\tde");
        emit_line("ld\t(hl), d");
        return;
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global) {
        emit_line("ld\t(%s), a", mangle(op.name).c_str());
        return;
    }
    if (op.kind == operand_kind::TEMP) {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end() && ri->second == temp_home::alt_a) {
            emit_line("ex\taf, af'");
            return;
        }
    }
    emit_line("ld\t%s, a", asm_.ix_rel(ix_offset_of(op)).c_str());
}

void z80_gen::load_hl_word(const operand &op, int word_index) {
    int word_byte = word_index * 2;
    if (op.kind == operand_kind::INT_CONST)
        emit_line("ld\thl, %s", asm_.imm((op.ival >> (word_index * 16)) & 0xFFFF).c_str());
    else if (op.kind == operand_kind::SYMBOL && op.is_global) {
        int total = op.byte_offset + word_byte;
        emit_line("ld\thl, %s", asm_.indir_global(mangle(op.name), total).c_str());
    } else {
        int off = ix_offset_of(op);
        emit_line("ld\tl, %s", asm_.ix_rel(off + word_byte).c_str());
        emit_line("ld\th, %s", asm_.ix_rel(off + word_byte + 1).c_str());
    }
}

void z80_gen::store_hl_word(const operand &op, int word_index) {
    int word_byte = word_index * 2;
    if (op.kind == operand_kind::SYMBOL && op.is_global) {
        int total = op.byte_offset + word_byte;
        emit_line("ld\t%s, hl", asm_.indir_global(mangle(op.name), total).c_str());
    } else {
        int off = ix_offset_of(op);
        emit_line("ld\t%s, l", asm_.ix_rel(off + word_byte).c_str());
        emit_line("ld\t%s, h", asm_.ix_rel(off + word_byte + 1).c_str());
    }
}

void z80_gen::load_hl_lo32 (const operand &op) { load_hl_word(op, 0); }
void z80_gen::load_hl_hi32 (const operand &op) { load_hl_word(op, 1); }
void z80_gen::store_hl_lo32(const operand &op) { store_hl_word(op, 0); }
void z80_gen::store_hl_hi32(const operand &op) { store_hl_word(op, 1); }

} // namespace xcc
