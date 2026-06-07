//
// z80gen.cpp — Z80 code generator: core utilities and icode dispatch.
//
// Responsibilities kept here:
//   z80cc helpers, constructor, emit_line/label/comment, mangle,
//   emit_function/prologue/epilogue, gen_icode dispatch switch.
//
// Split into:
//   z80gen_data.cpp     — emit_module, emit_globals, emit_strings
//   z80gen_operand.cpp  — op_size/alloc_temp/addr_of/load/store helpers
//   z80gen_ctrl.cpp     — gen_label/goto/ifx/function/return/send/receive/call
//   z80gen_mem.cpp      — gen_assign/address_of/get_value_at/set_value_at
//   z80gen_arith.cpp    — arithmetic and bitwise handlers
//   z80gen_regalloc.cpp — register allocation pre-pass
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"
#include "backend/z80/z80peep.h"
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace xcc {

namespace {

bool is_compare_op(icode_op op) {
    switch (op) {
    case icode_op::EQ:
    case icode_op::NE:
    case icode_op::LT:
    case icode_op::LE:
    case icode_op::GT:
    case icode_op::GE:
        return true;
    default:
        return false;
    }
}

bool operand_uses_temp(const operand &op, int temp_id) {
    return op.kind == operand_kind::TEMP && op.temp_id == temp_id;
}

bool operand_matches_symbol_slot(const operand &a, const operand &b) {
    return a.kind == operand_kind::SYMBOL &&
           b.kind == operand_kind::SYMBOL &&
           !a.is_global &&
           !b.is_global &&
           a.stack_offset == b.stack_offset &&
           a.name == b.name;
}

bool operands_equivalent(const operand &a, const operand &b) {
    if (a.kind != b.kind ||
        a.byte_offset != b.byte_offset ||
        a.is_global != b.is_global ||
        a.is_param != b.is_param ||
        a.is_func != b.is_func ||
        a.is_tls != b.is_tls ||
        a.is_sfr != b.is_sfr ||
        a.sfr_port != b.sfr_port ||
        a.stack_offset != b.stack_offset ||
        a.temp_id != b.temp_id ||
        a.name != b.name) {
        return false;
    }

    switch (a.kind) {
    case operand_kind::NONE:
        return true;
    case operand_kind::TEMP:
        return true;
    case operand_kind::SYMBOL:
        return true;
    case operand_kind::INT_CONST:
        return a.ival == b.ival;
    case operand_kind::FLOAT_CONST:
        return a.fval == b.fval;
    case operand_kind::LABEL_REF:
        return true;
    }
    return false;
}

bool extract_const_compare_case(const icode &cmp_ic,
                                operand &cond,
                                int64_t &case_value) {
    if (cmp_ic.op != icode_op::EQ)
        return false;

    if (cmp_ic.left.kind == operand_kind::INT_CONST &&
        cmp_ic.right.kind != operand_kind::INT_CONST) {
        case_value = cmp_ic.left.ival;
        cond = cmp_ic.right;
        return true;
    }
    if (cmp_ic.right.kind == operand_kind::INT_CONST &&
        cmp_ic.left.kind != operand_kind::INT_CONST) {
        case_value = cmp_ic.right.ival;
        cond = cmp_ic.left;
        return true;
    }
    return false;
}

} // namespace

// ----- z80_cc helpers ------------------------------------------------

const char *z80cc_name(z80_cc cc) {
    switch (cc) {
    case z80_cc::Z:  return "z";  case z80_cc::NZ: return "nz";
    case z80_cc::C:  return "c";  case z80_cc::NC: return "nc";
    case z80_cc::PE: return "pe"; case z80_cc::PO: return "po";
    case z80_cc::M:  return "m";  case z80_cc::P:  return "p";
    default:        return "";
    }
}

z80_cc z80cc_negate(z80_cc cc) {
    switch (cc) {
    case z80_cc::Z:  return z80_cc::NZ; case z80_cc::NZ: return z80_cc::Z;
    case z80_cc::C:  return z80_cc::NC; case z80_cc::NC: return z80_cc::C;
    case z80_cc::PE: return z80_cc::PO; case z80_cc::PO: return z80_cc::PE;
    case z80_cc::M:  return z80_cc::P;  case z80_cc::P:  return z80_cc::M;
    default:        return cc;
    }
}

// ----- z80_gen -------------------------------------------------------

z80_gen::z80_gen(asm_emitter &asm_out) : asm_(asm_out) {}

void z80_gen::set_debug(std::unique_ptr<debug_info_emitter> dbg) {
    debug_ = std::move(dbg);
}

void z80_gen::emit_line(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    track_emitted_instruction(buf);
    asm_.instr(buf);
}

void z80_gen::emit_label(const std::string &name, bool global) {
    invalidate_pair_cache();
    invalidate_a_cache();
    asm_.label(name, global);
}

void z80_gen::emit_comment(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    asm_.comment(buf);
}

void z80_gen::invalidate_pair_cache() {
    if (!pair_cache_enabled())
        return;
    invalidate_hl_cache();
    invalidate_de_cache();
}

void z80_gen::invalidate_hl_cache() {
    if (!pair_cache_enabled())
        return;
    hl_cache_.valid = false;
    hl_cache_.key.clear();
}

void z80_gen::invalidate_de_cache() {
    if (!pair_cache_enabled())
        return;
    de_cache_.valid = false;
    de_cache_.key.clear();
}

void z80_gen::invalidate_a_cache() {
    if (!a_cache_enabled())
        return;
    a_cache_.valid = false;
    a_cache_.key.clear();
}

void z80_gen::set_pair_cache(const reg_pair &r, const std::string &key) {
    if (!pair_cache_enabled())
        return;
    pair_cache_state &cache = (r.lo == 'l') ? hl_cache_ : de_cache_;
    cache.valid = true;
    cache.key = key;
}

bool z80_gen::pair_cache_matches(const reg_pair &r,
                                 const std::string &key) const {
    if (!pair_cache_enabled())
        return false;
    const pair_cache_state &cache = (r.lo == 'l') ? hl_cache_ : de_cache_;
    return cache.valid && cache.key == key;
}

void z80_gen::set_a_cache(const std::string &key) {
    if (!a_cache_enabled())
        return;
    if (key.empty()) {
        invalidate_a_cache();
        return;
    }
    a_cache_.valid = true;
    a_cache_.key = key;
}

bool z80_gen::a_cache_matches(const std::string &key) const {
    if (!a_cache_enabled() || key.empty())
        return false;
    return a_cache_.valid && a_cache_.key == key;
}

std::string z80_gen::pair_load_cache_key(const operand &op) const {
    std::ostringstream oss;
    oss << "pair:";
    switch (op.kind) {
    case operand_kind::INT_CONST:
        oss << "iconst:" << op.ival;
        break;
    case operand_kind::SYMBOL:
        oss << "sym:" << op.name
            << ':' << op.is_global
            << ':' << op.is_param
            << ':' << op.stack_offset;
        break;
    case operand_kind::TEMP:
        oss << "temp:" << op.temp_id;
        break;
    case operand_kind::LABEL_REF:
        oss << "label:" << op.name;
        break;
    default:
        oss << "kind:" << static_cast<int>(op.kind);
        break;
    }
    oss << ":bo=" << op.byte_offset;
    oss << ":sz=" << op_size(op);
    oss << ":u=" << (op.type && op.type->is_unsigned());
    return oss.str();
}

std::string z80_gen::pair_word_cache_key(const operand &op,
                                         int word_index) const {
    std::ostringstream oss;
    oss << pair_load_cache_key(op) << ":word=" << word_index;
    return oss.str();
}

std::string z80_gen::pair_ix_addr_cache_key(int off) const {
    std::ostringstream oss;
    oss << "ixaddr:" << off;
    return oss.str();
}

std::string z80_gen::a_load_cache_key(const operand &op) const {
    if (op.is_sfr)
        return {};

    std::ostringstream oss;
    oss << "byte:";
    switch (op.kind) {
    case operand_kind::INT_CONST:
        oss << "iconst:" << (op.ival & 0xFF);
        break;
    case operand_kind::SYMBOL:
        if (op.is_global)
            return {};
        oss << "sym:" << op.name
            << ':' << op.is_param
            << ':' << op.stack_offset;
        break;
    case operand_kind::TEMP:
        oss << "temp:" << op.temp_id;
        break;
    default:
        return {};
    }
    oss << ":bo=" << op.byte_offset;
    return oss.str();
}

void z80_gen::track_emitted_instruction(const std::string &line) {
    if (!pair_cache_enabled())
        return;

    using sv = std::string_view;

    auto starts = [&](sv prefix) {
        return sv(line).substr(0, prefix.size()) == prefix;
    };
    auto trim = [](sv text) {
        while (!text.empty() && text.front() == ' ')
            text.remove_prefix(1);
        while (!text.empty() && text.back() == ' ')
            text.remove_suffix(1);
        return text;
    };
    auto invalidate_byte_dest = [&](sv mnemonic, bool use_last_operand) {
        std::string prefix = std::string(mnemonic) + "\t";
        if (!starts(prefix))
            return false;
        sv rest = sv(line).substr(prefix.size());
        if (use_last_operand) {
            size_t comma = rest.rfind(',');
            if (comma == sv::npos)
                return false;
            rest = rest.substr(comma + 1);
        }
        rest = trim(rest);
        if (rest == "h" || rest == "l") {
            invalidate_hl_cache();
            return true;
        }
        if (rest == "d" || rest == "e") {
            invalidate_de_cache();
            return true;
        }
        if (rest == "a") {
            invalidate_a_cache();
            return true;
        }
        return false;
    };

    if (starts("call\t")) {
        invalidate_pair_cache();
        invalidate_a_cache();
        return;
    }
    if (starts("ex\tde, hl") || starts("ex\thl, de")) {
        std::swap(hl_cache_, de_cache_);
        return;
    }
    if (starts("ex\taf, af'") || starts("ex\taf,af'")) {
        invalidate_a_cache();
        return;
    }
    if (starts("exx") || starts("ldir") || starts("lddr") ||
        starts("ldi") || starts("ldd")) {
        invalidate_pair_cache();
        return;
    }
    if (starts("pop\thl")) {
        invalidate_hl_cache();
        return;
    }
    if (starts("pop\tde")) {
        invalidate_de_cache();
        return;
    }
    if (starts("pop\taf")) {
        invalidate_a_cache();
        return;
    }
    if (starts("inc\thl") || starts("dec\thl") ||
        starts("add\thl") || starts("adc\thl") ||
        starts("sbc\thl")) {
        invalidate_hl_cache();
        return;
    }
    if (starts("inc\tde") || starts("dec\tde")) {
        invalidate_de_cache();
        return;
    }
    if (starts("ld\thl,")) {
        invalidate_hl_cache();
        return;
    }
    if (starts("ld\tde,")) {
        invalidate_de_cache();
        return;
    }
    if (starts("ld\ta,")) {
        invalidate_a_cache();
        return;
    }
    if (starts("ld\th,") || starts("ld\tl,")) {
        invalidate_hl_cache();
        return;
    }
    if (starts("ld\td,") || starts("ld\te,")) {
        invalidate_de_cache();
        return;
    }
    if (starts("add\ta") || starts("adc\ta") || starts("sub\t") ||
        starts("sbc\ta") || starts("and\t") || starts("xor\t") ||
        starts("or\t") || starts("neg") || starts("cpl") ||
        starts("rla") || starts("rlca") || starts("rra") ||
        starts("rrca") || starts("in\ta,")) {
        invalidate_a_cache();
        return;
    }
    if (invalidate_byte_dest("inc", false) ||
        invalidate_byte_dest("dec", false) ||
        invalidate_byte_dest("rl", false) ||
        invalidate_byte_dest("rr", false) ||
        invalidate_byte_dest("rlc", false) ||
        invalidate_byte_dest("rrc", false) ||
        invalidate_byte_dest("sla", false) ||
        invalidate_byte_dest("sra", false) ||
        invalidate_byte_dest("srl", false) ||
        invalidate_byte_dest("set", true) ||
        invalidate_byte_dest("res", true)) {
        return;
    }
}

std::string z80_gen::mangle(const std::string &name) const {
    if (!name.empty() && name[0] == '_') return name;
    return "_" + name;
}

// ----- Function emission ---------------------------------------------

void z80_gen::emit_function(const ir_function &fn) {
    cur_fn_         = &fn;
    local_bytes_    = fn.local_bytes;
    fn_end_lbl_     = "__" + fn.name + "_end";
    temp_slots_.clear();
    temp_regs_.clear();
    incoming_symbol_homes_.clear();
    symbol_regs_.clear();
    next_temp_slot_ = 0;
    temp_stack_bytes_ = 0;
    temp_frame_bytes_ = 0;
    cur_convention_ = &get_abi_convention(fn.abi);
    cur_ic_index_ = 0;
    invalidate_pair_cache();
    invalidate_a_cache();

    if (regalloc_enabled())
        regalloc_prepass(fn);
    temp_stack_bytes_ = compute_temp_frame_bytes(fn);
    bool auto_temp_frame =
        opt_settings_.level == opt_level::O0 ||
        opt_settings_.level == opt_level::O1 ||
        opt_settings_.level == opt_level::O2 ||
        opt_settings_.level == opt_level::O3 ||
        opt_settings_.level == opt_level::Os;
    if (temp_frame_prealloc_enabled() ||
        (auto_temp_frame && temp_stack_bytes_ > 0 && !can_omit_frame_pointer(fn)))
        temp_frame_bytes_ = temp_stack_bytes_;

    for (size_t i = 0; i < fn.icodes.size(); ++i) {
        if (try_emit_switch_jump_table(fn, i))
            continue;
        if (try_emit_compare_ifx(fn, i))
            continue;
        gen_icode(fn.icodes[i]);
    }

    cur_fn_ = nullptr;
}

void z80_gen::emit_prologue(const ir_function &fn) {
    cur_convention_->emit_prologue(*this, fn);
}

void z80_gen::emit_epilogue(const ir_function &fn) {
    cur_convention_->emit_epilogue(*this, fn);
}

bool z80_gen::temp_value_used_after(const ir_function &fn, size_t start_idx,
                                    int temp_id) const {
    for (size_t i = start_idx; i < fn.icodes.size(); ++i) {
        const auto &ic = fn.icodes[i];
        if (operand_uses_temp(ic.left, temp_id) ||
            operand_uses_temp(ic.right, temp_id)) {
            return true;
        }
        if (operand_uses_temp(ic.result, temp_id))
            return false;
    }
    return false;
}

bool z80_gen::symbol_value_used_after(const ir_function &fn, size_t start_idx,
                                      const operand &sym) const {
    for (size_t i = start_idx; i < fn.icodes.size(); ++i) {
        const auto &ic = fn.icodes[i];
        if (operand_matches_symbol_slot(ic.left, sym) ||
            operand_matches_symbol_slot(ic.right, sym)) {
            return true;
        }
        if (operand_matches_symbol_slot(ic.result, sym))
            return false;
    }
    return false;
}

const icode *z80_gen::find_temp_def_before(int temp_id, size_t before_idx) const {
    if (!cur_fn_)
        return nullptr;
    if (before_idx > cur_fn_->icodes.size())
        before_idx = cur_fn_->icodes.size();
    for (size_t i = before_idx; i > 0; --i) {
        const auto &ic = cur_fn_->icodes[i - 1];
        if (ic.result.is_temp() && ic.result.temp_id == temp_id)
            return &ic;
    }
    return nullptr;
}

bool z80_gen::get_zero_extended_u8_source(const operand &op, operand &src) const {
    if (op.kind == operand_kind::INT_CONST) {
        if (op.ival < 0 || op.ival > 0xff)
            return false;
        src = op;
        return true;
    }

    if (op.type && op.type->size() == 1 && op.type->is_unsigned()) {
        src = op;
        return true;
    }

    if (!op.is_temp())
        return false;

    const icode *def = find_temp_def_before(op.temp_id, cur_ic_index_);
    if (!def)
        return false;

    if (def->op == icode_op::CAST &&
        def->result.type && def->result.type->size() >= 2) {
        return get_zero_extended_u8_source(def->left, src);
    }

    if (def->op == icode_op::ASSIGN)
        return get_zero_extended_u8_source(def->left, src);

    return false;
}

bool z80_gen::emit_rematerialize_hl(const operand &op) {
    if (!op.is_temp())
        return false;

    const icode *def = find_temp_def_before(op.temp_id, cur_ic_index_);
    if (!def || !def->result.is_temp() || def->result.temp_id != op.temp_id)
        return false;

    auto adjust_hl = [&](int extra_off) {
        if (extra_off == 0)
            return;
        if (extra_off >= -4 && extra_off <= 4) {
            const char *adj = extra_off > 0 ? "inc\thl" : "dec\thl";
            for (int i = 0; i < (extra_off > 0 ? extra_off : -extra_off); ++i)
                emit_line("%s", adj);
            return;
        }
        emit_line("ld\tbc, %s", asm_.imm(extra_off).c_str());
        emit_line("add\thl, bc");
    };

    std::function<bool(const operand &, int)> remat =
        [&](const operand &cur, int depth) -> bool {
            if (!cur.is_temp() || depth > 4)
                return false;

            const icode *cur_def = find_temp_def_before(cur.temp_id, cur_ic_index_);
            if (!cur_def || !cur_def->result.is_temp() ||
                cur_def->result.temp_id != cur.temp_id)
                return false;

            switch (cur_def->op) {
            case icode_op::ADDRESS_OF: {
                invalidate_pair_cache();
                if (cur_def->left.is_global) {
                    emit_line("ld\thl, %s",
                              asm_.imm_sym(mangle(cur_def->left.name)).c_str());
                } else {
                    int off = cur_def->left.is_param
                                  ? param_ix_offset(cur_def->left)
                                  : cur_def->left.stack_offset;
                    if (cur_def->left.kind == operand_kind::TEMP)
                        off = ix_offset_of(cur_def->left);
                    load_ix_addr_hl(off);
                }
                adjust_hl(cur.byte_offset);
                return true;
            }
            case icode_op::ASSIGN:
            case icode_op::CAST: {
                operand byte_src;
                if (!get_zero_extended_u8_source(cur_def->left, byte_src))
                    return false;
                invalidate_pair_cache();
                load_a(byte_src);
                emit_line("ld\tl, a");
                emit_line("ld\th, %s", asm_.imm(0).c_str());
                adjust_hl(cur.byte_offset);
                return true;
            }
            case icode_op::ADD: {
                const operand *base = nullptr;
                const operand *index = nullptr;
                auto base_ok = [](const operand &cand) {
                    return (cand.kind == operand_kind::SYMBOL &&
                            cand.is_global && !cand.is_tls &&
                            !cand.is_func && !cand.is_param) ||
                           cand.kind == operand_kind::LABEL_REF;
                };
                auto addr_temp_ok = [&](const operand &cand) {
                    if (!cand.is_temp())
                        return false;
                    const icode *addr_def =
                        find_temp_def_before(cand.temp_id, cur_ic_index_);
                    return addr_def && addr_def->op == icode_op::ADDRESS_OF;
                };
                if (base_ok(cur_def->left) || addr_temp_ok(cur_def->left)) {
                    base = &cur_def->left;
                    index = &cur_def->right;
                } else if (base_ok(cur_def->right) || addr_temp_ok(cur_def->right)) {
                    base = &cur_def->right;
                    index = &cur_def->left;
                } else {
                    return false;
                }

                operand byte_src;
                if (!get_zero_extended_u8_source(*index, byte_src))
                    return false;

                invalidate_pair_cache();
                load_a(byte_src);
                emit_line("ld\te, a");
                emit_line("ld\td, %s", asm_.imm(0).c_str());
                if (base->kind == operand_kind::LABEL_REF) {
                    emit_line("ld\thl, %s", asm_.imm_sym(base->name).c_str());
                } else if (base->kind == operand_kind::SYMBOL) {
                    emit_line("ld\thl, %s",
                              asm_.imm_sym(mangle(base->name)).c_str());
                } else if (base->is_temp()) {
                    if (!remat(*base, depth + 1))
                        return false;
                }
                emit_line("add\thl, de");
                adjust_hl(cur.byte_offset);
                return true;
            }
            default:
                return false;
            }
        };

    return remat(op, 0);
}

bool z80_gen::temp_home_uses_spill_slot(temp_home home) {
    switch (home) {
    case temp_home::stack:
    case temp_home::arg_a:
    case temp_home::arg_l:
    case temp_home::arg_hl:
    case temp_home::arg_de:
        return true;
    default:
        return false;
    }
}

int z80_gen::symbol_reg_key(const operand &op) {
    int key = op.stack_offset;
    if (op.is_param)
        key ^= 0x40000000;
    return key;
}

bool z80_gen::symbol_home_in_bc(const operand &op) const {
    if (op.kind != operand_kind::SYMBOL || op.is_global)
        return false;
    auto it = symbol_regs_.find(symbol_reg_key(op));
    return it != symbol_regs_.end() && it->second == temp_home::main_bc;
}

void z80_gen::maybe_materialize_incoming_arg_temp(const operand &op) {
    if (!cur_fn_ || !op.is_temp())
        return;

    auto it = temp_regs_.find(op.temp_id);
    if (it == temp_regs_.end())
        return;

    switch (it->second) {
    case temp_home::arg_a:
    case temp_home::arg_l:
    case temp_home::arg_hl:
    case temp_home::arg_de:
        break;
    default:
        return;
    }

    if (!temp_value_used_after(*cur_fn_, cur_ic_index_ + 1, op.temp_id))
        return;

    emit_comment("materialize incoming arg temp t%d for later reuse", op.temp_id);
    switch (it->second) {
    case temp_home::arg_a:
        store_frame_byte(ix_offset_of(op), 'a');
        break;
    case temp_home::arg_l:
        store_frame_byte(ix_offset_of(op), 'l');
        break;
    case temp_home::arg_hl:
        store_frame_word(reg_pair{"hl", 'l', 'h', false}, ix_offset_of(op));
        break;
    case temp_home::arg_de:
        store_frame_word(reg_pair{"de", 'e', 'd', true}, ix_offset_of(op));
        break;
    default:
        break;
    }
    it->second = temp_home::stack;
}

void z80_gen::maybe_materialize_incoming_arg_symbol(const operand &op) {
    if (!cur_fn_ || op.kind != operand_kind::SYMBOL || op.is_global)
        return;

    auto it = incoming_symbol_homes_.find(op.stack_offset);
    if (it == incoming_symbol_homes_.end())
        return;

    if (!symbol_value_used_after(*cur_fn_, cur_ic_index_ + 1, op))
        return;

    emit_comment("materialize incoming arg symbol %s for later reuse", op.name.c_str());
    switch (it->second) {
    case temp_home::arg_a:
        store_frame_byte(ix_offset_of(op), 'a');
        break;
    case temp_home::arg_l:
        store_frame_byte(ix_offset_of(op), 'l');
        break;
    case temp_home::arg_hl:
        store_frame_word(reg_pair{"hl", 'l', 'h', false}, op.stack_offset);
        break;
    case temp_home::arg_de:
        store_frame_word(reg_pair{"de", 'e', 'd', true}, op.stack_offset);
        break;
    default:
        break;
    }
    incoming_symbol_homes_.erase(it);
}

bool z80_gen::try_emit_compare_ifx(const ir_function &fn, size_t &idx) {
    if (!compare_ifx_fusion_enabled() || idx + 1 >= fn.icodes.size())
        return false;

    const auto &cmp_ic = fn.icodes[idx];
    const auto &ifx_ic = fn.icodes[idx + 1];

    auto invert_compare = [](icode_op op) -> icode_op {
        switch (op) {
        case icode_op::EQ: return icode_op::NE;
        case icode_op::NE: return icode_op::EQ;
        case icode_op::LT: return icode_op::GE;
        case icode_op::LE: return icode_op::GT;
        case icode_op::GT: return icode_op::LE;
        case icode_op::GE: return icode_op::LT;
        default: return op;
        }
    };

    if (!is_compare_op(cmp_ic.op) || ifx_ic.op != icode_op::IFX)
        return false;

    auto emit_fused = [&](const icode &effective_cmp,
                          icode_op branch_cmp,
                          const icode &final_ifx,
                          size_t consume_count) {
        if (debug_)
            debug_->emit_location(final_ifx.line ? final_ifx.line : effective_cmp.line);
        emit_compare_branch(effective_cmp, branch_cmp,
                            final_ifx.true_lbl, final_ifx.false_lbl);
        idx += consume_count;
        return true;
    };

    if (ifx_ic.true_lbl.empty())
        return false;

    if (cmp_ic.result.is_temp() && ifx_ic.left.is_temp() &&
        cmp_ic.result.temp_id == ifx_ic.left.temp_id &&
        !temp_value_used_after(fn, idx + 2, cmp_ic.result.temp_id)) {
        return emit_fused(cmp_ic, cmp_ic.op, ifx_ic, 1);
    }

    if (idx + 2 >= fn.icodes.size())
        return false;

    const auto &bool_ic = fn.icodes[idx + 1];
    const auto &final_ifx = fn.icodes[idx + 2];
    if (bool_ic.op != icode_op::EQ && bool_ic.op != icode_op::NE)
        return false;
    if (final_ifx.op != icode_op::IFX || final_ifx.true_lbl.empty())
        return false;
    if (!cmp_ic.result.is_temp() || !bool_ic.result.is_temp() ||
        !final_ifx.left.is_temp())
        return false;
    if (bool_ic.result.temp_id != final_ifx.left.temp_id)
        return false;
    if (temp_value_used_after(fn, idx + 2, bool_ic.result.temp_id))
        return false;

    bool cmp_on_left = bool_ic.left.is_temp() &&
                       bool_ic.left.temp_id == cmp_ic.result.temp_id;
    bool cmp_on_right = bool_ic.right.is_temp() &&
                        bool_ic.right.temp_id == cmp_ic.result.temp_id;
    if (!cmp_on_left && !cmp_on_right)
        return false;
    const operand &other = cmp_on_left ? bool_ic.right : bool_ic.left;
    if (other.kind != operand_kind::INT_CONST)
        return false;
    if (temp_value_used_after(fn, idx + 1, cmp_ic.result.temp_id))
        return false;

    bool invert = false;
    if ((bool_ic.op == icode_op::NE && other.ival == 0) ||
        (bool_ic.op == icode_op::EQ && other.ival == 1)) {
        invert = false;
    } else if ((bool_ic.op == icode_op::EQ && other.ival == 0) ||
               (bool_ic.op == icode_op::NE && other.ival == 1)) {
        invert = true;
    } else {
        return false;
    }

    return emit_fused(cmp_ic, invert ? invert_compare(cmp_ic.op) : cmp_ic.op,
                      final_ifx, 2);
}

bool z80_gen::try_emit_switch_jump_table(const ir_function &fn, size_t &idx) {
    if (!switch_jump_tables_enabled() || idx + 2 >= fn.icodes.size())
        return false;

    struct switch_case_info {
        int64_t value = 0;
        std::string label;
    };

    operand cond;
    bool cond_set = false;
    std::vector<switch_case_info> cases;
    size_t pos = idx;

    while (pos + 1 < fn.icodes.size()) {
        const auto &cmp_ic = fn.icodes[pos];
        const auto &ifx_ic = fn.icodes[pos + 1];
        if (cmp_ic.op != icode_op::EQ || ifx_ic.op != icode_op::IFX)
            break;
        if (ifx_ic.true_lbl.empty() || !ifx_ic.false_lbl.empty())
            break;
        if (!cmp_ic.result.is_temp() || !ifx_ic.left.is_temp() ||
            cmp_ic.result.temp_id != ifx_ic.left.temp_id)
            break;
        if (temp_value_used_after(fn, pos + 2, cmp_ic.result.temp_id))
            return false;

        operand cur_cond;
        int64_t case_value = 0;
        if (!extract_const_compare_case(cmp_ic, cur_cond, case_value))
            break;
        if (!cur_cond.type || !cur_cond.type->is_integer() || op_size(cur_cond) > 2)
            return false;

        if (!cond_set) {
            cond = cur_cond;
            cond_set = true;
        } else if (!operands_equivalent(cur_cond, cond)) {
            break;
        }

        cases.push_back({case_value, ifx_ic.true_lbl});
        pos += 2;
    }

    if (cases.size() < 4 || pos >= fn.icodes.size())
        return false;

    const auto &tail = fn.icodes[pos];
    if (tail.op != icode_op::GOTO || tail.label_name.empty())
        return false;

    std::vector<int64_t> values;
    values.reserve(cases.size());
    for (const auto &entry : cases)
        values.push_back(entry.value);
    std::sort(values.begin(), values.end());
    if (std::adjacent_find(values.begin(), values.end()) != values.end())
        return false;

    const int64_t min_value = values.front();
    const int64_t max_value = values.back();
    if (max_value < min_value)
        return false;
    const int64_t span64 = max_value - min_value + 1;
    if (span64 <= 0 || span64 > 16)
        return false;
    const size_t span = static_cast<size_t>(span64);
    if (span > cases.size() * 2)
        return false;

    if (op_size(cond) == 1 && (min_value < 0 || max_value > 0xFF))
        return false;

    std::vector<std::string> table_labels(span, tail.label_name);
    for (const auto &entry : cases)
        table_labels[static_cast<size_t>(entry.value - min_value)] = entry.label;

    if (debug_)
        debug_->emit_location(fn.icodes[idx].line);

    emit_comment("O3 jump-table switch (%zu cases, span=%zu)",
                 cases.size(), span);

    if (op_size(cond) == 1) {
        load_a(cond);
        if (min_value != 0)
            emit_line("sub\t%s", asm_.imm(min_value).c_str());
        emit_line("cp\t%s", asm_.imm(static_cast<long long>(span)).c_str());
        emit_line("jp\tnc, %s", tail.label_name.c_str());
        emit_line("add\ta, a");
        emit_line("ld\te, a");
        emit_line("ld\td, %s", asm_.imm(0).c_str());
    } else {
        load_hl(cond);
        if (min_value != 0) {
            emit_line("ld\tde, %s", asm_.imm(min_value).c_str());
            emit_line("or\ta, a");
            emit_line("sbc\thl, de");
        }
        emit_line("ld\ta, h");
        emit_line("or\ta, a");
        emit_line("jp\tnz, %s", tail.label_name.c_str());
        emit_line("ld\ta, l");
        emit_line("cp\t%s", asm_.imm(static_cast<long long>(span)).c_str());
        emit_line("jp\tnc, %s", tail.label_name.c_str());
        emit_line("add\ta, a");
        emit_line("ld\te, a");
        emit_line("ld\td, %s", asm_.imm(0).c_str());
    }

    const std::string table_lbl =
        "__" + fn.name + "_swtab_" + std::to_string(idx);
    emit_line("ld\thl, %s", asm_.imm_sym(table_lbl).c_str());
    emit_line("add\thl, de");
    emit_line("ld\te, (hl)");
    emit_line("inc\thl");
    emit_line("ld\td, (hl)");
    emit_line("ex\tde, hl");
    emit_line("jp\t(hl)");

    emit_label(table_lbl, false);
    for (const auto &lbl : table_labels)
        asm_.dw_sym(lbl);

    idx = pos;
    return true;
}

// ----- Icode dispatch ------------------------------------------------

void z80_gen::gen_icode(const icode &ic) {
    if (cur_fn_)
        cur_ic_index_ = static_cast<size_t>(&ic - cur_fn_->icodes.data());
    if (debug_) debug_->emit_location(ic.line);
    switch (ic.op) {
    case icode_op::LABEL:        gen_label(ic);        break;
    case icode_op::GOTO:         gen_goto(ic);         break;
    case icode_op::IFX:          gen_ifx(ic);          break;
    case icode_op::FUNCTION:     gen_function(ic);     break;
    case icode_op::ENDFUNCTION:  gen_endfunction(ic);  break;
    case icode_op::RETURN:       gen_return(ic);       break;
    case icode_op::SEND:         gen_send(ic);         break;
    case icode_op::RECEIVE:      gen_receive(ic);      break;
    case icode_op::CALL:         gen_call(ic);         break;
    case icode_op::ASSIGN:       gen_assign(ic);       break;
    case icode_op::ADDRESS_OF:   gen_address_of(ic);   break;
    case icode_op::GET_VALUE_AT: gen_get_value_at(ic); break;
    case icode_op::SET_VALUE_AT: gen_set_value_at(ic); break;
    case icode_op::ADD:          gen_add(ic);          break;
    case icode_op::SUB:          gen_sub(ic);          break;
    case icode_op::MUL:          gen_mul(ic);          break;
    case icode_op::DIV:          gen_div_mod(ic, false); break;
    case icode_op::MOD:          gen_div_mod(ic, true);  break;
    case icode_op::NEG:          gen_neg(ic);          break;
    case icode_op::BAND:         gen_band(ic);         break;
    case icode_op::BOR:          gen_bor(ic);          break;
    case icode_op::BXOR:         gen_bxor(ic);         break;
    case icode_op::BNOT:         gen_bnot(ic);         break;
    case icode_op::SHL:          gen_shift(ic, false, false); break;
    case icode_op::SHR:
        if ((ic.result.type && ic.result.type->is_unsigned()) ||
            (ic.left.type && ic.left.type->is_unsigned()))
            gen_shift(ic, true, false);
        else
            gen_shift(ic, true, true);
        break;
    case icode_op::ROL:          gen_rotate(ic, false); break;
    case icode_op::ROR:          gen_rotate(ic, true);  break;
    case icode_op::PACK_BYTES:   gen_pack_bytes(ic);    break;
    case icode_op::EQ:  gen_compare(ic, icode_op::EQ); break;
    case icode_op::NE:  gen_compare(ic, icode_op::NE); break;
    case icode_op::LT:  gen_compare(ic, icode_op::LT); break;
    case icode_op::LE:  gen_compare(ic, icode_op::LE); break;
    case icode_op::GT:  gen_compare(ic, icode_op::GT); break;
    case icode_op::GE:  gen_compare(ic, icode_op::GE); break;
    case icode_op::CAST:         gen_cast(ic);         break;
    case icode_op::FADD:
    case icode_op::FSUB:
    case icode_op::FMUL:
    case icode_op::FDIV:
    case icode_op::FITOSF:
    case icode_op::FSTOI:        gen_float_arith(ic);  break;
    case icode_op::ALLOCA:       gen_alloca(ic);       break;
    case icode_op::INLINE_ASM:   gen_inline_asm(ic);   break;
    case icode_op::MAKE_COMPLEX: gen_make_complex(ic); break;
    }
}

} // namespace xcc
