//
// z80gen_operand.cpp — Operand addressing and register load/store helpers.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"
#include <cstring>

namespace xcc {

namespace {

uint16_t int_const_word(const operand &op, int word_index) {
    if (word_index < 0)
        return 0;

    const int type_size = op.type ? op.type->size() : 8;
    const int clamped_size =
        type_size < 0 ? 0 : (type_size > 8 ? 8 : type_size);
    const bool sign_fill =
        op.type && !op.type->is_unsigned() && op.ival < 0;
    const int shift = word_index * 16;
    const int bit_width = clamped_size * 8;

    if (shift >= 64 || shift >= bit_width)
        return sign_fill ? 0xffffu : 0x0000u;

    const uint64_t bits = static_cast<uint64_t>(op.ival);
    return static_cast<uint16_t>((bits >> shift) & 0xffffu);
}

uint16_t fp_const_word(const operand &op, int word_index) {
    const int size = op.type ? op.type->size() : 8;
    if (size == 8) {
        if (word_index < 0 || word_index >= 4)
            return 0;
        const uint64_t bits =
            static_cast<uint64_t>(encode_float_constant(op.fval, op.type));
        return static_cast<uint16_t>((bits >> (word_index * 16)) & 0xffffu);
    }

    if (word_index < 0 || word_index >= 2)
        return 0;
    const uint32_t bits =
        static_cast<uint32_t>(encode_float_constant(op.fval, op.type));
    return static_cast<uint16_t>((bits >> (word_index * 16)) & 0xffffu);
}

bool is_16bit_sfr_port(const operand &op) {
    return op.is_sfr && op.sfr_port > 0xff;
}

bool same_stack_storage_base(const operand &a, const operand &b) {
    if (a.kind != b.kind)
        return false;
    switch (a.kind) {
    case operand_kind::TEMP:
        return a.temp_id == b.temp_id;
    case operand_kind::SYMBOL:
        return !a.is_global && !b.is_global &&
               a.is_param == b.is_param &&
               a.is_func == b.is_func &&
               a.is_tls == b.is_tls &&
               a.is_sfr == b.is_sfr &&
               a.sfr_port == b.sfr_port &&
               a.stack_offset == b.stack_offset &&
               a.name == b.name;
    default:
        return false;
    }
}

bool byte_store_memory_barrier(const icode &ic) {
    switch (ic.op) {
    case icode_op::CALL:
    case icode_op::GET_VALUE_AT:
    case icode_op::SET_VALUE_AT:
    case icode_op::BLOCK_FILL:
    case icode_op::ALLOCA:
    case icode_op::INLINE_ASM:
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
        return true;
    default:
        return false;
    }
}

} // namespace

bool z80_gen::fits_ix_disp(int off) {
    return off >= -128 && off <= 127;
}

void z80_gen::load_ix_addr_hl(int off) {
    if (has_known_sp_ix_delta()) {
        // When the SP<->IX delta is known, materializing an address via SP is
        // usually smaller than cloning IX into HL and then walking/adding the
        // displacement. Keep the tiny +/-1 and exact-zero cases on the IX path.
        if (!fits_ix_disp(off) || off < -1 || off > 1) {
            emit_line("ld\thl, %s", asm_.imm(off - current_sp_ix_delta()).c_str());
            emit_line("add\thl, sp");
            set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                           pair_ix_addr_cache_key(off));
            return;
        }
    }

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

    // Deep IX offsets need HL as an address scratch. Preserve it so callers can
    // safely spill multi-byte register arguments one byte at a time.
    emit_line("push\taf");
    emit_line("push\tbc");
    emit_line("push\thl");
    if (src != 'a')
        emit_line("ld\ta, %c", src);
    load_ix_addr_hl(off);
    emit_line("ld\t(hl), a");
    emit_line("pop\thl");
    emit_line("pop\tbc");
    emit_line("pop\taf");
}

void z80_gen::load_frame_word(const reg_pair &r, int off) {
    if (fits_ix_disp(off) && fits_ix_disp(off + 1)) {
        emit_line("ld\t%c, %s", r.lo, asm_.ix_rel(off).c_str());
        emit_line("ld\t%c, %s", r.hi, asm_.ix_rel(off + 1).c_str());
        return;
    }

    emit_line("push\taf");
    emit_line("push\tbc");
    load_ix_addr_hl(off);
    emit_line("ld\tc, (hl)");
    emit_line("inc\thl");
    emit_line("ld\tb, (hl)");
    emit_line("ld\t%c, c", r.lo);
    emit_line("ld\t%c, b", r.hi);
    emit_line("pop\tbc");
    emit_line("pop\taf");
}

void z80_gen::store_frame_word(const reg_pair &r, int off) {
    invalidate_pair_cache();
    if (fits_ix_disp(off) && fits_ix_disp(off + 1)) {
        emit_line("ld\t%s, %c", asm_.ix_rel(off).c_str(),     r.lo);
        emit_line("ld\t%s, %c", asm_.ix_rel(off + 1).c_str(), r.hi);
        return;
    }

    emit_line("push\taf");
    emit_line("push\tbc");
    if (r.lo == 'l') {
        emit_line("push\tde");
        emit_line("ld\td, h");
        emit_line("ld\te, l");
    } else {
        // Deep frame stores use HL as an address scratch, so preserve the
        // caller's high-word source when we're spilling a DE-backed word.
        emit_line("push\thl");
    }
    load_ix_addr_hl(off);
    if (r.lo == 'l') {
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_line("pop\tde");
        emit_line("pop\tbc");
        emit_line("pop\taf");
        return;
    }
    emit_line("ld\t(hl), %c", r.lo);
    emit_line("inc\thl");
    emit_line("ld\t(hl), %c", r.hi);
    emit_line("pop\thl");
    emit_line("pop\tbc");
    emit_line("pop\taf");
}

int z80_gen::op_size(const operand &op) const {
    if (op.kind == operand_kind::SYMBOL &&
        !op.is_global &&
        cur_fn_ &&
        op.type && op.type->size() > 1) {
        auto same_symbol_slot = [&](const operand &cand) {
            return cand.kind == operand_kind::SYMBOL &&
                   cand.is_global == op.is_global &&
                   cand.is_param == op.is_param &&
                   cand.is_tls == op.is_tls &&
                   cand.is_sfr == op.is_sfr &&
                   cand.is_func == op.is_func &&
                   cand.stack_offset == op.stack_offset &&
                   cand.byte_offset == op.byte_offset &&
                   cand.name == op.name;
        };

        int stored_size = 0;
        auto consider_def = [&](const operand &cand) {
            if (!same_symbol_slot(cand) || !cand.type)
                return;
            int sz = cand.type->size();
            if (sz <= 0)
                return;
            if (stored_size == 0 || sz < stored_size)
                stored_size = sz;
        };

        for (const auto &ic : cur_fn_->icodes)
            consider_def(ic.result);

        if (stored_size > 0 && stored_size < op.type->size())
            return stored_size;
    }

    if (!op.type) return 2;
    return op.type->size();
}

bool z80_gen::op_is_16bit(const operand &op) const {
    return op_size(op) >= 2;
}

namespace {

int temp_storage_bytes(const operand &op) {
    int sz = 2;
    if (op.type && op.type->size() > 0)
        sz = op.type->size();
    if (sz < 1)
        sz = 1;
    return sz + std::max(op.byte_offset, 0);
}

} // namespace

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

    // RECEIVE lowering runs before the prologue allocates its frame. If a
    // fused consumer made an incoming argument look spill-free during frame
    // planning, reserve the late slot in that same frame rather than moving
    // SP early. Early movement made local-address calculations overlap the
    // newly allocated slot.
    if (reserving_prologue_spills_) {
        temp_stack_bytes_ += sz;
        temp_frame_bytes_ += sz;
        return offset;
    }

    // Some temps are intentionally left in incoming argument registers for an
    // initial use and only spill later if another use appears. Those late
    // materializations need real stack space even when the precomputed temp
    // frame did not reserve a slot for them, so grow SP whenever we mint a
    // brand-new spill slot.
    if (sz == 1) {
        emit_line("dec\tsp");
    } else if (sz == 2) {
        emit_line("dec\tsp");
        emit_line("dec\tsp");
    } else if (sz == 4) {
        for (int i = 0; i < 4; ++i) emit_line("dec\tsp");
    } else if (sz == 8) {
        for (int i = 0; i < 8; ++i) emit_line("dec\tsp");
    } else {
        for (int i = 0; i < sz; ++i) emit_line("dec\tsp");
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
        if (op.is_global) {
            std::string name = mangle(op.name);
            if (op.byte_offset != 0)
                name += " + " + std::to_string(op.byte_offset);
            return name;
        }
        const int off = (op.is_param ? param_ix_offset(op) : op.stack_offset) +
                        op.byte_offset;
        return std::to_string(off) + "(ix)";
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
        int sz = temp_storage_bytes(op);
        base = const_cast<z80_gen*>(this)->alloc_temp(op.temp_id, sz);
    } else {
        base = op.is_param ? param_ix_offset(op) : op.stack_offset;
    }
    return base + op.byte_offset;
}

bool z80_gen::can_elide_current_byte_store(const operand &op) const {
    if (opt_settings_.level != opt_level::Os &&
        opt_settings_.level != opt_level::Of &&
        opt_settings_.level != opt_level::O3) {
        return false;
    }
    if (!cur_fn_ || cur_ic_index_ + 1 >= cur_fn_->icodes.size())
        return false;
    if (cur_fn_->icodes[cur_ic_index_].op == icode_op::RECEIVE)
        return false;
    if (op_size(op) != 1)
        return false;
    if (op.kind == operand_kind::SYMBOL) {
        if (op.is_global || op.is_tls || op.is_sfr || op.is_func)
            return false;
        if (op.type && op.type->is_volatile)
            return false;
    } else if (op.kind != operand_kind::TEMP) {
        return false;
    }

    for (const auto &ic : cur_fn_->icodes) {
        if (ic.op != icode_op::RECEIVE)
            continue;
        if (same_stack_storage_base(ic.result, op))
            return false;
    }

    std::unordered_set<size_t> active;
    return byte_slot_overwritten_before_read(cur_ic_index_ + 1, op, 160, active);
}

bool z80_gen::byte_slot_overwritten_before_read(
        size_t start,
        const operand &slot,
        size_t budget,
        std::unordered_set<size_t> &active) const {
    if (!cur_fn_ || start >= cur_fn_->icodes.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    auto label_index = [&](const std::string &label) -> size_t {
        if (label.empty())
            return cur_fn_->icodes.size();
        for (size_t i = 0; i < cur_fn_->icodes.size(); ++i) {
            const icode &ic = cur_fn_->icodes[i];
            if (ic.op == icode_op::LABEL && ic.label_name == label)
                return i;
        }
        return cur_fn_->icodes.size();
    };

    auto overlaps_slot = [&](const operand &op) {
        if (!same_stack_storage_base(op, slot))
            return false;
        const int op_width = std::max(1, op_size(op));
        const int op_begin = op.byte_offset;
        const int op_end = op_begin + op_width;
        const int slot_begin = slot.byte_offset;
        const int slot_end = slot_begin + 1;
        return op_begin < slot_end && slot_begin < op_end;
    };

    auto reads_slot = [&](const icode &ic) {
        switch (ic.op) {
        case icode_op::LABEL:
        case icode_op::FUNCTION:
        case icode_op::ENDFUNCTION:
        case icode_op::RECEIVE:
        case icode_op::GOTO:
        case icode_op::CALL:
        case icode_op::INLINE_ASM:
            return false;
        default:
            return overlaps_slot(ic.left) || overlaps_slot(ic.right);
        }
    };

    auto writes_slot = [&](const icode &ic) {
        switch (ic.op) {
        case icode_op::LABEL:
        case icode_op::GOTO:
        case icode_op::IFX:
        case icode_op::FUNCTION:
        case icode_op::ENDFUNCTION:
        case icode_op::RETURN:
        case icode_op::SEND:
        case icode_op::ADDRESS_OF:
        case icode_op::SET_VALUE_AT:
        case icode_op::ALLOCA:
        case icode_op::INLINE_ASM:
            return false;
        default:
            return overlaps_slot(ic.result);
        }
    };

    for (size_t k = start; k < cur_fn_->icodes.size() && budget > 0;
         ++k, --budget) {
        if (skipped_icodes_.find(k) != skipped_icodes_.end())
            return finish(false);

        const icode &ic = cur_fn_->icodes[k];
        if (ic.op == icode_op::LABEL)
            continue;

        if (ic.op == icode_op::GOTO) {
            const size_t target = label_index(ic.label_name);
            if (target == cur_fn_->icodes.size())
                return finish(false);
            return finish(byte_slot_overwritten_before_read(target, slot,
                                                            budget - 1, active));
        }

        if (ic.op == icode_op::IFX) {
            if (reads_slot(ic))
                return finish(false);

            auto branch_ok = [&](const std::string &label) {
                const size_t next = label.empty() ? (k + 1) : label_index(label);
                if (next >= cur_fn_->icodes.size())
                    return false;
                return byte_slot_overwritten_before_read(next, slot,
                                                         budget - 1, active);
            };

            return finish(branch_ok(ic.true_lbl) && branch_ok(ic.false_lbl));
        }

        if (reads_slot(ic))
            return finish(false);
        if (byte_store_memory_barrier(ic))
            return finish(false);
        if (writes_slot(ic))
            return finish(true);
        if (ic.op == icode_op::RETURN)
            return finish(false);
    }

    return finish(false);
}

void z80_gen::emit_load_rr(const reg_pair &r, const operand &op) {
    const std::string cache_key = pair_load_cache_key(op);
    if (pair_cache_matches(r, cache_key))
        return;

    auto effective_byte_type = [&](const operand &byte_op) -> type_ptr {
        if (byte_op.kind == operand_kind::SYMBOL &&
            !byte_op.is_global &&
            cur_fn_ &&
            byte_op.type && byte_op.type->size() >= 1) {
            auto same_symbol_slot = [&](const operand &cand) {
                return cand.kind == operand_kind::SYMBOL &&
                       cand.is_global == byte_op.is_global &&
                       cand.is_param == byte_op.is_param &&
                       cand.is_tls == byte_op.is_tls &&
                       cand.is_sfr == byte_op.is_sfr &&
                       cand.is_func == byte_op.is_func &&
                       cand.stack_offset == byte_op.stack_offset &&
                       cand.byte_offset == byte_op.byte_offset &&
                       cand.name == byte_op.name;
            };

            type_ptr stored_type;
            for (const auto &ic : cur_fn_->icodes) {
                if (!same_symbol_slot(ic.result) || !ic.result.type)
                    continue;
                if (ic.result.type->size() != 1)
                    continue;
                stored_type = ic.result.type;
                break;
            }
            if (stored_type)
                return stored_type;
        }
        return byte_op.type;
    };

    auto extend_loaded_byte = [&](char lo, char hi) {
        type_ptr byte_type = effective_byte_type(op);
        if (byte_type && !byte_type->is_unsigned()) {
            emit_line("ld\ta, %c", lo);
            emit_line("rlca");
            emit_line("sbc\ta, a");
            emit_line("ld\t%c, a", hi);
        } else {
            emit_line("ld\t%c, %s", hi, asm_.imm(0).c_str());
        }
    };

    auto load_promoted_byte_pair = [&](const operand &byte_src, bool sign_extend) {
        if (byte_src.kind == operand_kind::INT_CONST) {
            const uint8_t raw = static_cast<uint8_t>(byte_src.ival & 0xff);
            const uint16_t widened =
                sign_extend
                    ? static_cast<uint16_t>(static_cast<int16_t>(
                          static_cast<int8_t>(raw)))
                    : static_cast<uint16_t>(raw);
            emit_line("ld\t%s, %s", r.name, asm_.imm(widened).c_str());
            set_pair_cache(r, cache_key);
            return;
        }
        load_a(byte_src);
        emit_line("ld\t%c, a", r.lo);
        if (sign_extend) {
            emit_line("rlca");
            emit_line("sbc\ta, a");
            emit_line("ld\t%c, a", r.hi);
        } else {
            emit_line("ld\t%c, %s", r.hi, asm_.imm(0).c_str());
        }
        set_pair_cache(r, cache_key);
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
                if (r.lo == 'a') {
                    emit_line("ld\ta, (%s)", mangle(op.name).c_str());
                } else {
                    emit_line("ld\ta, (%s)", mangle(op.name).c_str());
                    emit_line("ld\t%c, a", r.lo);
                }
                extend_loaded_byte(r.lo, r.hi);
                return;
            }
            if (!op.is_global) {
                auto sri = symbol_regs_.find(symbol_reg_key(op));
                if (sri != symbol_regs_.end()) {
                    switch (sri->second) {
                    case temp_home::main_b:
                        emit_line("ld\t%c, b", r.lo);
                        extend_loaded_byte(r.lo, r.hi);
                        return;
                    case temp_home::main_c:
                        emit_line("ld\t%c, c", r.lo);
                        extend_loaded_byte(r.lo, r.hi);
                        return;
                    default:
                        break;
                    }
                }
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
                case temp_home::main_b:
                    emit_line("ld\t%c, b", r.lo);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::main_c:
                    emit_line("ld\t%c, c", r.lo);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::main_d:
                    emit_line("ld\t%c, d", r.lo);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::main_e:
                    emit_line("ld\t%c, e", r.lo);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::main_bc:
                    emit_line("ld\t%c, %c", r.lo,
                              op.byte_offset == 0 ? 'c' : 'b');
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::main_de:
                    emit_line("ld\t%c, %c", r.lo,
                              op.byte_offset == 0 ? 'e' : 'd');
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::alt_a:
                    emit_line("ex\taf, af'");
                    emit_line("ld\t%c, a", r.lo);
                    emit_line("ex\taf, af'");
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::remat_hl:
                    if (emit_rematerialize_hl(op)) {
                        emit_line("ld\t%c, %c", r.lo,
                                  op.byte_offset == 0 ? 'l' : 'h');
                        extend_loaded_byte(r.lo, r.hi);
                        return;
                    }
                    [[fallthrough]];
                case temp_home::main_hl:
                    emit_line("ld\t%c, %c", r.lo,
                              op.byte_offset == 0 ? 'l' : 'h');
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
                case temp_home::arg_hl:
                    emit_line("ld\t%c, %c", r.lo,
                              op.byte_offset == 0 ? 'l' : 'h');
                    maybe_materialize_incoming_arg_temp(op);
                    extend_loaded_byte(r.lo, r.hi);
                    return;
                case temp_home::arg_de:
                    emit_line("ld\t%c, %c", r.lo,
                              op.byte_offset == 0 ? 'e' : 'd');
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

    operand byte_src;
    if (get_zero_extended_u8_source(op, byte_src)) {
        load_promoted_byte_pair(byte_src, false);
        return;
    }
    if (get_sign_extended_i8_source(op, byte_src)) {
        load_promoted_byte_pair(byte_src, true);
        return;
    }

    switch (op.kind) {
    case operand_kind::INT_CONST:
        emit_line("ld\t%s, %s", r.name, asm_.imm(op.ival).c_str());
        break;
    case operand_kind::FLOAT_CONST:
        emit_line("ld\t%s, %s", r.name,
                  asm_.imm(encode_float_constant(op.fval, op.type) & 0xFFFF).c_str());
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
            const std::string global_addr =
                asm_.indir_global(mangle(op.name), op.byte_offset);
            if (r.via_hl) {
                emit_line("ld\thl, %s", global_addr.c_str());
                emit_line("ex\tde, hl");
            } else {
                emit_line("ld\t%s, %s", r.name, global_addr.c_str());
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
            if (symbol_home_in_iy(op) && op.byte_offset == 0) {
                if ((opt_settings_.level == opt_level::Of ||
                     opt_settings_.level == opt_level::O3) && r.lo != 'l') {
                    emit_line("ld\td, iyh");
                    emit_line("ld\te, iyl");
                } else {
                    emit_line("push\tiy");
                    emit_line(r.lo == 'l' ? "pop\thl" : "pop\tde");
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
            case temp_home::main_de:
                if (r.lo == 'l') { emit_line("ld\th, d"); emit_line("ld\tl, e"); }
                break;
            case temp_home::main_iy:
                if ((opt_settings_.level == opt_level::Of ||
                     opt_settings_.level == opt_level::O3) && r.lo != 'l') {
                    emit_line("ld\td, iyh");
                    emit_line("ld\te, iyl");
                } else {
                    emit_line("push\tiy");
                    emit_line(r.lo == 'l' ? "pop\thl" : "pop\tde");
                }
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
        emit_line("ld\t%s, %s", r.name,
                  asm_.imm_sym(asm_label_ref_name(op.name)).c_str());
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
            emit_line("ld\t%s, %s",
                      asm_.indir_global(mangle(op.name), op.byte_offset).c_str(),
                      r.name);
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
            case temp_home::main_de:
                if (r.lo == 'l') { emit_line("ld\td, h"); emit_line("ld\te, l"); }
                break;
            case temp_home::main_iy:
                if ((opt_settings_.level == opt_level::Of ||
                     opt_settings_.level == opt_level::O3) && r.lo != 'l') {
                    emit_line("ld\tiyh, d");
                    emit_line("ld\tiyl, e");
                } else {
                    emit_line(r.lo == 'l' ? "push\thl" : "push\tde");
                    emit_line("pop\tiy");
                }
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

void z80_gen::load_hl (const operand &op) {
    if (direct_word_value_pending_ &&
        direct_word_value_.is_temp() &&
        op.is_temp() &&
        op.temp_id == direct_word_value_.temp_id &&
        op_is_16bit(op)) {
        invalidate_pair_cache();
        invalidate_a_cache();
        emit_line("ex\tde, hl");
        direct_word_value_pending_ = false;
        direct_word_value_ = operand{};
        return;
    }
    constexpr reg_pair HL{"hl",'l','h',false};
    emit_load_rr(HL, op);
}
void z80_gen::load_de (const operand &op) {
    if (direct_word_value_pending_ &&
        direct_word_value_.is_temp() &&
        op.is_temp() &&
        op.temp_id == direct_word_value_.temp_id &&
        op_is_16bit(op)) {
        direct_word_value_pending_ = false;
        direct_word_value_ = operand{};
        return;
    }
    constexpr reg_pair DE{"de",'e','d',true};
    emit_load_rr(DE, op);
}
void z80_gen::store_hl(const operand &op) { constexpr reg_pair HL{"hl",'l','h',false}; emit_store_rr(HL, op); }
void z80_gen::store_de(const operand &op) { constexpr reg_pair DE{"de",'e','d',true};  emit_store_rr(DE, op); }

void z80_gen::load_de_zero_extended_u8(const operand &op) {
    if (op.kind == operand_kind::INT_CONST) {
        load_de(operand::make_int(op.ival & 0xff, type::make_uint()));
        return;
    }

    invalidate_de_cache();
    load_a(op);
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
}

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
        if (is_16bit_sfr_port(op)) {
            emit_line("push\tbc");
            emit_line("ld\tbc, %s", asm_.imm(op.sfr_port).c_str());
            emit_line("in\ta, (c)");
            emit_line("pop\tbc");
        } else {
            emit_line("in\ta, (%s)", asm_.imm(op.sfr_port).c_str());
        }
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
        // Honour a non-zero byte offset (e.g. the bank byte of a far
        // pointer at sym+2, or the tail byte of any odd-sized global).
        if (op.byte_offset != 0)
            emit_line("ld\ta, %s",
                      asm_.indir_global(mangle(op.name), op.byte_offset).c_str());
        else
            emit_line("ld\ta, (%s)", mangle(op.name).c_str());
        invalidate_a_cache();
        return;
    }
    if (op.kind == operand_kind::SYMBOL && !op.is_global) {
        auto sri = symbol_regs_.find(symbol_reg_key(op));
        if (sri != symbol_regs_.end()) {
            switch (sri->second) {
            case temp_home::main_b:
                emit_line("ld\ta, b");
                set_a_cache(cache_key);
                return;
            case temp_home::main_c:
                emit_line("ld\ta, c");
                set_a_cache(cache_key);
                return;
            default:
                break;
            }
        }
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
        auto remat = iy_u32_remat_offsets_.find(op.temp_id);
        if (remat != iy_u32_remat_offsets_.end()) {
            const int64_t disp = remat->second + op.byte_offset;
            emit_line("ld\ta, %lld(iy)", static_cast<long long>(disp));
            set_a_cache(cache_key);
            return;
        }
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end()) {
            switch (ri->second) {
            case temp_home::main_a:
                set_a_cache(cache_key);
                return;
            case temp_home::main_b:
                emit_line("ld\ta, b");
                set_a_cache(cache_key);
                return;
            case temp_home::main_c:
                emit_line("ld\ta, c");
                set_a_cache(cache_key);
                return;
            case temp_home::main_d:
                emit_line("ld\ta, d");
                set_a_cache(cache_key);
                return;
            case temp_home::main_e:
                emit_line("ld\ta, e");
                set_a_cache(cache_key);
                return;
            case temp_home::main_bc:
                emit_line("ld\ta, %c", op.byte_offset == 0 ? 'c' : 'b');
                set_a_cache(cache_key);
                return;
            case temp_home::main_de:
                emit_line("ld\ta, %c", op.byte_offset == 0 ? 'e' : 'd');
                set_a_cache(cache_key);
                return;
            case temp_home::alt_a:
                emit_line("ex\taf, af'");
                set_a_cache(cache_key);
                return;
            case temp_home::remat_hl:
                if (emit_rematerialize_hl(op)) {
                    emit_line("ld\ta, %c", op.byte_offset == 0 ? 'l' : 'h');
                    set_a_cache(cache_key);
                    return;
                }
                [[fallthrough]];
            case temp_home::main_hl:
                emit_line("ld\ta, %c", op.byte_offset == 0 ? 'l' : 'h');
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
            case temp_home::arg_hl:
                emit_line("ld\ta, %c", op.byte_offset == 0 ? 'l' : 'h');
                maybe_materialize_incoming_arg_temp(op);
                set_a_cache(cache_key);
                return;
            case temp_home::arg_de:
                emit_line("ld\ta, %c", op.byte_offset == 0 ? 'e' : 'd');
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
    if (jump_table_selector_loads_.count(cur_ic_index_) && op.is_temp()) {
        set_a_cache(cache_key);
        return;
    }
    // [[sdcc::sfr(N)]]: write via OUT instruction
    if (op.is_sfr && op.sfr_port >= 0) {
        if (is_16bit_sfr_port(op)) {
            emit_line("push\tbc");
            emit_line("ld\tbc, %s", asm_.imm(op.sfr_port).c_str());
            emit_line("out\t(c), a");
            emit_line("pop\tbc");
        } else {
            emit_line("out\t(%s), a", asm_.imm(op.sfr_port).c_str());
        }
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
        if (op.byte_offset != 0)
            emit_line("ld\t%s, a",
                      asm_.indir_global(mangle(op.name), op.byte_offset).c_str());
        else
            emit_line("ld\t(%s), a", mangle(op.name).c_str());
        invalidate_a_cache();
        return;
    }
    if (op.kind == operand_kind::SYMBOL && !op.is_global) {
        auto sri = symbol_regs_.find(symbol_reg_key(op));
        if (sri != symbol_regs_.end()) {
            switch (sri->second) {
            case temp_home::main_b:
                emit_line("ld\tb, a");
                set_a_cache(cache_key);
                return;
            case temp_home::main_c:
                emit_line("ld\tc, a");
                set_a_cache(cache_key);
                return;
            default:
                break;
            }
        }
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
            case temp_home::main_b:
                emit_line("ld\tb, a");
                set_a_cache(cache_key);
                return;
            case temp_home::main_c:
                emit_line("ld\tc, a");
                set_a_cache(cache_key);
                return;
            case temp_home::main_d:
                emit_line("ld\td, a");
                set_a_cache(cache_key);
                return;
            case temp_home::main_e:
                emit_line("ld\te, a");
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
    if (can_elide_current_byte_store(op)) {
        invalidate_a_cache();
        return;
    }
    const int frame_off = ix_offset_of(op);
    store_frame_byte(frame_off, 'a');
    set_a_cache(cache_key);
}

void z80_gen::load_hl_word(const operand &op, int word_index) {
    const std::string cache_key = pair_word_cache_key(op, word_index);
    if (pair_cache_matches(reg_pair{"hl", 'l', 'h', false}, cache_key))
        return;

    int word_byte = word_index * 2;
    if (op.is_temp()) {
        auto remat = iy_u32_remat_offsets_.find(op.temp_id);
        if (remat != iy_u32_remat_offsets_.end()) {
            const int64_t disp = remat->second + word_byte;
            emit_line("ld\tl, %lld(iy)", static_cast<long long>(disp));
            emit_line("ld\th, %lld(iy)", static_cast<long long>(disp + 1));
            set_pair_cache(reg_pair{"hl", 'l', 'h', false}, cache_key);
            return;
        }
    }
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
            if (ri->second == temp_home::main_iy) {
                emit_line("push\tiy");
                emit_line("pop\thl");
                set_pair_cache(reg_pair{"hl", 'l', 'h', false}, cache_key);
                return;
            }
        }
    }
    if (op.kind == operand_kind::INT_CONST)
        emit_line("ld\thl, %s", asm_.imm(int_const_word(op, word_index)).c_str());
    else if (op.kind == operand_kind::FLOAT_CONST)
        emit_line("ld\thl, %s", asm_.imm(fp_const_word(op, word_index)).c_str());
    else if (op.kind == operand_kind::SYMBOL && op.is_global) {
        int total = op.byte_offset + word_byte;
        emit_line("ld\thl, %s", asm_.indir_global(mangle(op.name), total).c_str());
    } else if (op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op) &&
               op.byte_offset + word_byte == 0) {
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
    } else if (op.kind == operand_kind::SYMBOL && symbol_home_in_iy(op) &&
               op.byte_offset + word_byte == 0) {
        emit_line("push\tiy");
        emit_line("pop\thl");
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
    if (op.is_temp()) {
        auto remat = iy_u32_remat_offsets_.find(op.temp_id);
        if (remat != iy_u32_remat_offsets_.end()) {
            const int64_t disp = remat->second + word_byte;
            emit_line("ld\te, %lld(iy)", static_cast<long long>(disp));
            emit_line("ld\td, %lld(iy)", static_cast<long long>(disp + 1));
            set_pair_cache(reg_pair{"de", 'e', 'd', true}, cache_key);
            return;
        }
    }
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
            if (ri->second == temp_home::main_iy) {
                if (opt_settings_.level == opt_level::Of ||
                    opt_settings_.level == opt_level::O3) {
                    emit_line("ld\td, iyh");
                    emit_line("ld\te, iyl");
                } else {
                    emit_line("push\tiy");
                    emit_line("pop\tde");
                }
                set_pair_cache(reg_pair{"de", 'e', 'd', true}, cache_key);
                return;
            }
        }
    }
    if (op.kind == operand_kind::INT_CONST) {
        emit_line("ld\tde, %s", asm_.imm(int_const_word(op, word_index)).c_str());
    } else if (op.kind == operand_kind::FLOAT_CONST) {
        emit_line("ld\tde, %s", asm_.imm(fp_const_word(op, word_index)).c_str());
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
            if (ri->second == temp_home::main_iy) {
                emit_line("push\thl");
                emit_line("pop\tiy");
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

void z80_gen::store_de_word(const operand &op, int word_index) {
    int word_byte = word_index * 2;
    bool preserves_de = true;
    if (op.kind == operand_kind::TEMP) {
        auto ri = temp_regs_.find(op.temp_id);
        if (ri != temp_regs_.end() && word_index == 0 && ri->second == temp_home::main_bc) {
            emit_line("ld\tb, d");
            emit_line("ld\tc, e");
            set_pair_cache(reg_pair{"de", 'e', 'd', true},
                           pair_word_cache_key(op, word_index));
            return;
        }
        if (ri != temp_regs_.end() && word_index == 0 &&
            ri->second == temp_home::main_iy) {
            if (opt_settings_.level == opt_level::Of ||
                opt_settings_.level == opt_level::O3) {
                emit_line("ld\tiyh, d");
                emit_line("ld\tiyl, e");
            } else {
                emit_line("push\tde");
                emit_line("pop\tiy");
            }
            set_pair_cache(reg_pair{"de", 'e', 'd', true},
                           pair_word_cache_key(op, word_index));
            return;
        }
    }
    if (op.kind == operand_kind::SYMBOL && op.is_global) {
        int total = op.byte_offset + word_byte;
        emit_line("ld\t%s, de", asm_.indir_global(mangle(op.name), total).c_str());
    } else if (op.kind == operand_kind::SYMBOL && symbol_home_in_bc(op) &&
               op.byte_offset + word_byte == 0) {
        emit_line("ld\tb, d");
        emit_line("ld\tc, e");
    } else {
        int off = ix_offset_of(op) + word_byte;
        preserves_de = fits_ix_disp(off) && fits_ix_disp(off + 1);
        store_frame_word(reg_pair{"de", 'e', 'd', true},
                         ix_offset_of(op) + word_byte);
    }

    if (preserves_de)
        set_pair_cache(reg_pair{"de", 'e', 'd', true},
                       pair_word_cache_key(op, word_index));
    else
        invalidate_de_cache();
}

void z80_gen::load_hl_lo32 (const operand &op) { load_hl_word(op, 0); }
void z80_gen::load_hl_hi32 (const operand &op) { load_hl_word(op, 1); }
void z80_gen::store_hl_lo32(const operand &op) { store_hl_word(op, 0); }
void z80_gen::store_hl_hi32(const operand &op) { store_hl_word(op, 1); }

void z80_gen::load_reg64(const operand &op) {
    invalidate_pair_cache();
    for (int w = 3; w >= 0; --w) {
        load_hl_word(op, w);
        emit_line("push\thl");
    }
    emit_line("pop\tde");
    emit_line("pop\thl");
    emit_line("exx");
    emit_line("pop\tde");
    emit_line("pop\thl");
    emit_line("exx");
    invalidate_pair_cache();
}

void z80_gen::store_reg64(const operand &op) {
    invalidate_pair_cache();
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("exx");
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("exx");

    emit_line("pop\thl");
    store_hl_word(op, 3);
    emit_line("pop\thl");
    store_hl_word(op, 2);
    emit_line("pop\thl");
    store_hl_word(op, 1);
    emit_line("pop\thl");
    store_hl_word(op, 0);
    invalidate_pair_cache();
}

} // namespace xcc
