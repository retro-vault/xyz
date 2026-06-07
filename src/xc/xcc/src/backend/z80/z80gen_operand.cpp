//
// z80gen_operand.cpp — Operand addressing and register load/store helpers.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"

namespace xcc {

bool z80_gen::fits_ix_disp(int off) {
    return off >= -128 && off <= 127;
}

void z80_gen::load_ix_addr_hl(int off) {
    emit_line("push\tix");
    emit_line("pop\thl");
    if (off == 0)
    {
        set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                       pair_ix_addr_cache_key(off));
        return;
    }

    if (off >= -4 && off <= 4) {
        const char *op = off > 0 ? "inc\thl" : "dec\thl";
        for (int i = 0; i < (off > 0 ? off : -off); ++i)
            emit_line("%s", op);
        set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                       pair_ix_addr_cache_key(off));
        return;
    }

    emit_line("ld\tbc, %s", asm_.imm(off).c_str());
    emit_line("add\thl, bc");
    set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                   pair_ix_addr_cache_key(off));
}

void z80_gen::load_frame_byte(char dst, int off) {
    if (fits_ix_disp(off)) {
        emit_line("ld\t%c, %s", dst, asm_.ix_rel(off).c_str());
        return;
    }

    if (dst != 'a')
        emit_line("push\taf");
    emit_line("push\tbc");
    load_ix_addr_hl(off);
    emit_line("ld\ta, (hl)");
    emit_line("pop\tbc");
    if (dst != 'a')
        emit_line("ld\t%c, a", dst);
    if (dst != 'a')
        emit_line("pop\taf");
}

void z80_gen::store_frame_byte(int off, char src) {
    invalidate_pair_cache();
    if (fits_ix_disp(off)) {
        emit_line("ld\t%s, %c", asm_.ix_rel(off).c_str(), src);
        return;
    }

    emit_line("push\taf");
    emit_line("push\tbc");
    if (src != 'a')
        emit_line("ld\ta, %c", src);
    load_ix_addr_hl(off);
    emit_line("ld\t(hl), a");
    emit_line("pop\tbc");
    emit_line("pop\taf");
}

void z80_gen::load_frame_word(const reg_pair &r, int off) {
    if (fits_ix_disp(off) && fits_ix_disp(off + 1)) {
        emit_line("ld\t%c, %s", r.lo, asm_.ix_rel(off).c_str());
        emit_line("ld\t%c, %s", r.hi, asm_.ix_rel(off + 1).c_str());
        return;
    }

    emit_line("push\tbc");
    load_ix_addr_hl(off);
    emit_line("ld\tc, (hl)");
    emit_line("inc\thl");
    emit_line("ld\tb, (hl)");
    emit_line("ld\t%c, c", r.lo);
    emit_line("ld\t%c, b", r.hi);
    emit_line("pop\tbc");
}

void z80_gen::store_frame_word(const reg_pair &r, int off) {
    invalidate_pair_cache();
    if (fits_ix_disp(off) && fits_ix_disp(off + 1)) {
        emit_line("ld\t%s, %c", asm_.ix_rel(off).c_str(),     r.lo);
        emit_line("ld\t%s, %c", asm_.ix_rel(off + 1).c_str(), r.hi);
        return;
    }

    if (r.lo == 'l') {
        emit_line("push\tde");
        emit_line("ld\td, h");
        emit_line("ld\te, l");
    }
    load_ix_addr_hl(off);
    if (r.lo == 'l') {
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_line("pop\tde");
        return;
    }
    emit_line("ld\t(hl), %c", r.lo);
    emit_line("inc\thl");
    emit_line("ld\t(hl), %c", r.hi);
}

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

    auto home_it = temp_regs_.find(temp_id);
    if (home_it != temp_regs_.end() &&
        !temp_home_uses_spill_slot(home_it->second)) {
        return 0;
    }

    next_temp_slot_ -= sz;
    int offset = next_temp_slot_ - local_bytes_;
    temp_slots_[temp_id] = offset;

    if (temp_frame_bytes_ == 0) {
        if (sz == 1)      emit_line("dec\tsp");
        else if (sz == 2) { emit_line("dec\tsp"); emit_line("dec\tsp"); }
        else if (sz == 4) {
            for (int i = 0; i < 4; ++i) emit_line("dec\tsp");
        } else if (sz == 8) {
            for (int i = 0; i < 8; ++i) emit_line("dec\tsp");
        }
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
        int off = ix_offset_of(op);
        return std::to_string(off) + "(ix)";
    }
    return "?";
}

int z80_gen::ix_offset_of(const operand &op) const {
    int base;
    if (op.kind == operand_kind::TEMP) {
        int sz = (op.type && op.type->size() > 0) ? op.type->size() : 2;
        if (sz < 1) sz = 1;
        base = const_cast<z80_gen*>(this)->alloc_temp(op.temp_id, sz);
    } else {
        base = op.is_param ? param_ix_offset(op) : op.stack_offset;
    }
    return base + op.byte_offset;
}

void z80_gen::emit_load_rr(const reg_pair &r, const operand &op) {
    const std::string cache_key = pair_load_cache_key(op);
    if (pair_cache_matches(r, cache_key))
        return;

    auto extend_loaded_byte = [&](char lo, char hi) {
        if (op.type && !op.type->is_unsigned()) {
            emit_line("ld\ta, %c", lo);
            emit_line("rlca");
            emit_line("sbc\ta, a");
            emit_line("ld\t%c, a", hi);
        } else {
            emit_line("ld\t%c, %s", hi, asm_.imm(0).c_str());
        }
    };

    if (op_size(op) == 1) {
        switch (op.kind) {
        case operand_kind::INT_CONST:
            emit_line("ld\t%c, %s", r.lo, asm_.imm(op.ival & 0xFF).c_str());
            extend_loaded_byte(r.lo, r.hi);
            return;
        case operand_kind::SYMBOL:
            if (op.is_global && op.is_tls) {
                int off = tls_offsets_.count(mangle(op.name)) ? tls_offsets_.at(mangle(op.name)) : 0;
                emit_line("call\t__tls_base");
                emit_line("ld\tbc, %s", asm_.imm(off).c_str());
                emit_line("add\thl, bc");
                emit_line("ld\t%c, (hl)", r.lo);
                extend_loaded_byte(r.lo, r.hi);
                return;
            } else if (op.is_global) {
                emit_line("ld\t%c, (%s)", r.lo, mangle(op.name).c_str());
                extend_loaded_byte(r.lo, r.hi);
                return;
            }
            if (!op.is_global) {
                auto si = incoming_symbol_homes_.find(op.stack_offset);
                if (si != incoming_symbol_homes_.end()) {
                    switch (si->second) {
                    case temp_home::arg_a:
                        emit_line("ld\t%c, a", r.lo);
                        maybe_materialize_incoming_arg_symbol(op);
                        extend_loaded_byte(r.lo, r.hi);
                        return;
                    case temp_home::arg_l:
                        emit_line("ld\t%c, l", r.lo);
                        maybe_materialize_incoming_arg_symbol(op);
                        extend_loaded_byte(r.lo, r.hi);
                        return;
                    case temp_home::arg_hl:
                        emit_line("ld\t%c, %c", r.lo,
                                  op.byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_symbol(op);
                        extend_loaded_byte(r.lo, r.hi);
                        return;
                    case temp_home::arg_de:
                        emit_line("ld\t%c, %c", r.lo,
                                  op.byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_symbol(op);
                        extend_loaded_byte(r.lo, r.hi);
                        return;
                    default:
                        break;
                    }
                }
            }
            load_frame_byte(r.lo, ix_offset_of(op));
            extend_loaded_byte(r.lo, r.hi);
            return;
        case operand_kind::TEMP: {
            auto ri = temp_regs_.find(op.temp_id);
            if (ri != temp_regs_.end()) {
                switch (ri->second) {
                case temp_home::main_a:
                    emit_line("ld\t%c, a", r.lo);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::main_c:
                    emit_line("ld\t%c, c", r.lo);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::alt_a:
                    emit_line("ex\taf, af'");
                    emit_line("ld\t%c, a", r.lo);
                    emit_line("ex\taf, af'");
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::arg_a:
                    emit_line("ld\t%c, a", r.lo);
                    maybe_materialize_incoming_arg_temp(op);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::arg_l:
                    emit_line("ld\t%c, l", r.lo);
                    maybe_materialize_incoming_arg_temp(op);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                default:
                    break;
                }
            }
            load_frame_byte(r.lo, ix_offset_of(op));
            extend_loaded_byte(r.lo, r.hi);
            return;
        }
        default:
            break;
        }
    }

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
        } else if (op.is_global && op.is_func) {
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
            if (symbol_home_in_bc(op) && op.byte_offset == 0) {
                if (r.lo == 'l') {
                    emit_line("ld\th, b");
                    emit_line("ld\tl, c");
                } else {
                    emit_line("ld\td, b");
                    emit_line("ld\te, c");
                }
                break;
            }
            auto si = incoming_symbol_homes_.find(op.stack_offset);
            if (si != incoming_symbol_homes_.end() && op.byte_offset == 0) {
                switch (si->second) {
                case temp_home::arg_hl:
                    if (r.lo != 'l') {
                        emit_line("ld\td, h");
                        emit_line("ld\te, l");
                    }
                    maybe_materialize_incoming_arg_symbol(op);
                    break;
                case temp_home::arg_de:
                    if (r.lo == 'l') {
                        emit_line("ld\th, d");
                        emit_line("ld\tl, e");
                    }
                    maybe_materialize_incoming_arg_symbol(op);
                    break;
                default:
                    load_frame_word(r, ix_offset_of(op));
                    break;
                }
                break;
            }
            load_frame_word(r, ix_offset_of(op));
        }
        break;
    case operand_kind::TEMP: {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end()) {
            switch (ri->second) {
            case temp_home::remat_hl:
                if (emit_rematerialize_hl(op)) {
                    if (r.lo != 'l') {
                        emit_line("ld\td, h");
                        emit_line("ld\te, l");
                    }
                    break;
                }
                [[fallthrough]];
            case temp_home::main_hl:
                if (r.lo != 'l') {
                    emit_line("ld\td, h");
                    emit_line("ld\te, l");
                }
                break;
            case temp_home::main_bc:
                if (r.lo == 'l') { emit_line("ld\th, b"); emit_line("ld\tl, c"); }
                else             { emit_line("ld\td, b"); emit_line("ld\te, c"); }
                break;
            case temp_home::arg_hl:
                if (r.lo != 'l') {
                    emit_line("ld\td, h");
                    emit_line("ld\te, l");
                }
                maybe_materialize_incoming_arg_temp(op);
                break;
            case temp_home::arg_de:
                if (r.lo == 'l') {
                    emit_line("ld\th, d");
                    emit_line("ld\tl, e");
                }
                maybe_materialize_incoming_arg_temp(op);
                break;
            default:
                load_frame_word(r, ix_offset_of(op));
                break;
            }
            break;
        }
        load_frame_word(r, ix_offset_of(op));
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

    set_pair_cache(r, cache_key);
}

void z80_gen::emit_store_rr(const reg_pair &r, const operand &op) {
    bool preserves_pair = true;
    invalidate_pair_cache();
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
            preserves_pair = false;
        } else if (op.is_global) {
            emit_line("ld\t(%s), %s", mangle(op.name).c_str(), r.name);
        } else {
            if (symbol_home_in_bc(op) && op.byte_offset == 0) {
                incoming_symbol_homes_.erase(op.stack_offset);
                if (r.lo == 'l') {
                    emit_line("ld\tb, h");
                    emit_line("ld\tc, l");
                } else {
                    emit_line("ld\tb, d");
                    emit_line("ld\tc, e");
                }
                break;
            }
            incoming_symbol_homes_.erase(op.stack_offset);
            int off = ix_offset_of(op);
            preserves_pair =
                r.lo != 'l' || (fits_ix_disp(off) && fits_ix_disp(off + 1));
            store_frame_word(r, ix_offset_of(op));
        }
        break;
    case operand_kind::TEMP: {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end()) {
            switch (ri->second) {
            case temp_home::remat_hl:
                break;
            case temp_home::main_hl:
                if (r.lo == 'l') {
                    break;
                }
                emit_line("ld\th, d");
                emit_line("ld\tl, e");
                break;
            case temp_home::main_bc:
                if (r.lo == 'l') { emit_line("ld\tb, h"); emit_line("ld\tc, l"); }
                else             { emit_line("ld\tb, d"); emit_line("ld\tc, e"); }
                break;
            default:
                ri->second = temp_home::stack;
                {
                    int off = ix_offset_of(op);
                    preserves_pair =
                        r.lo != 'l' || (fits_ix_disp(off) && fits_ix_disp(off + 1));
                }
                store_frame_word(r, ix_offset_of(op));
                break;
            }
            break;
        }
        {
            int off = ix_offset_of(op);
            preserves_pair =
                r.lo != 'l' || (fits_ix_disp(off) && fits_ix_disp(off + 1));
        }
        store_frame_word(r, ix_offset_of(op));
        break;
    }
    default:
        emit_comment("store_rr: unhandled operand_kind %d", (int)op.kind);
        preserves_pair = false;
        break;
    }

    if (preserves_pair)
        set_pair_cache(r, pair_load_cache_key(op));
    else if (r.lo == 'l')
        invalidate_hl_cache();
    else
        invalidate_de_cache();
}

void z80_gen::load_hl (const operand &op) { constexpr reg_pair HL{"hl",'l','h',false}; emit_load_rr (HL, op); }
void z80_gen::load_de (const operand &op) { constexpr reg_pair DE{"de",'e','d',true};  emit_load_rr (DE, op); }
void z80_gen::store_hl(const operand &op) { constexpr reg_pair HL{"hl",'l','h',false}; emit_store_rr(HL, op); }
void z80_gen::store_de(const operand &op) { constexpr reg_pair DE{"de",'e','d',true};  emit_store_rr(DE, op); }

void z80_gen::load_bc(const operand &op) {
    if (symbol_home_in_bc(op))
        return;
    if (op.kind == operand_kind::TEMP) {
        auto it = temp_regs_.find(op.temp_id);
        if (it != temp_regs_.end() && it->second == temp_home::main_bc)
            return;
    }
    // Load op into BC via HL (no direct BC load from memory on Z80).
    load_hl(op);
    emit_line("ld\tb, h");
    emit_line("ld\tc, l");
}

void z80_gen::load_a(const operand &op) {
    const std::string cache_key = a_load_cache_key(op);
    if (a_cache_matches(cache_key))
        return;

    // [[sdcc::sfr(N)]]: read via IN instruction
    if (op.is_sfr && op.sfr_port >= 0) {
        emit_line("in\ta, (%s)", asm_.imm(op.sfr_port).c_str());
        invalidate_a_cache();
        return;
    }
    if (op.kind == operand_kind::INT_CONST) {
        emit_line("ld\ta, %s", asm_.imm(op.ival & 0xFF).c_str());
        set_a_cache(cache_key);
        return;
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global && op.is_tls) {
        int off = tls_offsets_.count(mangle(op.name)) ? tls_offsets_.at(mangle(op.name)) : 0;
        emit_line("call\t__tls_base");
        emit_line("ld\tbc, %s", asm_.imm(off).c_str());
        emit_line("add\thl, bc");
        emit_line("ld\ta, (hl)");
        invalidate_a_cache();
        return;
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global) {
        emit_line("ld\ta, (%s)", mangle(op.name).c_str());
        invalidate_a_cache();
        return;
    }
    if (op.kind == operand_kind::SYMBOL && !op.is_global) {
        if (symbol_home_in_bc(op)) {
            emit_line("ld\ta, %c", op.byte_offset == 0 ? 'c' : 'b');
            set_a_cache(cache_key);
            return;
        }
        auto si = incoming_symbol_homes_.find(op.stack_offset);
        if (si != incoming_symbol_homes_.end()) {
            switch (si->second) {
            case temp_home::arg_a:
                maybe_materialize_incoming_arg_symbol(op);
                set_a_cache(cache_key);
                return;
            case temp_home::arg_l:
                emit_line("ld\ta, l");
                maybe_materialize_incoming_arg_symbol(op);
                set_a_cache(cache_key);
                return;
            case temp_home::arg_hl:
                emit_line("ld\ta, %c", op.byte_offset == 0 ? 'l' : 'h');
                maybe_materialize_incoming_arg_symbol(op);
                set_a_cache(cache_key);
                return;
            case temp_home::arg_de:
                emit_line("ld\ta, %c", op.byte_offset == 0 ? 'e' : 'd');
                maybe_materialize_incoming_arg_symbol(op);
                set_a_cache(cache_key);
                return;
            default:
                break;
            }
        }
    }
    if (op.kind == operand_kind::TEMP) {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end()) {
            switch (ri->second) {
            case temp_home::main_a:
                set_a_cache(cache_key);
                return;
            case temp_home::main_c:
                emit_line("ld\ta, c");
                set_a_cache(cache_key);
                return;
            case temp_home::alt_a:
                emit_line("ex\taf, af'");
                set_a_cache(cache_key);
                return;
            case temp_home::arg_a:
                maybe_materialize_incoming_arg_temp(op);
                set_a_cache(cache_key);
                return;
            case temp_home::arg_l:
                emit_line("ld\ta, l");
                maybe_materialize_incoming_arg_temp(op);
                set_a_cache(cache_key);
                return;
            default:
                break;
            }
        }
    }
    load_frame_byte('a', ix_offset_of(op));
    set_a_cache(cache_key);
}

void z80_gen::store_a(const operand &op) {
    invalidate_pair_cache();
    const std::string cache_key = a_load_cache_key(op);
    // [[sdcc::sfr(N)]]: write via OUT instruction
    if (op.is_sfr && op.sfr_port >= 0) {
        emit_line("out\t(%s), a", asm_.imm(op.sfr_port).c_str());
        invalidate_a_cache();
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
        invalidate_a_cache();
        return;
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global) {
        emit_line("ld\t(%s), a", mangle(op.name).c_str());
        invalidate_a_cache();
        return;
    }
    if (op.kind == operand_kind::SYMBOL && !op.is_global) {
        if (symbol_home_in_bc(op)) {
            emit_line("ld\t%c, a", op.byte_offset == 0 ? 'c' : 'b');
            set_a_cache(cache_key);
            return;
        }
        incoming_symbol_homes_.erase(op.stack_offset);
    }
    if (op.kind == operand_kind::TEMP) {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end()) {
            switch (ri->second) {
            case temp_home::main_a:
                set_a_cache(cache_key);
                return;
            case temp_home::main_c:
                emit_line("ld\tc, a");
                set_a_cache(cache_key);
                return;
            case temp_home::alt_a:
                emit_line("ex\taf, af'");
                invalidate_a_cache();
                return;
            default:
                ri->second = temp_home::stack;
                break;
            }
        }
    }
    store_frame_byte(ix_offset_of(op), 'a');
    set_a_cache(cache_key);
}

void z80_gen::load_hl_word(const operand &op, int word_index) {
    const std::string cache_key = pair_word_cache_key(op, word_index);
    if (pair_cache_matches(reg_pair{"hl", 'l', 'h', false}, cache_key))
        return;

    int word_byte = word_index * 2;
    if (op.kind == operand_kind::TEMP) {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end() && word_index == 0) {
            if (ri->second == temp_home::remat_hl) {
                if (emit_rematerialize_hl(op)) {
                    set_pair_cache(reg_pair{"hl", 'l', 'h', false}, cache_key);
                    return;
                }
            }
            if (ri->second == temp_home::main_hl) {
                set_pair_cache(reg_pair{"hl", 'l', 'h', false}, cache_key);
                return;
            }
            if (ri->second == temp_home::main_bc) {
                emit_line("ld\th, b");
                emit_line("ld\tl, c");
                set_pair_cache(reg_pair{"hl", 'l', 'h', false}, cache_key);
                return;
            }
        }
    }
    if (op.kind == operand_kind::INT_CONST)
        emit_line("ld\thl, %s", asm_.imm((op.ival >> (word_index * 16)) & 0xFFFF).c_str());
    else if (op.kind == operand_kind::SYMBOL && op.is_global) {
        int total = op.byte_offset + word_byte;
        emit_line("ld\thl, %s", asm_.indir_global(mangle(op.name), total).c_str());
    } else if (op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op) &&
               op.byte_offset + word_byte == 0) {
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
    } else {
        load_frame_word(reg_pair{"hl", 'l', 'h', false},
                        ix_offset_of(op) + word_byte);
    }

    set_pair_cache(reg_pair{"hl", 'l', 'h', false}, cache_key);
}

void z80_gen::load_de_word(const operand &op, int word_index) {
    const std::string cache_key = pair_word_cache_key(op, word_index);
    if (pair_cache_matches(reg_pair{"de", 'e', 'd', true}, cache_key))
        return;

    int word_byte = word_index * 2;
    if (op.kind == operand_kind::TEMP) {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end() && word_index == 0) {
            if (ri->second == temp_home::remat_hl) {
                if (emit_rematerialize_hl(op)) {
                    emit_line("ld\td, h");
                    emit_line("ld\te, l");
                    set_pair_cache(reg_pair{"de", 'e', 'd', true}, cache_key);
                    return;
                }
            }
            if (ri->second == temp_home::main_hl) {
                emit_line("ld\td, h");
                emit_line("ld\te, l");
                set_pair_cache(reg_pair{"de", 'e', 'd', true}, cache_key);
                return;
            }
            if (ri->second == temp_home::main_bc) {
                emit_line("ld\td, b");
                emit_line("ld\te, c");
                set_pair_cache(reg_pair{"de", 'e', 'd', true}, cache_key);
                return;
            }
        }
    }
    if (op.kind == operand_kind::INT_CONST) {
        emit_line("ld\tde, %s", asm_.imm((op.ival >> (word_index * 16)) & 0xFFFF).c_str());
    } else if (op.kind == operand_kind::SYMBOL && op.is_global) {
        int total = op.byte_offset + word_byte;
        emit_line("ld\thl, %s", asm_.indir_global(mangle(op.name), total).c_str());
        emit_line("ex\tde, hl");
    } else if (op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op) &&
               op.byte_offset + word_byte == 0) {
        emit_line("ld\td, b");
        emit_line("ld\te, c");
    } else {
        load_frame_word(reg_pair{"de", 'e', 'd', true},
                        ix_offset_of(op) + word_byte);
    }

    set_pair_cache(reg_pair{"de", 'e', 'd', true}, cache_key);
}

void z80_gen::store_hl_word(const operand &op, int word_index) {
    int word_byte = word_index * 2;
    bool preserves_hl = true;
    if (op.kind == operand_kind::TEMP) {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end() && word_index == 0) {
            if (ri->second == temp_home::remat_hl) {
                set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                               pair_word_cache_key(op, word_index));
                return;
            }
            if (ri->second == temp_home::main_hl && word_index == 0) {
                set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                               pair_word_cache_key(op, word_index));
                return;
            }
            if (ri->second == temp_home::main_bc) {
                emit_line("ld\tb, h");
                emit_line("ld\tc, l");
                set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                               pair_word_cache_key(op, word_index));
                return;
            }
        }
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global) {
        int total = op.byte_offset + word_byte;
        emit_line("ld\t%s, hl", asm_.indir_global(mangle(op.name), total).c_str());
    } else if (op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op) &&
               op.byte_offset + word_byte == 0) {
        emit_line("ld\tb, h");
        emit_line("ld\tc, l");
    } else {
        int off = ix_offset_of(op) + word_byte;
        preserves_hl = fits_ix_disp(off) && fits_ix_disp(off + 1);
        store_frame_word(reg_pair{"hl", 'l', 'h', false},
                         ix_offset_of(op) + word_byte);
    }

    if (preserves_hl)
        set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                       pair_word_cache_key(op, word_index));
    else
        invalidate_hl_cache();
}

void z80_gen::load_hl_lo32 (const operand &op) { load_hl_word(op, 0); }
void z80_gen::load_hl_hi32 (const operand &op) { load_hl_word(op, 1); }
void z80_gen::store_hl_lo32(const operand &op) { store_hl_word(op, 0); }
void z80_gen::store_hl_hi32(const operand &op) { store_hl_word(op, 1); }

} // namespace xcc
