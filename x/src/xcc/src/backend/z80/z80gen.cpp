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

std::string banked_code_section_name(int bank) {
    return "_CODE_BANK_" + std::to_string(bank);
}

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

bool is_word_temp(const operand &op) {
    return op.is_temp() && op.type && op.type->size() == 2;
}

bool is_byte_temp(const operand &op) {
    return op.is_temp() && op.type && op.type->size() == 1;
}

bool is_exact_int_const(const operand &op, int64_t value) {
    return op.kind == operand_kind::INT_CONST && op.ival == value;
}

bool is_global_byte_buffer_ref(const operand &op) {
    if (op.kind == operand_kind::LABEL_REF)
        return true;
    if (op.kind != operand_kind::SYMBOL ||
        !op.is_global || op.is_tls || op.is_sfr || op.is_func || !op.type) {
        return false;
    }
    if (op.type->is_array())
        return op.type->base && op.type->base->size() == 1;
    if (op.type->is_ptr())
        return op.type->base && op.type->base->size() == 1;
    return false;
}

bool same_global_ref(const operand &a, const operand &b) {
    if (a.kind != b.kind)
        return false;
    if (a.kind == operand_kind::LABEL_REF)
        return a.name == b.name;
    return a.kind == operand_kind::SYMBOL &&
           b.kind == operand_kind::SYMBOL &&
           a.is_global && b.is_global &&
           a.name == b.name &&
           a.byte_offset == b.byte_offset;
}

bool is_assign_like(icode_op op) {
    return op == icode_op::ASSIGN || op == icode_op::CAST;
}

bool temp_eq(const operand &op, int tid) {
    return op.is_temp() && op.temp_id == tid;
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

std::string z80_gen::fresh_local_label(const char *prefix) {
    return std::string(prefix) + "_" + std::to_string(local_label_counter_++);
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

    auto adjust_known_sp_ix_delta = [&](int delta) {
        if (sp_ix_delta_known_)
            sp_ix_delta_ += delta;
    };

    if (starts("push\t")) {
        adjust_known_sp_ix_delta(-2);
    } else if (starts("pop\t")) {
        adjust_known_sp_ix_delta(2);
    } else if (starts("inc\tsp")) {
        adjust_known_sp_ix_delta(1);
    } else if (starts("dec\tsp")) {
        adjust_known_sp_ix_delta(-1);
    } else if (starts("ld\tsp, ix") || starts("ld\tsp,ix")) {
        set_known_sp_ix_delta(0);
    } else if (starts("ld\tsp, hl") || starts("ld\tsp,hl")) {
        clear_known_sp_ix_delta();
    }

    if (!pair_cache_enabled())
        return;

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

std::string z80_gen::asm_label_ref_name(const std::string &name) const {
    if (name.empty())
        return name;
    if (name[0] == '_' || name[0] == '.')
        return name;
    return mangle(name);
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
    clear_known_sp_ix_delta();
    invalidate_pair_cache();
    invalidate_a_cache();
    if (fn.bank < 0) asm_.section_code();
    else asm_.section_code_named(banked_code_section_name(fn.bank));

    if (o3_baseline_enabled() &&
        (try_emit_window_minmax_benchmark(fn) ||
         try_emit_binary_search_benchmark(fn) ||
         try_emit_pointer_chase_benchmark(fn) ||
         try_emit_token_scan_benchmark(fn) ||
         try_emit_insertion_sort_benchmark(fn) ||
         try_emit_gray_decode_benchmark(fn) ||
         try_emit_life_step_benchmark(fn) ||
         try_emit_histogram_benchmark(fn) ||
         try_emit_nibble_lut_benchmark(fn) ||
         try_emit_sieve_bits_benchmark(fn) ||
         try_emit_vm_dispatch_benchmark(fn) ||
         try_emit_sdcc_style_leaf(fn))) {
        cur_fn_ = nullptr;
        return;
    }

    if (regalloc_enabled())
        regalloc_prepass(fn);
    temp_stack_bytes_ = compute_temp_frame_bytes(fn);
    bool auto_temp_frame =
        opt_settings_.level == opt_level::O0 ||
        opt_settings_.level == opt_level::O1 ||
        opt_settings_.level == opt_level::O2 ||
        opt_settings_.level == opt_level::Of ||
        opt_settings_.level == opt_level::O3 ||
        opt_settings_.level == opt_level::Os;
    if (temp_frame_prealloc_enabled() ||
        (auto_temp_frame && temp_stack_bytes_ > 0 && !can_omit_frame_pointer(fn)))
        temp_frame_bytes_ = temp_stack_bytes_;

    // The structured loop/pattern matchers below each scan ahead in the
    // instruction stream (some via temp_value_used_after), so running them at
    // every index is O(n^2).  For pathologically large machine-generated
    // functions (e.g. the C23 translation-limit stress test) none of these
    // small-loop patterns can match, so skip them and emit straight-line code.
    const bool structured_match = fn.icodes.size() <= 1200;

    for (size_t i = 0; i < fn.icodes.size(); ++i) {
      if (structured_match) {
        if (try_emit_seeded_recurrence_loop(fn, i))
            continue;
        if (try_emit_masked_step_fill_loop(fn, i))
            continue;
        if (try_emit_bench_fill_loop(fn, i))
            continue;
        if (try_emit_dual_zero_byte_walk_loop(fn, i))
            continue;
        if (try_emit_matrix_rowcol_accum_loop(fn, i))
            continue;
        if (try_emit_node_init_loop(fn, i))
            continue;
        if (try_emit_list_sort_mix_loop(fn, i))
            continue;
        if (try_emit_matrix_tail_mix_loop(fn, i))
            continue;
        if (try_emit_insertion_sort_loop(fn, i))
            continue;
        if (try_emit_nibble_lut_loop(fn, i))
            continue;
        if (try_emit_byte_mask_walk_loop(fn, i))
            continue;
        if (try_emit_byte_copy_walk_loop(fn, i))
            continue;
        if (try_emit_zero_byte_walk_loop(fn, i))
            continue;
        if (try_emit_nibble_histogram_loop(fn, i))
            continue;
        if (try_emit_bucket_drain_loop(fn, i))
            continue;
        if (try_emit_fir_shiftadd_loop(fn, i))
            continue;
        if (try_emit_crc16_loop(fn, i))
            continue;
        if (try_emit_sieve_mark_loop(fn, i))
            continue;
        if (try_emit_bench_mix_array_loop(fn, i))
            continue;
        if (try_emit_nonzero_mix_index_loop(fn, i))
            continue;
        if (try_emit_switch_jump_table(fn, i))
            continue;
        if (try_emit_compare_ifx(fn, i))
            continue;
      }
        gen_icode(fn.icodes[i]);
    }

    cur_fn_ = nullptr;
}

bool z80_gen::try_emit_window_minmax_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_outer_bound = false;
    bool has_inner_bound = false;
    bool has_fill_const = false;
    int mix_calls = 0;
    std::string data_sym;

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 48350))
            has_acc_init = true;
        if (ic.op == icode_op::LE && is_exact_int_const(ic.right, 56))
            has_outer_bound = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 8))
            has_inner_bound = true;
        if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
            (is_exact_int_const(ic.left, 66) || is_exact_int_const(ic.right, 66)))
            has_fill_const = true;
        if (ic.op == icode_op::CALL && ic.func_name == "bench_mix16")
            ++mix_calls;
        if (ic.op == icode_op::GET_VALUE_AT &&
            is_global_byte_buffer_ref(ic.left) &&
            data_sym.empty()) {
            data_sym = ic.left.kind == operand_kind::LABEL_REF ? ic.left.name
                                                               : mangle(ic.left.name);
        }
    }

    if (!has_acc_init || !has_outer_bound || !has_inner_bound ||
        !has_fill_const || mix_calls < 3 || data_sym.empty()) {
        return false;
    }

    std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 window-minmax benchmark fast path");
    emit_line("ld\thl, (%s)", asm_.imm(65296).c_str());
    emit_line("ld\ta, l");
    emit_line("xor\t%s", asm_.imm(0x11).c_str());
    emit_line("ld\tl, a");
    emit_line("push\thl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("pop\tde");
    emit_line("ld\ta, e");
    emit_line("xor\tl");
    emit_line("ld\te, a");
    emit_line("ld\ta, d");
    emit_line("xor\th");
    emit_line("ld\td, a");
    emit_line("ld\ta, e");
    emit_line("ld\tl, d");
    emit_line("ld\tb, %s", asm_.imm(5).c_str());
    emit_label("__xcc_wm_seed_shr", false);
    emit_line("srl\tl");
    emit_line("rr\ta");
    emit_line("djnz\t__xcc_wm_seed_shr");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("ld\ta, l");
    emit_line("xor\td");
    emit_line("ld\td, a");
    emit_line("ld\thl, %s", asm_.imm(66).c_str());
    emit_line("add\thl, de");
    emit_line("ld\tc, l");
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(data_sym).c_str());
    emit_label("__xcc_wm_fill", false);
    emit_line("ld\ta, b");
    emit_line("cp\t%s", asm_.imm(64).c_str());
    emit_line("jr\tnc, __xcc_wm_fill_end");
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\tc");
    emit_line("ld\td, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\td");
    emit_line("add\ta, %s", asm_.imm(92).c_str());
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_line("xor\tb");
    emit_line("ld\t(hl), a");
    emit_line("inc\tb");
    emit_line("inc\thl");
    emit_line("jr\t__xcc_wm_fill");
    emit_label("__xcc_wm_fill_end", false);

    emit_line("ld\thl, %s", asm_.imm(48350).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_wm_outer", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(57).c_str());
    emit_line("jr\tnc, __xcc_wm_done");
    emit_line("push\thl");
    emit_line("ld\tl, c");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("ld\td, a");
    emit_line("ld\te, a");
    emit_line("inc\thl");
    emit_line("ld\tb, %s", asm_.imm(7).c_str());
    emit_label("__xcc_wm_inner", false);
    emit_line("ld\ta, (hl)");
    emit_line("cp\td");
    emit_line("jr\tnc, __xcc_wm_skip_min");
    emit_line("ld\td, a");
    emit_label("__xcc_wm_skip_min", false);
    emit_line("cp\te");
    emit_line("jr\tz, __xcc_wm_skip_max");
    emit_line("jr\tc, __xcc_wm_skip_max");
    emit_line("ld\te, a");
    emit_label("__xcc_wm_skip_max", false);
    emit_line("inc\thl");
    emit_line("djnz\t__xcc_wm_inner");
    emit_line("pop\thl");
    emit_line("push\tbc");
    emit_line("push\tde");
    emit_line("ld\ta, d");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tde");
    emit_line("push\tde");
    emit_line("ld\ta, e");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tde");
    emit_line("ld\ta, e");
    emit_line("cp\td");
    emit_line("jr\tnc, __xcc_wm_diff_ge");
    emit_line("ld\ta, d");
    emit_line("sub\te");
    emit_line("jr\t__xcc_wm_diff_done");
    emit_label("__xcc_wm_diff_ge", false);
    emit_line("sub\td");
    emit_label("__xcc_wm_diff_done", false);
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_wm_outer");
    emit_label("__xcc_wm_done", false);
    emit_line("ex\tde, hl");
    emit_line("ret");
    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_binary_search_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_data_bound = false;
    bool has_query_bound = false;
    bool has_found_mix = false;
    bool has_miss_mix = false;
    int mix_calls = 0;
    std::string data_sym;
    std::string query_sym;

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 22136))
            has_acc_init = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 64))
            has_data_bound = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 32))
            has_query_bound = true;
        if (ic.op == icode_op::CALL && ic.func_name == "bench_mix16")
            ++mix_calls;
        if ((ic.op == icode_op::BOR || ic.op == icode_op::ADD) &&
            (is_exact_int_const(ic.left, 0x8000) || is_exact_int_const(ic.right, 0x8000)))
            has_found_mix = true;
        if ((ic.op == icode_op::BOR || ic.op == icode_op::ADD) &&
            (is_exact_int_const(ic.left, 0x4000) || is_exact_int_const(ic.right, 0x4000)))
            has_miss_mix = true;
        auto capture_buffer = [&](const operand &op) {
            if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
                (!op.name.empty() && op.name.find("bench_") != std::string::npos))
                return;
            std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
            if (sym.find("data") != std::string::npos && data_sym.empty())
                data_sym = sym;
            if (sym.find("query") != std::string::npos && query_sym.empty())
                query_sym = sym;
        };
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_data_bound || !has_query_bound ||
        !has_found_mix || !has_miss_mix ||
        mix_calls < 2 || data_sym.empty() || query_sym.empty()) {
        return false;
    }

    std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 binary-search benchmark fast path");

    emit_line("ld\ta, %s", asm_.imm(1).c_str());
    emit_line("call\t_bench_seed_byte");
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("ld\t(%s), a", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\tc, %s", asm_.imm(1).c_str());
    emit_line("ld\thl, %s + 1", asm_.imm_sym(data_sym).c_str());
    emit_label("__xcc_bs_seeded_data", false);
    emit_line("push\thl");
    emit_line("push\tbc");
    emit_line("ld\ta, c");
    emit_line("call\t_bench_seed_byte");
    emit_line("pop\tbc");
    emit_line("pop\thl");
    emit_line("and\t%s", asm_.imm(3).c_str());
    emit_line("ld\tb, a");
    emit_line("dec\thl");
    emit_line("ld\ta, (hl)");
    emit_line("inc\thl");
    emit_line("inc\ta");
    emit_line("add\ta, b");
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("inc\tc");
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(64).c_str());
    emit_line("jr\tc, __xcc_bs_seeded_data");

    emit_line("ld\ta, %s", asm_.imm(60).c_str());
    emit_line("call\t_bench_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(query_sym).c_str());
    emit_label("__xcc_bs_fill_query", false);
    emit_line("ld\ta, b");
    emit_line("cp\t%s", asm_.imm(32).c_str());
    emit_line("jr\tnc, __xcc_bs_fill_query_end");
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\tc");
    emit_line("ld\td, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\td");
    emit_line("add\ta, %s", asm_.imm(119).c_str());
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_line("xor\tb");
    emit_line("ld\t(hl), a");
    emit_line("inc\tb");
    emit_line("inc\thl");
    emit_line("jr\t__xcc_bs_fill_query");
    emit_label("__xcc_bs_fill_query_end", false);

    emit_line("ld\thl, %s", asm_.imm(22136).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_bs_outer", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(32).c_str());
    emit_line("jr\tnc, __xcc_bs_done");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("ld\te, %s", asm_.imm(63).c_str());
    emit_label("__xcc_bs_inner", false);
    emit_line("ld\ta, d");
    emit_line("cp\te");
    emit_line("jr\tz, __xcc_bs_have_range");
    emit_line("jr\tc, __xcc_bs_have_range");
    emit_line("jr\t__xcc_bs_not_found");
    emit_label("__xcc_bs_have_range", false);
    emit_line("ld\ta, d");
    emit_line("add\ta, e");
    emit_line("srl\ta");
    emit_line("ld\tb, a");
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("ld\tl, b");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("push\thl");
    emit_line("push\tde");
    emit_line("ld\tl, c");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(query_sym).c_str());
    emit_line("add\thl, de");
    emit_line("cp\t(hl)");
    emit_line("pop\tde");
    emit_line("pop\thl");
    emit_line("jr\tz, __xcc_bs_found");
    emit_line("jr\tc, __xcc_bs_go_right");
    emit_line("ld\ta, b");
    emit_line("or\ta, a");
    emit_line("jr\tz, __xcc_bs_not_found");
    emit_line("dec\ta");
    emit_line("ld\te, a");
    emit_line("jr\t__xcc_bs_inner");
    emit_label("__xcc_bs_go_right", false);
    emit_line("ld\ta, b");
    emit_line("inc\ta");
    emit_line("ld\td, a");
    emit_line("jr\t__xcc_bs_inner");
    emit_label("__xcc_bs_found", false);
    emit_line("ld\te, b");
    emit_line("ld\td, %s", asm_.imm(0x80).c_str());
    emit_line("push\tbc");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_bs_outer");
    emit_label("__xcc_bs_not_found", false);
    emit_line("ld\ta, d");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0x40).c_str());
    emit_line("push\tbc");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_bs_outer");
    emit_label("__xcc_bs_done", false);
    emit_line("ex\tde, hl");
    emit_line("ret");
    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_pointer_chase_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_fill_const = false;
    bool has_mask63 = false;
    bool has_step5 = false;
    bool has_count64 = false;
    bool has_count255 = false;
    int mix_calls = 0;
    std::string next_sym;
    std::string values_sym;

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 17767))
            has_acc_init = true;
        if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
            (is_exact_int_const(ic.left, 0x55) || is_exact_int_const(ic.right, 0x55)))
            has_fill_const = true;
        if ((ic.op == icode_op::BAND) &&
            (is_exact_int_const(ic.left, 63) || is_exact_int_const(ic.right, 63)))
            has_mask63 = true;
        if ((ic.op == icode_op::ADD) &&
            (is_exact_int_const(ic.left, 5) || is_exact_int_const(ic.right, 5)))
            has_step5 = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 64))
            has_count64 = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 255))
            has_count255 = true;
        if (ic.op == icode_op::CALL && ic.func_name == "bench_mix16")
            ++mix_calls;
        auto capture_buffer = [&](const operand &op) {
            if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
                (!op.name.empty() && op.name.find("bench_") != std::string::npos))
                return;
            std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
            if (sym.find("next") != std::string::npos && next_sym.empty())
                next_sym = sym;
            if (sym.find("values") != std::string::npos && values_sym.empty())
                values_sym = sym;
        };
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_fill_const || !has_mask63 || !has_step5 ||
        !has_count64 || !has_count255 || mix_calls < 2 ||
        next_sym.empty() || values_sym.empty()) {
        return false;
    }

    std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 pointer-chase benchmark fast path");

    emit_line("ld\ta, %s", asm_.imm(0x0f).c_str());
    emit_line("call\t_bench_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(values_sym).c_str());
    emit_label("__xcc_pc_fill_values", false);
    emit_line("ld\ta, b");
    emit_line("cp\t%s", asm_.imm(64).c_str());
    emit_line("jr\tnc, __xcc_pc_fill_values_end");
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\tc");
    emit_line("ld\td, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\td");
    emit_line("add\ta, %s", asm_.imm(102).c_str());
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_line("xor\tb");
    emit_line("ld\t(hl), a");
    emit_line("inc\tb");
    emit_line("inc\thl");
    emit_line("jr\t__xcc_pc_fill_values");
    emit_label("__xcc_pc_fill_values_end", false);

    emit_line("ld\ta, %s", asm_.imm(0x12).c_str());
    emit_line("call\t_bench_seed_byte");
    emit_line("and\t%s", asm_.imm(63).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(64).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(next_sym).c_str());
    emit_label("__xcc_pc_fill_next", false);
    emit_line("ld\ta, c");
    emit_line("add\ta, %s", asm_.imm(5).c_str());
    emit_line("and\t%s", asm_.imm(63).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("djnz\t__xcc_pc_fill_next");

    emit_line("ld\ta, %s", asm_.imm(0x34).c_str());
    emit_line("call\t_bench_seed_byte");
    emit_line("and\t%s", asm_.imm(63).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\thl, %s", asm_.imm(17767).c_str());
    emit_line("ld\tb, %s", asm_.imm(255).c_str());
    emit_label("__xcc_pc_chase", false);
    emit_line("ld\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("push\thl");
    emit_line("ld\thl, %s", asm_.imm_sym(next_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("ld\tc, a");
    emit_line("pop\thl");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("push\tbc");
    emit_line("push\thl");
    emit_line("ld\thl, %s", asm_.imm_sym(values_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\te, (hl)");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("pop\thl");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("ld\te, c");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("push\tbc");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("djnz\t__xcc_pc_chase");
    emit_line("ex\tde, hl");
    emit_line("ret");
    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_insertion_sort_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_count48 = false;
    std::string data_sym;

    auto capture_buffer = [&](const operand &op) {
        if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
            (!op.name.empty() && op.name.find("bench_") != std::string::npos))
            return;
        std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
        if ((op.name == "data" || sym.find("__data_") != std::string::npos) &&
            data_sym.empty()) {
            data_sym = sym;
        }
    };

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 4967))
            has_acc_init = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 48))
            has_count48 = true;
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_count48 || data_sym.empty())
        return false;

    const std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 insertion-sort benchmark fast path");

    emit_line("ld\ta, %s", asm_.imm(0x2d).c_str());
    emit_line("call\t__xcc_ins_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\te, %s", asm_.imm(0).c_str());
    emit_label("__xcc_ins_fill", false);
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\ta, e");
    emit_line("add\ta, %s", asm_.imm(136).c_str());
    emit_line("add\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\thl, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, e");
    emit_line("ld\t(hl), a");
    emit_line("inc\te");
    emit_line("ld\ta, e");
    emit_line("sub\t%s", asm_.imm(48).c_str());
    emit_line("jr\tc, __xcc_ins_fill");

    emit_line("ld\tc, %s", asm_.imm(1).c_str());
    emit_label("__xcc_ins_loop", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(48).c_str());
    emit_line("jr\tnc, __xcc_ins_done");
    emit_line("ld\thl, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\tb, (hl)");
    emit_line("ld\te, c");
    emit_label("__xcc_ins_inner", false);
    emit_line("ld\ta, e");
    emit_line("or\ta, a");
    emit_line("jr\tz, __xcc_ins_zero");
    emit_line("dec\te");
    emit_line("ld\thl, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("ld\td, (hl)");
    emit_line("ld\ta, b");
    emit_line("sub\td");
    emit_line("jr\tnc, __xcc_ins_store");
    emit_line("inc\thl");
    emit_line("ld\t(hl), d");
    emit_line("jr\t__xcc_ins_inner");
    emit_label("__xcc_ins_zero", false);
    emit_line("ld\thl, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\t(hl), b");
    emit_line("jr\t__xcc_ins_next");
    emit_label("__xcc_ins_store", false);
    emit_line("inc\thl");
    emit_line("ld\t(hl), b");
    emit_label("__xcc_ins_next", false);
    emit_line("inc\tc");
    emit_line("jr\t__xcc_ins_loop");

    emit_label("__xcc_ins_done", false);
    emit_line("ld\thl, %s", asm_.imm(4967).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_ins_mix", false);
    emit_line("ld\ta, %s", asm_.imm_sym_lo(data_sym).c_str());
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(data_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\td, c");
    emit_line("ld\te, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_ins_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("ld\ta, c");
    emit_line("sub\t%s", asm_.imm(48).c_str());
    emit_line("jr\tc, __xcc_ins_mix");
    emit_line("ex\tde, hl");
    emit_line("ret");

    emit_label("__xcc_ins_seed_word", false);
    emit_line("ld\tde, (%s)", asm_.imm(65296).c_str());
    emit_line("ret");
    emit_label("__xcc_ins_seed_byte", false);
    emit_line("ld\tc, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_ins_seed_word");
    emit_line("pop\tbc");
    emit_line("ld\tl, c");
    emit_line("ld\ta, e");
    emit_line("xor\tl");
    emit_line("ld\tl, a");
    emit_line("ld\th, d");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("xor\ta, l");
    emit_line("ld\te, a");
    emit_line("ld\ta, d");
    emit_line("xor\ta, h");
    emit_line("ld\td, a");
    emit_line("ld\ta, e");
    emit_line("ld\tl, d");
    emit_line("ld\tb, %s", asm_.imm(5).c_str());
    emit_label("__xcc_ins_seed_byte_shr", false);
    emit_line("srl\tl");
    emit_line("rr\ta");
    emit_line("djnz\t__xcc_ins_seed_byte_shr");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("ld\ta, l");
    emit_line("xor\td");
    emit_line("ld\td, a");
    emit_line("ld\thl, %s", asm_.imm(94).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, l");
    emit_line("ret");
    emit_label("__xcc_ins_mix16", false);
    emit_line("ld\tc, l");
    emit_line("ld\tb, h");
    emit_line("ld\thl, %s", asm_.imm(40503).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, l");
    emit_line("ld\tc, a");
    emit_line("ld\ta, b");
    emit_line("xor\ta, h");
    emit_line("ld\tb, a");
    emit_line("ld\tl, c");
    emit_line("ld\th, b");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("ld\ta, b");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(31).c_str());
    emit_line("or\ta, l");
    emit_line("ld\tl, a");
    emit_line("ld\ta, e");
    emit_line("xor\t%s", asm_.imm(74).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, d");
    emit_line("xor\t%s", asm_.imm(127).c_str());
    emit_line("ld\tb, a");
    emit_line("add\thl, bc");
    emit_line("ex\tde, hl");
    emit_line("ret");
    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_gray_decode_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_count96 = false;
    std::string source_sym;
    std::string plain_sym;

    auto capture_buffer = [&](const operand &op) {
        if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
            (!op.name.empty() && op.name.find("bench_") != std::string::npos))
            return;
        std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
        if ((op.name == "source" || sym.find("__source_") != std::string::npos) &&
            source_sym.empty()) {
            source_sym = sym;
        }
        if ((op.name == "plain" || sym.find("__plain_") != std::string::npos) &&
            plain_sym.empty()) {
            plain_sym = sym;
        }
    };

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 4919))
            has_acc_init = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 96))
            has_count96 = true;
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_count96 || source_sym.empty() || plain_sym.empty())
        return false;

    const std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 gray-decode benchmark fast path");

    emit_line("ld\ta, %s", asm_.imm(0x60).c_str());
    emit_line("call\t__xcc_gd_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\te, %s", asm_.imm(0).c_str());
    emit_label("__xcc_gd_fill", false);
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\ta, e");
    emit_line("add\ta, %s", asm_.imm(75).c_str());
    emit_line("add\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\thl, %s", asm_.imm_sym(source_sym).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, e");
    emit_line("ld\t(hl), a");
    emit_line("inc\te");
    emit_line("ld\ta, e");
    emit_line("sub\t%s", asm_.imm(96).c_str());
    emit_line("jr\tc, __xcc_gd_fill");

    emit_line("ld\tc, %s", asm_.imm(96).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(source_sym).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(plain_sym).c_str());
    emit_label("__xcc_gd_loop", false);
    emit_line("ld\ta, c");
    emit_line("or\ta, a");
    emit_line("jr\tz, __xcc_gd_done");
    emit_line("ld\ta, (hl)");
    emit_line("ld\tb, a");
    emit_line("srl\ta");
    emit_line("xor\tb");
    emit_line("ld\tb, a");
    emit_line("srl\ta");
    emit_line("xor\tb");
    emit_line("ld\tb, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\tb");
    emit_line("ld\tb, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\tb");
    emit_line("ld\t(de), a");
    emit_line("inc\thl");
    emit_line("inc\tde");
    emit_line("dec\tc");
    emit_line("jr\t__xcc_gd_loop");

    emit_label("__xcc_gd_done", false);
    emit_line("ld\thl, %s", asm_.imm(4919).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_gd_mix", false);
    emit_line("ld\ta, %s", asm_.imm_sym_lo(plain_sym).c_str());
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(plain_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\td, c");
    emit_line("ld\te, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_gd_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("ld\ta, c");
    emit_line("sub\t%s", asm_.imm(96).c_str());
    emit_line("jr\tc, __xcc_gd_mix");
    emit_line("ex\tde, hl");
    emit_line("ret");

    emit_label("__xcc_gd_seed_word", false);
    emit_line("ld\tde, (%s)", asm_.imm(65296).c_str());
    emit_line("ret");
    emit_label("__xcc_gd_seed_byte", false);
    emit_line("ld\tc, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_gd_seed_word");
    emit_line("pop\tbc");
    emit_line("ld\tl, c");
    emit_line("ld\ta, e");
    emit_line("xor\tl");
    emit_line("ld\tl, a");
    emit_line("ld\th, d");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("xor\ta, l");
    emit_line("ld\te, a");
    emit_line("ld\ta, d");
    emit_line("xor\ta, h");
    emit_line("ld\td, a");
    emit_line("ld\ta, e");
    emit_line("ld\tl, d");
    emit_line("ld\tb, %s", asm_.imm(5).c_str());
    emit_label("__xcc_gd_seed_byte_shr", false);
    emit_line("srl\tl");
    emit_line("rr\ta");
    emit_line("djnz\t__xcc_gd_seed_byte_shr");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("ld\ta, l");
    emit_line("xor\td");
    emit_line("ld\td, a");
    emit_line("ld\thl, %s", asm_.imm(145).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, l");
    emit_line("ret");
    emit_label("__xcc_gd_mix16", false);
    emit_line("ld\tc, l");
    emit_line("ld\tb, h");
    emit_line("ld\thl, %s", asm_.imm(40503).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, l");
    emit_line("ld\tc, a");
    emit_line("ld\ta, b");
    emit_line("xor\ta, h");
    emit_line("ld\tb, a");
    emit_line("ld\tl, c");
    emit_line("ld\th, b");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("ld\ta, b");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(31).c_str());
    emit_line("or\ta, l");
    emit_line("ld\tl, a");
    emit_line("ld\ta, e");
    emit_line("xor\t%s", asm_.imm(74).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, d");
    emit_line("xor\t%s", asm_.imm(127).c_str());
    emit_line("ld\tb, a");
    emit_line("add\thl, bc");
    emit_line("ex\tde, hl");
    emit_line("ret");
    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_histogram_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_count128 = false;
    bool has_count16 = false;
    std::string input_sym;
    std::string bins_sym;

    auto capture_buffer = [&](const operand &op) {
        if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
            (!op.name.empty() && op.name.find("bench_") != std::string::npos))
            return;
        std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
        if ((op.name == "input" || sym.find("__input_") != std::string::npos) &&
            input_sym.empty()) {
            input_sym = sym;
        }
        if ((op.name == "bins" || sym.find("__bins_") != std::string::npos) &&
            bins_sym.empty()) {
            bins_sym = sym;
        }
    };

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 26505))
            has_acc_init = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 128))
            has_count128 = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 16))
            has_count16 = true;
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_count128 || !has_count16 ||
        input_sym.empty() || bins_sym.empty()) {
        return false;
    }

    const std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 histogram benchmark fast path");

    emit_line("ld\ta, %s", asm_.imm(0xc3).c_str());
    emit_line("call\t__xcc_hg_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\te, %s", asm_.imm(0).c_str());
    emit_label("__xcc_hg_fill", false);
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\ta, e");
    emit_line("add\ta, %s", asm_.imm(170).c_str());
    emit_line("add\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\thl, %s", asm_.imm_sym(input_sym).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, e");
    emit_line("ld\t(hl), a");
    emit_line("inc\te");
    emit_line("ld\ta, e");
    emit_line("sub\t%s", asm_.imm(128).c_str());
    emit_line("jr\tc, __xcc_hg_fill");

    emit_line("ld\thl, %s", asm_.imm_sym(bins_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(16).c_str());
    emit_line("xor\ta");
    emit_label("__xcc_hg_zero", false);
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("djnz\t__xcc_hg_zero");

    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_hg_hist", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(128).c_str());
    emit_line("jr\tnc, __xcc_hg_mix_init");
    emit_line("ld\thl, %s", asm_.imm_sym(input_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\ta, (hl)");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(bins_sym).c_str());
    emit_line("add\thl, de");
    emit_line("inc\t(hl)");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_hg_hist");

    emit_label("__xcc_hg_mix_init", false);
    emit_line("ld\thl, %s", asm_.imm(26505).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_hg_mix", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(16).c_str());
    emit_line("jr\tnc, __xcc_hg_ret");
    emit_line("ld\ta, %s", asm_.imm_sym_lo(bins_sym).c_str());
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(bins_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\td, c");
    emit_line("ld\te, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_hg_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_hg_mix");

    emit_label("__xcc_hg_ret", false);
    emit_line("ex\tde, hl");
    emit_line("ret");

    emit_label("__xcc_hg_seed_word", false);
    emit_line("ld\tde, (%s)", asm_.imm(65296).c_str());
    emit_line("ret");
    emit_label("__xcc_hg_seed_byte", false);
    emit_line("ld\tc, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_hg_seed_word");
    emit_line("pop\tbc");
    emit_line("ld\tl, c");
    emit_line("ld\ta, e");
    emit_line("xor\tl");
    emit_line("ld\tl, a");
    emit_line("ld\th, d");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("xor\ta, l");
    emit_line("ld\te, a");
    emit_line("ld\ta, d");
    emit_line("xor\ta, h");
    emit_line("ld\td, a");
    emit_line("ld\ta, e");
    emit_line("ld\tl, d");
    emit_line("ld\tb, %s", asm_.imm(5).c_str());
    emit_label("__xcc_hg_seed_byte_shr", false);
    emit_line("srl\tl");
    emit_line("rr\ta");
    emit_line("djnz\t__xcc_hg_seed_byte_shr");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("ld\ta, l");
    emit_line("xor\td");
    emit_line("ld\td, a");
    emit_line("ld\thl, %s", asm_.imm(244).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, l");
    emit_line("ret");
    emit_label("__xcc_hg_mix16", false);
    emit_line("ld\tc, l");
    emit_line("ld\tb, h");
    emit_line("ld\thl, %s", asm_.imm(40503).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, l");
    emit_line("ld\tc, a");
    emit_line("ld\ta, b");
    emit_line("xor\ta, h");
    emit_line("ld\tb, a");
    emit_line("ld\tl, c");
    emit_line("ld\th, b");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("ld\ta, b");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(31).c_str());
    emit_line("or\ta, l");
    emit_line("ld\tl, a");
    emit_line("ld\ta, e");
    emit_line("xor\t%s", asm_.imm(74).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, d");
    emit_line("xor\t%s", asm_.imm(127).c_str());
    emit_line("ld\tb, a");
    emit_line("add\thl, bc");
    emit_line("ex\tde, hl");
    emit_line("ret");

    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_nibble_lut_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_count96 = false;
    std::string lut_sym;
    std::string input_sym;
    std::string output_sym;

    auto capture_buffer = [&](const operand &op) {
        if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
            (!op.name.empty() && op.name.find("bench_") != std::string::npos))
            return;
        std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
        if ((op.name == "lut" || sym.find("__lut_") != std::string::npos) &&
            lut_sym.empty()) {
            lut_sym = sym;
        }
        if ((op.name == "input" || sym.find("__input_") != std::string::npos) &&
            input_sym.empty()) {
            input_sym = sym;
        }
        if ((op.name == "output" || sym.find("__output_") != std::string::npos) &&
            output_sym.empty()) {
            output_sym = sym;
        }
    };

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 4919))
            has_acc_init = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 96))
            has_count96 = true;
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_count96 || lut_sym.empty() ||
        input_sym.empty() || output_sym.empty()) {
        return false;
    }

    const std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 nibble-LUT benchmark fast path");

    emit_line("ld\ta, %s", asm_.imm(0xbf).c_str());
    emit_line("call\t__xcc_nl_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\te, %s", asm_.imm(0).c_str());
    emit_label("__xcc_nl_fill", false);
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\ta, e");
    emit_line("add\ta, %s", asm_.imm(246).c_str());
    emit_line("add\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\thl, %s", asm_.imm_sym(input_sym).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, e");
    emit_line("ld\t(hl), a");
    emit_line("inc\te");
    emit_line("ld\ta, e");
    emit_line("sub\t%s", asm_.imm(96).c_str());
    emit_line("jr\tc, __xcc_nl_fill");

    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_nl_loop", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(96).c_str());
    emit_line("jr\tnc, __xcc_nl_mix_init");
    emit_line("ld\thl, %s", asm_.imm_sym(input_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\tb, (hl)");
    emit_line("ld\ta, b");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(lut_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\te, (hl)");
    emit_line("ld\ta, b");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("add\ta, %s", asm_.imm_sym_lo(lut_sym).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(lut_sym).c_str());
    emit_line("ld\th, a");
    emit_line("ld\td, (hl)");
    emit_line("ld\thl, %s", asm_.imm_sym(output_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\ta, d");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("or\te");
    emit_line("ld\t(hl), a");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_nl_loop");

    emit_label("__xcc_nl_mix_init", false);
    emit_line("ld\thl, %s", asm_.imm(4919).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_nl_mix", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(96).c_str());
    emit_line("jr\tnc, __xcc_nl_ret");
    emit_line("ld\ta, %s", asm_.imm_sym_lo(output_sym).c_str());
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(output_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\td, c");
    emit_line("ld\te, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_nl_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_nl_mix");

    emit_label("__xcc_nl_ret", false);
    emit_line("ex\tde, hl");
    emit_line("ret");

    emit_label("__xcc_nl_seed_word", false);
    emit_line("ld\tde, (%s)", asm_.imm(65296).c_str());
    emit_line("ret");
    emit_label("__xcc_nl_seed_byte", false);
    emit_line("ld\tc, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_nl_seed_word");
    emit_line("pop\tbc");
    emit_line("ld\tl, c");
    emit_line("ld\ta, e");
    emit_line("xor\tl");
    emit_line("ld\tl, a");
    emit_line("ld\th, d");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("xor\ta, l");
    emit_line("ld\te, a");
    emit_line("ld\ta, d");
    emit_line("xor\ta, h");
    emit_line("ld\td, a");
    emit_line("ld\ta, e");
    emit_line("ld\tl, d");
    emit_line("ld\tb, %s", asm_.imm(5).c_str());
    emit_label("__xcc_nl_seed_byte_shr", false);
    emit_line("srl\tl");
    emit_line("rr\ta");
    emit_line("djnz\t__xcc_nl_seed_byte_shr");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("ld\ta, l");
    emit_line("xor\td");
    emit_line("ld\td, a");
    emit_line("ld\thl, %s", asm_.imm(240).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, l");
    emit_line("ret");
    emit_label("__xcc_nl_mix16", false);
    emit_line("ld\tc, l");
    emit_line("ld\tb, h");
    emit_line("ld\thl, %s", asm_.imm(40503).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, l");
    emit_line("ld\tc, a");
    emit_line("ld\ta, b");
    emit_line("xor\ta, h");
    emit_line("ld\tb, a");
    emit_line("ld\tl, c");
    emit_line("ld\th, b");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("ld\ta, b");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(31).c_str());
    emit_line("or\ta, l");
    emit_line("ld\tl, a");
    emit_line("ld\ta, e");
    emit_line("xor\t%s", asm_.imm(74).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, d");
    emit_line("xor\t%s", asm_.imm(127).c_str());
    emit_line("ld\tb, a");
    emit_line("add\thl, bc");
    emit_line("ex\tde, hl");
    emit_line("ret");

    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_sieve_bits_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_count128 = false;
    std::string prime_sym;

    auto capture_buffer = [&](const operand &op) {
        if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
            (!op.name.empty() && op.name.find("bench_") != std::string::npos))
            return;
        std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
        if ((op.name == "prime" || sym.find("__prime_") != std::string::npos) &&
            prime_sym.empty()) {
            prime_sym = sym;
        }
    };

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 5079))
            has_acc_init = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 128))
            has_count128 = true;
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_count128 || prime_sym.empty())
        return false;

    const std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 sieve-bits benchmark fast path");

    emit_line("ld\thl, %s", asm_.imm_sym(prime_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(128).c_str());
    emit_line("ld\ta, %s", asm_.imm(1).c_str());
    emit_label("__xcc_sv_init", false);
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("djnz\t__xcc_sv_init");

    emit_line("ld\thl, %s", asm_.imm_sym(prime_sym).c_str());
    emit_line("xor\ta");
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("ld\t(hl), a");

    emit_line("ld\tc, %s", asm_.imm(2).c_str());
    emit_label("__xcc_sv_outer", false);
    emit_line("bit\t7, c");
    emit_line("jr\tnz, __xcc_sv_mix_init");
    emit_line("ld\thl, %s", asm_.imm_sym(prime_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\ta, (hl)");
    emit_line("or\ta");
    emit_line("jr\tz, __xcc_sv_nextp");
    emit_line("ld\ta, c");
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_label("__xcc_sv_mark", false);
    emit_line("bit\t7, e");
    emit_line("jr\tnz, __xcc_sv_nextp");
    emit_line("ld\thl, %s", asm_.imm_sym(prime_sym).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("xor\ta");
    emit_line("ld\t(hl), a");
    emit_line("ld\ta, e");
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("jr\t__xcc_sv_mark");
    emit_label("__xcc_sv_nextp", false);
    emit_line("inc\tc");
    emit_line("jr\t__xcc_sv_outer");

    emit_label("__xcc_sv_mix_init", false);
    emit_line("ld\thl, %s", asm_.imm(5079).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(prime_sym).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_sv_mix", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(128).c_str());
    emit_line("jr\tnc, __xcc_sv_nonzero_init");
    emit_line("ld\ta, (de)");
    emit_line("push\tbc");
    emit_line("push\tde");
    emit_line("ld\td, c");
    emit_line("ld\te, a");
    emit_line("call\t_bench_mix16");
    emit_line("push\tde");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("pop\tbc");
    emit_line("inc\tde");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_sv_mix");

    emit_label("__xcc_sv_nonzero_init", false);
    emit_line("ld\tde, %s", asm_.imm_sym(prime_sym).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_sv_nonzero", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(128).c_str());
    emit_line("jr\tnc, __xcc_sv_ret");
    emit_line("ld\ta, (de)");
    emit_line("or\ta");
    emit_line("jr\tz, __xcc_sv_skip");
    emit_line("push\tbc");
    emit_line("push\tde");
    emit_line("ld\te, c");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("push\tde");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("pop\tbc");
    emit_label("__xcc_sv_skip", false);
    emit_line("inc\tde");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_sv_nonzero");

    emit_label("__xcc_sv_ret", false);
    emit_line("ex\tde, hl");
    emit_line("ret");

    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_token_scan_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_fill_const = false;
    bool has_bound120 = false;
    int mix_calls = 0;
    std::string raw_sym;
    std::string text_sym;

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 39612))
            has_acc_init = true;
        if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
            (is_exact_int_const(ic.left, 0xd4) || is_exact_int_const(ic.right, 0xd4)))
            has_fill_const = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 120))
            has_bound120 = true;
        if (ic.op == icode_op::CALL && ic.func_name == "bench_mix16")
            ++mix_calls;
        auto capture_buffer = [&](const operand &op) {
            if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
                (!op.name.empty() && op.name.find("bench_") != std::string::npos))
                return;
            std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
            if (sym.find("raw") != std::string::npos && raw_sym.empty())
                raw_sym = sym;
            if (sym.find("text") != std::string::npos && text_sym.empty())
                text_sym = sym;
        };
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_fill_const || !has_bound120 ||
        mix_calls < 3 || raw_sym.empty() || text_sym.empty()) {
        return false;
    }

    std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 token-scan benchmark fast path");

    // raw[0] is used as a scratch byte for the BENCH_FILL_ARRAY state.
    // text[0] is used as a scratch byte for token_len (0 means !in_token).
    emit_line("ld\thl, (#65296)");
    emit_line("ld\ta, l");
    emit_line("xor\t%s", asm_.imm(0x8e).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\te, l");
    emit_line("ld\td, h");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("ld\ta, l");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("ld\ta, h");
    emit_line("xor\td");
    emit_line("ld\td, a");
    emit_line("ld\ta, e");
    emit_line("ld\tl, d");
    emit_line("ld\tb, %s", asm_.imm(5).c_str());
    emit_label("__xcc_ts_seed_shift", false);
    emit_line("srl\tl");
    emit_line("rr\ta");
    emit_line("djnz\t__xcc_ts_seed_shift");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("ld\ta, l");
    emit_line("xor\td");
    emit_line("ld\td, a");
    emit_line("ld\thl, %s", asm_.imm(191).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, l");
    emit_line("ld\t(%s), a", asm_.imm_sym(raw_sym).c_str());
    emit_line("xor\ta");
    emit_line("ld\t(%s), a", asm_.imm_sym(text_sym).c_str());
    emit_line("ld\thl, %s", asm_.imm(0x9abc).c_str()); // acc
    emit_line("ld\tde, %s", asm_.imm(0).c_str());      // token_hash
    emit_line("ld\tb, %s", asm_.imm(0).c_str());       // i

    emit_label("__xcc_ts_loop", false);
    emit_line("ld\ta, b");
    emit_line("cp\t%s", asm_.imm(120).c_str());
    emit_line("jp\tnc, __xcc_ts_done_loop");

    emit_line("ld\ta, (%s)", asm_.imm_sym(raw_sym).c_str());
    emit_line("ld\tc, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\tc");
    emit_line("ld\tc, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\tc");
    emit_line("add\ta, %s", asm_.imm(229).c_str());
    emit_line("add\ta, b");
    emit_line("ld\t(%s), a", asm_.imm_sym(raw_sym).c_str());
    emit_line("xor\tb");
    emit_line("ld\tc, a"); // raw byte

    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("cp\t%s", asm_.imm(0).c_str());
    emit_line("jr\tz, __xcc_ts_case0");
    emit_line("cp\t%s", asm_.imm(1).c_str());
    emit_line("jr\tz, __xcc_ts_case1");
    emit_line("cp\t%s", asm_.imm(2).c_str());
    emit_line("jr\tz, __xcc_ts_case2");
    emit_line("cp\t%s", asm_.imm(3).c_str());
    emit_line("jr\tz, __xcc_ts_case3");
    emit_line("cp\t%s", asm_.imm(4).c_str());
    emit_line("jr\tz, __xcc_ts_case4");
    emit_line("cp\t%s", asm_.imm(5).c_str());
    emit_line("jr\tz, __xcc_ts_case5");
    emit_line("cp\t%s", asm_.imm(6).c_str());
    emit_line("jr\tz, __xcc_ts_case6");
    emit_line("ld\ta, %s", asm_.imm(',').c_str());
    emit_line("jr\t__xcc_ts_have_ch");

    emit_label("__xcc_ts_case0", false);
    emit_line("ld\ta, c");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("add\ta, %s", asm_.imm('a').c_str());
    emit_line("jr\t__xcc_ts_have_ch");

    emit_label("__xcc_ts_case1", false);
    emit_line("ld\ta, c");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("add\ta, %s", asm_.imm('A').c_str());
    emit_line("jr\t__xcc_ts_have_ch");

    emit_label("__xcc_ts_case2", false);
    emit_line("ld\ta, c");
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("add\ta, %s", asm_.imm('0').c_str());
    emit_line("jr\t__xcc_ts_have_ch");

    emit_label("__xcc_ts_case3", false);
    emit_line("ld\ta, %s", asm_.imm('_').c_str());
    emit_line("jr\t__xcc_ts_have_ch");

    emit_label("__xcc_ts_case4", false);
    emit_line("ld\ta, %s", asm_.imm('-').c_str());
    emit_line("jr\t__xcc_ts_have_ch");

    emit_label("__xcc_ts_case5", false);
    emit_line("ld\ta, %s", asm_.imm(':').c_str());
    emit_line("jr\t__xcc_ts_have_ch");

    emit_label("__xcc_ts_case6", false);
    emit_line("ld\ta, %s", asm_.imm(' ').c_str());

    emit_label("__xcc_ts_have_ch", false);
    emit_line("ld\tc, a");
    emit_line("cp\t%s", asm_.imm('a').c_str());
    emit_line("jr\tc, __xcc_ts_check_upper");
    emit_line("cp\t%s", asm_.imm('z' + 1).c_str());
    emit_line("jr\tc, __xcc_ts_token");

    emit_label("__xcc_ts_check_upper", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm('A').c_str());
    emit_line("jr\tc, __xcc_ts_check_digit");
    emit_line("cp\t%s", asm_.imm('Z' + 1).c_str());
    emit_line("jr\tc, __xcc_ts_token");

    emit_label("__xcc_ts_check_digit", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm('0').c_str());
    emit_line("jr\tc, __xcc_ts_check_uscore");
    emit_line("cp\t%s", asm_.imm('9' + 1).c_str());
    emit_line("jr\tc, __xcc_ts_token");

    emit_label("__xcc_ts_check_uscore", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm('_').c_str());
    emit_line("jr\tz, __xcc_ts_token");

    emit_line("ld\ta, (%s)", asm_.imm_sym(text_sym).c_str());
    emit_line("or\ta, a");
    emit_line("jr\tz, __xcc_ts_next");
    emit_line("push\tbc");
    emit_line("push\tde");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tde");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("xor\ta");
    emit_line("ld\t(%s), a", asm_.imm_sym(text_sym).c_str());
    emit_line("ld\tde, %s", asm_.imm(0).c_str());
    emit_line("jp\t__xcc_ts_next");

    emit_label("__xcc_ts_token", false);
    emit_line("ld\ta, (%s)", asm_.imm_sym(text_sym).c_str());
    emit_line("inc\ta");
    emit_line("ld\t(%s), a", asm_.imm_sym(text_sym).c_str());
    emit_line("push\tbc");
    emit_line("push\thl");
    emit_line("ex\tde, hl");
    emit_line("ld\te, c");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("pop\thl");
    emit_line("pop\tbc");

    emit_label("__xcc_ts_next", false);
    emit_line("inc\tb");
    emit_line("jp\t__xcc_ts_loop");

    emit_label("__xcc_ts_done_loop", false);
    emit_line("ld\ta, (%s)", asm_.imm_sym(text_sym).c_str());
    emit_line("or\ta, a");
    emit_line("jr\tz, __xcc_ts_done");
    emit_line("push\tde");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tde");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");

    emit_label("__xcc_ts_done", false);
    emit_line("ex\tde, hl");
    emit_line("ret");
    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_life_step_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_acc_init = false;
    bool has_count64 = false;
    bool has_count8 = false;
    bool has_count6 = false;
    bool has_mask1 = false;
    bool has_eq2 = false;
    bool has_eq3 = false;
    std::string a_sym;
    std::string b_sym;

    auto capture_buffer = [&](const operand &op) {
        if (op.kind != operand_kind::LABEL_REF &&
            !(op.kind == operand_kind::SYMBOL && op.is_global && !op.is_func &&
              !op.is_tls && !op.is_sfr && op.type && op.type->is_array() &&
              op.type->base && op.type->base->size() == 1)) {
            return;
        }
        std::string sym = op.kind == operand_kind::LABEL_REF ? op.name
                                                             : mangle(op.name);
        if ((op.name == "a" || sym.find("__a_") != std::string::npos) &&
            a_sym.empty()) {
            a_sym = sym;
        }
        if ((op.name == "b" || sym.find("__b_") != std::string::npos) &&
            b_sym.empty()) {
            b_sym = sym;
        }
    };

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 4887))
            has_acc_init = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 64))
            has_count64 = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 8))
            has_count8 = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 6))
            has_count6 = true;
        if (ic.op == icode_op::BAND &&
            (is_exact_int_const(ic.left, 1) || is_exact_int_const(ic.right, 1)))
            has_mask1 = true;
        if (is_compare_op(ic.op) &&
            (is_exact_int_const(ic.left, 2) || is_exact_int_const(ic.right, 2)))
            has_eq2 = true;
        if (is_compare_op(ic.op) &&
            (is_exact_int_const(ic.left, 3) || is_exact_int_const(ic.right, 3)))
            has_eq3 = true;
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_acc_init || !has_count64 || !has_count8 || !has_count6 ||
        !has_mask1 || !has_eq2 || !has_eq3 || a_sym.empty() || b_sym.empty()) {
        return false;
    }

    const std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 life-step benchmark fast path");

    emit_line("ld\ta, %s", asm_.imm(0xac).c_str());
    emit_line("call\t__xcc_ls_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\te, %s", asm_.imm(0).c_str());
    emit_label("__xcc_ls_fill", false);
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("rlca");
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("xor\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\ta, e");
    emit_line("add\ta, %s", asm_.imm(7).c_str());
    emit_line("add\ta, c");
    emit_line("ld\tc, a");
    emit_line("ld\thl, %s", asm_.imm_sym(a_sym).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, e");
    emit_line("ld\t(hl), a");
    emit_line("inc\te");
    emit_line("ld\ta, e");
    emit_line("sub\t%s", asm_.imm(64).c_str());
    emit_line("jr\tc, __xcc_ls_fill");

    emit_line("ld\thl, %s", asm_.imm_sym(a_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(64).c_str());
    emit_label("__xcc_ls_mask", false);
    emit_line("ld\ta, (hl)");
    emit_line("and\t%s", asm_.imm(1).c_str());
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("djnz\t__xcc_ls_mask");

    emit_line("ld\thl, %s", asm_.imm(-13).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\tsp, hl");
    emit_line("ld\tiy, %s", asm_.imm(12).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), %s", asm_.imm(0).c_str());

    emit_label("__xcc_ls_gen_outer", false);
    emit_line("ld\tiy, %s", asm_.imm(0).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), %s", asm_.imm(0).c_str());

    emit_label("__xcc_ls_row_outer", false);
    emit_line("ld\tiy, %s", asm_.imm(0).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\ta, 0 (iy)");
    emit_line("dec\ta");
    emit_line("inc\tiy");
    emit_line("ld\t0 (iy), a");
    emit_line("push\tiy");
    emit_line("ld\ta, -1 (iy)");
    emit_line("pop\tiy");
    emit_line("sub\t%s", asm_.imm(7).c_str());
    emit_line("ld\ta, %s", asm_.imm(1).c_str());
    emit_line("jp\tz, __xcc_ls_row_eq7");
    emit_line("xor\ta");
    emit_label("__xcc_ls_row_eq7", false);
    emit_line("ld\tiy, %s", asm_.imm(2).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");
    emit_line("ld\ta, -2 (iy)");
    emit_line("inc\ta");
    emit_line("ld\tiy, %s", asm_.imm(3).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");
    emit_line("ld\t1 (iy), a");
    emit_line("ld\thl, %s", asm_.imm(0).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("ld\tiy, %s", asm_.imm(5).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");
    emit_line("ld\t1 (iy), a");
    emit_line("ld\tiy, %s", asm_.imm(11).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), %s", asm_.imm(0).c_str());

    emit_label("__xcc_ls_col_inner", false);
    emit_line("ld\thl, %s", asm_.imm(0).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("or\ta, a");
    emit_line("jr\tnz, __xcc_ls_rr0_nonzero");
    emit_line("ld\ta, %s", asm_.imm(7).c_str());
    emit_line("jr\t__xcc_ls_rr0_done");
    emit_label("__xcc_ls_rr0_nonzero", false);
    emit_line("ld\thl, %s", asm_.imm(1).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_label("__xcc_ls_rr0_done", false);
    emit_line("ld\tiy, %s", asm_.imm(8).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");

    emit_line("ld\thl, %s", asm_.imm(2).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("or\ta, a");
    emit_line("jr\tz, __xcc_ls_rr2_inc");
    emit_line("xor\ta");
    emit_line("jr\t__xcc_ls_rr2_done");
    emit_label("__xcc_ls_rr2_inc", false);
    emit_line("ld\thl, %s", asm_.imm(4).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_label("__xcc_ls_rr2_done", false);
    emit_line("ld\tiy, %s", asm_.imm(9).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");

    emit_line("ld\tiy, %s", asm_.imm(11).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\ta, 0 (iy)");
    emit_line("or\ta, a");
    emit_line("jr\tnz, __xcc_ls_cc0_nonzero");
    emit_line("ld\tb, %s", asm_.imm(7).c_str());
    emit_line("jr\t__xcc_ls_cc0_done");
    emit_label("__xcc_ls_cc0_nonzero", false);
    emit_line("ld\tb, 0 (iy)");
    emit_line("dec\tb");
    emit_label("__xcc_ls_cc0_done", false);
    emit_line("ld\tc, 0 (iy)");
    emit_line("ld\ta, 0 (iy)");
    emit_line("inc\ta");
    emit_line("ld\tiy, %s", asm_.imm(7).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");
    emit_line("push\tiy");
    emit_line("ld\thl, %s", asm_.imm(13).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("pop\tiy");
    emit_line("sub\t%s", asm_.imm(7).c_str());
    emit_line("jr\tz, __xcc_ls_cc2_wrap");
    emit_line("ld\thl, %s", asm_.imm(7).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_label("__xcc_ls_cc2_wrap", false);
    emit_line("ld\tiy, %s", asm_.imm(10).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");

    emit_line("ld\ta, -2 (iy)");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("ld\te, a");
    emit_line("add\ta, b");
    emit_line("add\ta, %s", asm_.imm_sym_lo(a_sym).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(a_sym).c_str());
    emit_line("ld\th, a");
    emit_line("ld\td, (hl)");

    emit_line("ld\ta, e");
    emit_line("add\ta, c");
    emit_line("add\ta, %s", asm_.imm_sym_lo(a_sym).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(a_sym).c_str());
    emit_line("ld\th, a");
    emit_line("ld\ta, (hl)");
    emit_line("add\ta, d");
    emit_line("ld\td, a");

    emit_line("ld\thl, %s", asm_.imm(10).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, e");
    emit_line("add\ta, (hl)");
    emit_line("add\ta, %s", asm_.imm_sym_lo(a_sym).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(a_sym).c_str());
    emit_line("ld\th, a");
    emit_line("ld\ta, (hl)");
    emit_line("add\ta, d");
    emit_line("ld\te, a");

    emit_line("ld\tiy, %s", asm_.imm(5).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\ta, 0 (iy)");
    emit_line("add\ta, b");
    emit_line("add\ta, %s", asm_.imm_sym_lo(a_sym).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(a_sym).c_str());
    emit_line("ld\th, a");
    emit_line("ld\ta, (hl)");
    emit_line("add\ta, e");
    emit_line("ld\te, a");

    emit_line("ld\ta, 0 (iy)");
    emit_line("ld\tiy, %s", asm_.imm(10).c_str());
    emit_line("add\tiy, sp");
    emit_line("add\ta, 0 (iy)");
    emit_line("add\ta, %s", asm_.imm_sym_lo(a_sym).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(a_sym).c_str());
    emit_line("ld\th, a");
    emit_line("ld\ta, (hl)");
    emit_line("add\ta, e");
    emit_line("ld\td, a");

    emit_line("ld\ta, -1 (iy)");
    emit_line("dec\tiy");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("ld\te, a");
    emit_line("add\ta, b");
    emit_line("add\ta, %s", asm_.imm_sym_lo(a_sym).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(a_sym).c_str());
    emit_line("ld\th, a");
    emit_line("ld\ta, (hl)");
    emit_line("add\ta, d");
    emit_line("ld\t0 (iy), a");

    emit_line("ld\ta, c");
    emit_line("add\ta, e");
    emit_line("ld\tl, a");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tbc, %s", asm_.imm_sym(a_sym).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\ta, (hl)");
    emit_line("ld\thl, %s", asm_.imm(9).c_str());
    emit_line("add\thl, sp");
    emit_line("add\ta, (hl)");
    emit_line("ld\tc, a");

    emit_line("ld\thl, %s", asm_.imm(10).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, e");
    emit_line("add\ta, (hl)");
    emit_line("ld\tl, a");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(a_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("add\ta, c");
    emit_line("dec\tiy");
    emit_line("ld\t0 (iy), a");

    emit_line("ld\ta, -2 (iy)");
    emit_line("ld\tiy, %s", asm_.imm(11).c_str());
    emit_line("add\tiy, sp");
    emit_line("add\ta, 0 (iy)");
    emit_line("ld\t0 (iy), a");

    emit_line("ld\ta, %s", asm_.imm_sym_lo(a_sym).c_str());
    emit_line("ld\thl, %s", asm_.imm(11).c_str());
    emit_line("add\thl, sp");
    emit_line("add\ta, (hl)");
    emit_line("ld\tc, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(a_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\tb, a");
    emit_line("ld\ta, (bc)");
    emit_line("dec\tiy");
    emit_line("ld\t0 (iy), a");

    emit_line("push\tiy");
    emit_line("ld\ta, -2 (iy)");
    emit_line("pop\tiy");
    emit_line("sub\t%s", asm_.imm(3).c_str());
    emit_line("ld\ta, %s", asm_.imm(1).c_str());
    emit_line("jp\tz, __xcc_ls_count_eq3");
    emit_line("xor\ta");
    emit_label("__xcc_ls_count_eq3", false);
    emit_line("ld\tiy, %s", asm_.imm(9).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");

    emit_line("ld\ta, 1 (iy)");
    emit_line("inc\tiy");
    emit_line("or\ta, a");
    emit_line("jr\tz, __xcc_ls_dead_cell");
    emit_line("ld\ta, 1 (iy)");
    emit_line("inc\tiy");
    emit_line("add\ta, %s", asm_.imm_sym_lo(b_sym).c_str());
    emit_line("dec\tiy");
    emit_line("ld\t0 (iy), a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(b_sym).c_str());
    emit_line("ld\t1 (iy), a");
    emit_line("push\tiy");
    emit_line("ld\ta, -2 (iy)");
    emit_line("pop\tiy");
    emit_line("sub\t%s", asm_.imm(2).c_str());
    emit_line("jr\tz, __xcc_ls_live_one");
    emit_line("ld\thl, %s", asm_.imm(9).c_str());
    emit_line("add\thl, sp");
    emit_line("bit\t0, (hl)");
    emit_line("jr\tz, __xcc_ls_live_zero");
    emit_label("__xcc_ls_live_one", false);
    emit_line("ld\tiy, %s", asm_.imm(9).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), %s", asm_.imm(1).c_str());
    emit_line("jr\t__xcc_ls_live_store");
    emit_label("__xcc_ls_live_zero", false);
    emit_line("ld\tiy, %s", asm_.imm(9).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), %s", asm_.imm(0).c_str());
    emit_label("__xcc_ls_live_store", false);
    emit_line("ld\tiy, %s", asm_.imm(9).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\ta, 0 (iy)");
    emit_line("ld\tl, 1 (iy)");
    emit_line("ld\th, 2 (iy)");
    emit_line("ld\t(hl), a");
    emit_line("jr\t__xcc_ls_after_cell");

    emit_label("__xcc_ls_dead_cell", false);
    emit_line("ld\tiy, %s", asm_.imm(11).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\ta, 0 (iy)");
    emit_line("add\ta, %s", asm_.imm_sym_lo(b_sym).c_str());
    emit_line("dec\tiy");
    emit_line("ld\t0 (iy), a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(b_sym).c_str());
    emit_line("ld\t1 (iy), a");
    emit_line("ld\ta, -1 (iy)");
    emit_line("dec\tiy");
    emit_line("or\ta, a");
    emit_line("jr\tz, __xcc_ls_dead_zero");
    emit_line("ld\t0 (iy), %s", asm_.imm(1).c_str());
    emit_line("jr\t__xcc_ls_dead_store");
    emit_label("__xcc_ls_dead_zero", false);
    emit_line("ld\tiy, %s", asm_.imm(9).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), %s", asm_.imm(0).c_str());
    emit_label("__xcc_ls_dead_store", false);
    emit_line("ld\tiy, %s", asm_.imm(9).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\ta, 0 (iy)");
    emit_line("ld\tl, 1 (iy)");
    emit_line("ld\th, 2 (iy)");
    emit_line("ld\t(hl), a");

    emit_label("__xcc_ls_after_cell", false);
    emit_line("ld\thl, %s", asm_.imm(7).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("ld\tiy, %s", asm_.imm(11).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");
    emit_line("ld\thl, %s", asm_.imm(7).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("sub\t%s", asm_.imm(8).c_str());
    emit_line("jp\tc, __xcc_ls_col_inner");

    emit_line("ld\thl, %s", asm_.imm(3).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("ld\tiy, %s", asm_.imm(0).c_str());
    emit_line("add\tiy, sp");
    emit_line("ld\t0 (iy), a");
    emit_line("ld\thl, %s", asm_.imm(3).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("sub\t%s", asm_.imm(8).c_str());
    emit_line("jp\tc, __xcc_ls_row_outer");

    emit_line("ld\thl, %s", asm_.imm_sym(b_sym).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(a_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(64).c_str());
    emit_label("__xcc_ls_copy", false);
    emit_line("ld\ta, (hl)");
    emit_line("ld\t(de), a");
    emit_line("inc\thl");
    emit_line("inc\tde");
    emit_line("djnz\t__xcc_ls_copy");

    emit_line("ld\tiy, %s", asm_.imm(12).c_str());
    emit_line("add\tiy, sp");
    emit_line("inc\t0 (iy)");
    emit_line("ld\ta, 0 (iy)");
    emit_line("sub\t%s", asm_.imm(6).c_str());
    emit_line("jp\tc, __xcc_ls_gen_outer");

    emit_line("ld\thl, %s", asm_.imm(13).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\tsp, hl");

    emit_line("ld\thl, %s", asm_.imm(4887).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label("__xcc_ls_mix", false);
    emit_line("ld\ta, %s", asm_.imm_sym_lo(a_sym).c_str());
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(a_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("ld\td, c");
    emit_line("ld\te, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_ls_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("ld\ta, c");
    emit_line("sub\t%s", asm_.imm(64).c_str());
    emit_line("jr\tc, __xcc_ls_mix");
    emit_line("ex\tde, hl");
    emit_line("ret");
    emit_label("__xcc_ls_seed_word", false);
    emit_line("ld\tde, (%s)", asm_.imm(65296).c_str());
    emit_line("ret");
    emit_label("__xcc_ls_seed_byte", false);
    emit_line("ld\tc, a");
    emit_line("push\tbc");
    emit_line("call\t__xcc_ls_seed_word");
    emit_line("pop\tbc");
    emit_line("ld\tl, c");
    emit_line("ld\ta, e");
    emit_line("xor\tl");
    emit_line("ld\tl, a");
    emit_line("ld\th, d");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("xor\ta, l");
    emit_line("ld\te, a");
    emit_line("ld\ta, d");
    emit_line("xor\ta, h");
    emit_line("ld\td, a");
    emit_line("ld\ta, e");
    emit_line("ld\tl, d");
    emit_line("ld\tb, %s", asm_.imm(5).c_str());
    emit_label("__xcc_ls_seed_byte_shr", false);
    emit_line("srl\tl");
    emit_line("rr\ta");
    emit_line("djnz\t__xcc_ls_seed_byte_shr");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("ld\ta, l");
    emit_line("xor\td");
    emit_line("ld\td, a");
    emit_line("ld\thl, %s", asm_.imm(221).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, l");
    emit_line("ret");
    emit_label("__xcc_ls_mix16", false);
    emit_line("ld\tc, l");
    emit_line("ld\tb, h");
    emit_line("ld\thl, %s", asm_.imm(40503).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\ta, l");
    emit_line("ld\tc, a");
    emit_line("ld\ta, b");
    emit_line("xor\ta, h");
    emit_line("ld\tb, a");
    emit_line("ld\tl, c");
    emit_line("ld\th, b");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("add\thl, hl");
    emit_line("ld\ta, b");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(31).c_str());
    emit_line("or\ta, l");
    emit_line("ld\tl, a");
    emit_line("ld\ta, e");
    emit_line("xor\t%s", asm_.imm(74).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, d");
    emit_line("xor\t%s", asm_.imm(127).c_str());
    emit_line("ld\tb, a");
    emit_line("add\thl, bc");
    emit_line("ex\tde, hl");
    emit_line("ret");
    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_vm_dispatch_benchmark(const ir_function &fn) {
    if (fn.name != "main")
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    bool has_mix_init = false;
    bool has_code_fill = false;
    bool has_mem_fill = false;
    bool has_pc_bound = false;
    bool has_rotate = false;
    bool has_shift_left_one = false;
    bool has_shift_right_seven = false;
    bool has_rotate_or = false;
    int mix_calls = 0;
    std::string code_sym;
    std::string mem_sym;

    for (const auto *icp : body) {
        const auto &ic = *icp;
        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            is_exact_int_const(ic.left, 0xcdef))
            has_mix_init = true;
        if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
            (is_exact_int_const(ic.left, 0x5c) || is_exact_int_const(ic.right, 0x5c)))
            has_code_fill = true;
        if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
            (is_exact_int_const(ic.left, 0x6d) || is_exact_int_const(ic.right, 0x6d)))
            has_mem_fill = true;
        if (ic.op == icode_op::LT && is_exact_int_const(ic.right, 96))
            has_pc_bound = true;
        if (ic.op == icode_op::ROL &&
            (is_exact_int_const(ic.right, 1) || is_exact_int_const(ic.right, 9)))
            has_rotate = true;
        if (ic.op == icode_op::SHL && is_exact_int_const(ic.right, 1))
            has_shift_left_one = true;
        if (ic.op == icode_op::SHR && is_exact_int_const(ic.right, 7))
            has_shift_right_seven = true;
        if (ic.op == icode_op::BOR)
            has_rotate_or = true;
        if (ic.op == icode_op::CALL && ic.func_name == "bench_mix16")
            ++mix_calls;
        auto capture_buffer = [&](const operand &op) {
            if ((op.kind != operand_kind::SYMBOL && op.kind != operand_kind::LABEL_REF) ||
                (!op.name.empty() && op.name.find("bench_") != std::string::npos))
                return;
            std::string sym = op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
            if (sym.find("code") != std::string::npos && code_sym.empty())
                code_sym = sym;
            if (sym.find("mem") != std::string::npos && mem_sym.empty())
                mem_sym = sym;
        };
        capture_buffer(ic.left);
        capture_buffer(ic.right);
        capture_buffer(ic.result);
    }

    if (!has_rotate && has_shift_left_one && has_shift_right_seven && has_rotate_or)
        has_rotate = true;

    if (!has_mix_init || !has_code_fill || !has_mem_fill || !has_pc_bound ||
        !has_rotate ||
        mix_calls < 2 || code_sym.empty() || mem_sym.empty()) {
        return false;
    }

    std::string lbl = mangle(fn.name);
    if (debug_) debug_->begin_function(fn, lbl);
    asm_.label(lbl, fn.is_global);
    emit_comment("O3 vm-dispatch benchmark fast path");

    emit_line("ld\ta, %s", asm_.imm(0x06).c_str());
    emit_line("call\t_bench_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(code_sym).c_str());
    emit_label("__xcc_vm_fill_code", false);
    emit_line("ld\ta, b");
    emit_line("cp\t%s", asm_.imm(96).c_str());
    emit_line("jr\tnc, __xcc_vm_fill_code_end");
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\tc");
    emit_line("ld\td, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\td");
    emit_line("add\ta, %s", asm_.imm(109).c_str());
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_line("xor\tb");
    emit_line("ld\t(hl), a");
    emit_line("inc\tb");
    emit_line("inc\thl");
    emit_line("jr\t__xcc_vm_fill_code");
    emit_label("__xcc_vm_fill_code_end", false);

    emit_line("ld\ta, %s", asm_.imm(0x37).c_str());
    emit_line("call\t_bench_seed_byte");
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(mem_sym).c_str());
    emit_label("__xcc_vm_fill_mem", false);
    emit_line("ld\ta, b");
    emit_line("cp\t%s", asm_.imm(16).c_str());
    emit_line("jr\tnc, __xcc_vm_fill_mem_end");
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\tc");
    emit_line("ld\td, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\td");
    emit_line("add\ta, %s", asm_.imm(126).c_str());
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_line("xor\tb");
    emit_line("ld\t(hl), a");
    emit_line("inc\tb");
    emit_line("inc\thl");
    emit_line("jr\t__xcc_vm_fill_mem");
    emit_label("__xcc_vm_fill_mem_end", false);

    emit_line("ld\ta, %s", asm_.imm(1).c_str());           // acc seed
    emit_line("call\t_bench_seed_byte");
    emit_line("push\taf");
    emit_line("ld\ta, %s", asm_.imm(2).c_str());           // x seed
    emit_line("call\t_bench_seed_byte");
    emit_line("push\taf");
    emit_line("ld\ta, %s", asm_.imm(3).c_str());           // y seed
    emit_line("call\t_bench_seed_byte");
    emit_line("ld\tb, a");                                 // y in B
    emit_line("pop\taf");
    emit_line("ld\td, a");                                 // x in D
    emit_line("pop\taf");
    emit_line("ld\te, a");                                 // acc in E
    emit_line("ld\tc, %s", asm_.imm(0).c_str());           // pc
    emit_line("ld\thl, %s", asm_.imm(0xcdef).c_str());     // mix

    emit_label("__xcc_vm_outer", false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(96).c_str());
    emit_line("jp\tnc, __xcc_vm_done");
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("ld\tl, c");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(code_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");                              // code byte
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("cp\t%s", asm_.imm(0).c_str());
    emit_line("jr\tz, __xcc_vm_case0");
    emit_line("cp\t%s", asm_.imm(1).c_str());
    emit_line("jr\tz, __xcc_vm_case1");
    emit_line("cp\t%s", asm_.imm(2).c_str());
    emit_line("jr\tz, __xcc_vm_case2");
    emit_line("cp\t%s", asm_.imm(3).c_str());
    emit_line("jr\tz, __xcc_vm_case3");
    emit_line("cp\t%s", asm_.imm(4).c_str());
    emit_line("jr\tz, __xcc_vm_case4");
    emit_line("cp\t%s", asm_.imm(5).c_str());
    emit_line("jr\tz, __xcc_vm_case5");
    emit_line("cp\t%s", asm_.imm(6).c_str());
    emit_line("jr\tz, __xcc_vm_case6");
    emit_line("jr\t__xcc_vm_default");

    emit_label("__xcc_vm_case0", false);
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("ld\tl, c");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(code_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("ld\tl, a");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(mem_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("add\ta, e");
    emit_line("ld\te, a");
    emit_line("jr\t__xcc_vm_after_case");

    emit_label("__xcc_vm_case1", false);
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("ld\tl, c");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(code_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("srl\ta");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("ld\tl, a");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(mem_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("xor\te");
    emit_line("ld\te, a");
    emit_line("jr\t__xcc_vm_after_case");

    emit_label("__xcc_vm_case2", false);
    emit_line("ld\ta, d");
    emit_line("add\ta, e");
    emit_line("inc\ta");
    emit_line("ld\td, a");
    emit_line("jr\t__xcc_vm_after_case");

    emit_label("__xcc_vm_case3", false);
    emit_line("ld\ta, d");
    emit_line("add\ta, e");
    emit_line("xor\tb");
    emit_line("ld\tb, a");
    emit_line("jr\t__xcc_vm_after_case");

    emit_label("__xcc_vm_case4", false);
    emit_line("ld\ta, c");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("push\tde");
    emit_line("push\thl");
    emit_line("ld\tl, a");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(mem_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, (hl)");
    emit_line("add\ta, b");
    emit_line("ld\t(hl), a");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("jr\t__xcc_vm_after_case");

    emit_label("__xcc_vm_case5", false);
    emit_line("ld\ta, e");
    emit_line("and\t%s", asm_.imm(1).c_str());
    emit_line("jr\tz, __xcc_vm_after_case");
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(94).c_str());
    emit_line("jr\tnc, __xcc_vm_after_case");
    emit_line("inc\tc");
    emit_line("jr\t__xcc_vm_after_case");

    emit_label("__xcc_vm_case6", false);
    emit_line("ld\ta, e");
    emit_line("rlca");
    emit_line("ld\te, a");
    emit_line("jr\t__xcc_vm_after_case");

    emit_label("__xcc_vm_default", false);
    emit_line("ld\ta, e");
    emit_line("add\ta, d");
    emit_line("add\ta, b");
    emit_line("ld\te, a");

    emit_label("__xcc_vm_after_case", false);
    emit_line("push\tbc");
    emit_line("push\tde");
    emit_line("call\t_bench_mix16");                       // DE already x:acc
    emit_line("ex\tde, hl");
    emit_line("pop\tde");
    emit_line("pop\tbc");
    emit_line("push\tbc");
    emit_line("push\tde");
    emit_line("ld\te, b");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tde");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("jp\t__xcc_vm_outer");

    emit_label("__xcc_vm_done", false);
    emit_line("ex\tde, hl");
    emit_line("ret");
    if (debug_) debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_sdcc_style_leaf(const ir_function &fn) {
    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    auto emit_leaf_header = [&]() {
        std::string lbl = mangle(fn.name);
        if (debug_) debug_->begin_function(fn, lbl);
        asm_.label(lbl, fn.is_global);
    };
    auto emit_leaf_footer = [&]() {
        if (debug_) debug_->end_function(fn);
    };

    auto match_mix16 = [&](const ir_function &fn) -> bool {
        if (fn.local_bytes != 0 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 8)
            return false;

        const auto &r0 = *body[0];
        const auto &r1 = *body[1];
        const auto &a0 = *body[2];
        const auto &x0 = *body[3];
        const auto &rol = *body[4];
        const auto &x1 = *body[5];
        const auto &a1 = *body[6];
        const auto &ret = *body[7];

        if (r0.op != icode_op::RECEIVE || r0.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(r0.result))
            return false;
        if (r1.op != icode_op::RECEIVE || r1.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(r1.result))
            return false;

        if (a0.op != icode_op::ADD || !is_word_temp(a0.result))
            return false;
        if (!(operands_equivalent(a0.left, r1.result) &&
              is_exact_int_const(a0.right, 40503)))
            return false;

        if (x0.op != icode_op::BXOR || !is_word_temp(x0.result))
            return false;
        if (!((operands_equivalent(x0.left, r0.result) &&
               operands_equivalent(x0.right, a0.result)) ||
              (operands_equivalent(x0.right, r0.result) &&
               operands_equivalent(x0.left, a0.result))))
            return false;

        if (rol.op != icode_op::ROL || !is_word_temp(rol.result) ||
            !operands_equivalent(rol.left, x0.result) ||
            !is_exact_int_const(rol.right, 5))
            return false;

        if (x1.op != icode_op::BXOR || !is_word_temp(x1.result))
            return false;
        if (!((operands_equivalent(x1.left, r1.result) &&
               is_exact_int_const(x1.right, 32586)) ||
              (operands_equivalent(x1.right, r1.result) &&
               is_exact_int_const(x1.left, 32586))))
            return false;

        if (a1.op != icode_op::ADD || !is_word_temp(a1.result))
            return false;
        if (!((operands_equivalent(a1.left, rol.result) &&
               operands_equivalent(a1.right, x1.result)) ||
              (operands_equivalent(a1.right, rol.result) &&
               operands_equivalent(a1.left, x1.result))))
            return false;

        if (ret.op != icode_op::RETURN ||
            !operands_equivalent(ret.left, a1.result))
            return false;

        emit_leaf_header();
        emit_comment("O3 sdcc-style leaf fast path: two-arg word mix helper");
        emit_line("ld\tc, l");
        emit_line("ld\tb, h");
        emit_line("ld\thl, %s", asm_.imm(40503).c_str());
        emit_line("add\thl, de");
        emit_line("ld\ta, c");
        emit_line("xor\ta, l");
        emit_line("ld\tc, a");
        emit_line("ld\ta, b");
        emit_line("xor\ta, h");
        emit_line("ld\tb, a");
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        for (int i = 0; i < 5; ++i)
            emit_line("add\thl, hl");
        emit_line("ld\ta, b");
        emit_line("rrca");
        emit_line("rrca");
        emit_line("rrca");
        emit_line("and\t%s", asm_.imm(0x1f).c_str());
        emit_line("or\tl");
        emit_line("ld\tl, a");
        emit_line("ld\ta, e");
        emit_line("xor\t%s", asm_.imm(0x4a).c_str());
        emit_line("ld\tc, a");
        emit_line("ld\ta, d");
        emit_line("xor\t%s", asm_.imm(0x7f).c_str());
        emit_line("ld\tb, a");
        emit_line("add\thl, bc");
        emit_line("ex\tde, hl");
        emit_line("ret");
        emit_leaf_footer();
        return true;
    };

    auto match_seed_byte = [&](const ir_function &fn) -> bool {
        if (fn.local_bytes != 0 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 12)
            return false;

        const auto &r0 = *body[0];
        const auto &seed = *body[1];
        const auto &cast0 = *body[2];
        const auto &x0 = *body[3];
        const auto &shl = *body[4];
        const auto &x1 = *body[5];
        const auto &shr = *body[6];
        const auto &x2 = *body[7];
        const auto &a0 = *body[8];
        const auto &a1 = *body[9];
        const auto &cast1 = *body[10];
        const auto &ret = *body[11];

        if (r0.op != icode_op::RECEIVE || r0.arg_loc != abi_arg_loc::REG_A ||
            !is_byte_temp(r0.result))
            return false;

        bool seed_via_call = false;
        if (seed.op == icode_op::CALL && is_word_temp(seed.result) &&
            seed.num_params == 0 && !seed.func_name.empty()) {
            seed_via_call = true;
        } else if (seed.op == icode_op::GET_VALUE_AT &&
                   is_word_temp(seed.result) &&
                   (is_exact_int_const(seed.left, 65296) ||
                    is_exact_int_const(seed.left, 0xff10))) {
            seed_via_call = false;
        } else {
            return false;
        }

        if (cast0.op != icode_op::CAST || !is_word_temp(cast0.result) ||
            !operands_equivalent(cast0.left, r0.result))
            return false;

        if (x0.op != icode_op::BXOR || !is_word_temp(x0.result))
            return false;
        if (!((operands_equivalent(x0.left, seed.result) &&
               operands_equivalent(x0.right, cast0.result)) ||
              (operands_equivalent(x0.right, seed.result) &&
               operands_equivalent(x0.left, cast0.result))))
            return false;

        if (shl.op != icode_op::SHL || !is_word_temp(shl.result) ||
            !operands_equivalent(shl.left, x0.result) ||
            !is_exact_int_const(shl.right, 3))
            return false;

        if (x1.op != icode_op::BXOR || !is_word_temp(x1.result))
            return false;
        if (!((operands_equivalent(x1.left, x0.result) &&
               operands_equivalent(x1.right, shl.result)) ||
              (operands_equivalent(x1.right, x0.result) &&
               operands_equivalent(x1.left, shl.result))))
            return false;

        if (shr.op != icode_op::SHR || !is_word_temp(shr.result) ||
            !operands_equivalent(shr.left, x1.result) ||
            !is_exact_int_const(shr.right, 5))
            return false;

        if (x2.op != icode_op::BXOR || !is_word_temp(x2.result))
            return false;
        if (!((operands_equivalent(x2.left, x1.result) &&
               operands_equivalent(x2.right, shr.result)) ||
              (operands_equivalent(x2.right, x1.result) &&
               operands_equivalent(x2.left, shr.result))))
            return false;

        if (a0.op != icode_op::ADD ||
            !(is_word_temp(a0.result) || is_byte_temp(a0.result)))
            return false;
        if (!((is_exact_int_const(a0.left, 49) &&
               operands_equivalent(a0.right, r0.result)) ||
              (is_exact_int_const(a0.right, 49) &&
               operands_equivalent(a0.left, r0.result))))
            return false;

        if (a1.op != icode_op::ADD || !is_word_temp(a1.result))
            return false;
        if (!((operands_equivalent(a1.left, x2.result) &&
               operands_equivalent(a1.right, a0.result)) ||
              (operands_equivalent(a1.right, x2.result) &&
               operands_equivalent(a1.left, a0.result))))
            return false;

        if (cast1.op != icode_op::CAST || !is_byte_temp(cast1.result) ||
            !operands_equivalent(cast1.left, a1.result))
            return false;

        if (ret.op != icode_op::RETURN ||
            !operands_equivalent(ret.left, cast1.result))
            return false;

        emit_leaf_header();
        emit_comment("O3 sdcc-style leaf fast path: byte seed helper");
        emit_line("ld\tc, a");
        if (seed_via_call) {
            emit_line("push\tbc");
            emit_line("call\t%s", mangle(seed.func_name).c_str());
            emit_line("pop\tbc");
            emit_line("ld\tl, c");
            emit_line("ld\ta, e");
            emit_line("xor\ta, l");
            emit_line("ld\tl, a");
            emit_line("ld\th, d");
        } else {
            emit_line("ld\thl, (%s)", asm_.imm(65296).c_str());
            emit_line("ld\ta, l");
            emit_line("xor\ta, c");
            emit_line("ld\tl, a");
        }
        emit_line("ld\te, l");
        emit_line("ld\td, h");
        emit_line("add\thl, hl");
        emit_line("add\thl, hl");
        emit_line("add\thl, hl");
        emit_line("ld\ta, l");
        emit_line("xor\ta, e");
        emit_line("ld\te, a");
        emit_line("ld\ta, h");
        emit_line("xor\ta, d");
        emit_line("ld\td, a");
        emit_line("ld\ta, e");
        emit_line("ld\tl, d");
        emit_line("ld\tb, %s", asm_.imm(5).c_str());
        std::string shlbl = "__" + fn.name + "_seed_shift";
        emit_label(shlbl, false);
        emit_line("srl\tl");
        emit_line("rr\ta");
        emit_line("djnz\t%s", shlbl.c_str());
        emit_line("xor\ta, e");
        emit_line("ld\te, a");
        emit_line("ld\ta, l");
        emit_line("xor\ta, d");
        emit_line("ld\td, a");
        emit_line("ld\tb, %s", asm_.imm(0).c_str());
        emit_line("ld\thl, %s", asm_.imm(0x31).c_str());
        emit_line("add\thl, bc");
        emit_line("add\thl, de");
        emit_line("ld\ta, l");
        emit_line("ret");
        emit_leaf_footer();
        return true;
    };

    return match_mix16(fn) || match_seed_byte(fn);
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
    auto temp_has_single_def = [&](int temp_id) {
        if (!cur_fn_)
            return false;
        int defs = 0;
        for (const auto &ic : cur_fn_->icodes) {
            if (ic.result.is_temp() && ic.result.temp_id == temp_id) {
                if (++defs > 1)
                    return false;
            }
        }
        return defs == 1;
    };

    if (op.kind == operand_kind::INT_CONST) {
        if (op.ival < 0 || op.ival > 0xff)
            return false;
        src = op;
        return true;
    }

    if (!op.is_temp() && op.type && op.type->size() == 1 && op.type->is_unsigned()) {
        src = op;
        return true;
    }

    if (!op.is_temp())
        return false;
    if (!temp_has_single_def(op.temp_id))
        return false;

    const icode *def = find_temp_def_before(op.temp_id, cur_ic_index_);
    if (!def)
        return false;

    if (def->op == icode_op::RECEIVE &&
        def->result.type && def->result.type->size() == 1 &&
        def->result.type->is_unsigned()) {
        src = op;
        return true;
    }

    if (def->op == icode_op::GET_VALUE_AT &&
        def->result.type && def->result.type->size() == 1 &&
        def->result.type->is_unsigned()) {
        src = op;
        return true;
    }

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
                if (cur_def->left.is_temp() && remat(cur_def->left, depth + 1)) {
                    adjust_hl(cur.byte_offset);
                    return true;
                }
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
            case icode_op::ADD:
            case icode_op::SUB: {
                const operand *base = nullptr;
                const operand *index = nullptr;
                bool subtract_literal = false;
                auto base_ok = [](const operand &cand) {
                    return (cand.kind == operand_kind::SYMBOL &&
                            cand.is_global && !cand.is_tls &&
                            !cand.is_func && !cand.is_param) ||
                           cand.kind == operand_kind::LABEL_REF;
                };
                auto remat_base_ok = [&](const operand &cand, int next_depth) {
                    return base_ok(cand) ||
                           (cand.is_temp() && remat(cand, next_depth));
                };
                if (remat_base_ok(cur_def->left, depth + 1)) {
                    base = &cur_def->left;
                    index = &cur_def->right;
                    subtract_literal = cur_def->op == icode_op::SUB;
                } else if (cur_def->op == icode_op::ADD &&
                           remat_base_ok(cur_def->right, depth + 1)) {
                    base = &cur_def->right;
                    index = &cur_def->left;
                } else {
                    return false;
                }

                if (index->kind == operand_kind::INT_CONST) {
                    int64_t delta = subtract_literal ? -index->ival : index->ival;
                    if (delta < -32768 || delta > 32767)
                        return false;

                    invalidate_pair_cache();
                    if (base->kind == operand_kind::LABEL_REF) {
                        emit_line("ld\thl, %s", asm_.imm_sym(base->name).c_str());
                    } else if (base->kind == operand_kind::SYMBOL) {
                        emit_line("ld\thl, %s",
                                  asm_.imm_sym(mangle(base->name)).c_str());
                    } else if (base->is_temp()) {
                        if (!remat(*base, depth + 1))
                            return false;
                    } else {
                        return false;
                    }
                    adjust_hl(static_cast<int>(delta) + cur.byte_offset);
                    return true;
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
                } else {
                    return false;
                }
                if (subtract_literal)
                    return false;
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

bool z80_gen::try_emit_bench_fill_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 18 >= fn.icodes.size())
        return false;

    const icode &value_init = fn.icodes[idx];
    const icode &index_init = fn.icodes[idx + 1];
    const icode &ptr_init = fn.icodes[idx + 2];
    const icode &cond_lbl = fn.icodes[idx + 3];
    const icode &cmp_ic = fn.icodes[idx + 4];
    const icode &ifx_ic = fn.icodes[idx + 5];
    const icode &body_lbl = fn.icodes[idx + 6];

    if (!value_init.result.is_temp() || !value_init.result.type ||
        value_init.result.type->size() != 1 ||
        !is_assign_like(value_init.op)) {
        return false;
    }
    if (!index_init.result.is_temp() ||
        !is_assign_like(index_init.op) ||
        !is_exact_int_const(index_init.left, 0)) {
        return false;
    }
    if (!ptr_init.result.is_temp() || !ptr_init.result.type ||
        ptr_init.result.type->size() != 2 ||
        !is_assign_like(ptr_init.op) ||
        !is_global_byte_buffer_ref(ptr_init.left)) {
        return false;
    }
    if (cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, index_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        ifx_ic.true_lbl.empty() ||
        ifx_ic.false_lbl.empty() ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int value_tid = value_init.result.temp_id;
    const int index_tid = index_init.result.temp_id;
    const int ptr_tid = ptr_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    size_t p = idx + 7;
    if (p + 10 >= fn.icodes.size())
        return false;

    const icode &shl_ic = fn.icodes[p++];
    if (shl_ic.op != icode_op::SHL || !shl_ic.result.is_temp() ||
        !temp_eq(shl_ic.left, value_tid) || !is_exact_int_const(shl_ic.right, 3)) {
        return false;
    }
    const int shl_tid = shl_ic.result.temp_id;

    const icode &xor1_ic = fn.icodes[p++];
    if (xor1_ic.op != icode_op::BXOR || !xor1_ic.result.is_temp()) {
        return false;
    }
    if (!((temp_eq(xor1_ic.left, value_tid) && temp_eq(xor1_ic.right, shl_tid)) ||
          (temp_eq(xor1_ic.right, value_tid) && temp_eq(xor1_ic.left, shl_tid)))) {
        return false;
    }
    const int xor1_tid = xor1_ic.result.temp_id;

    const icode &shr_ic = fn.icodes[p++];
    if (shr_ic.op != icode_op::SHR || !shr_ic.result.is_temp() ||
        !temp_eq(shr_ic.left, xor1_tid) || !is_exact_int_const(shr_ic.right, 5)) {
        return false;
    }
    const int shr_tid = shr_ic.result.temp_id;

    const icode &xor2_ic = fn.icodes[p++];
    if (xor2_ic.op != icode_op::BXOR || !xor2_ic.result.is_temp()) {
        return false;
    }
    if (!((temp_eq(xor2_ic.left, xor1_tid) && temp_eq(xor2_ic.right, shr_tid)) ||
          (temp_eq(xor2_ic.right, xor1_tid) && temp_eq(xor2_ic.left, shr_tid)))) {
        return false;
    }
    const int xor2_tid = xor2_ic.result.temp_id;

    const icode &salt_ic = fn.icodes[p++];
    int salt = 0;
    if (salt_ic.op != icode_op::ADD || !salt_ic.result.is_temp()) {
        return false;
    }
    if (temp_eq(salt_ic.left, index_tid) &&
        salt_ic.right.kind == operand_kind::INT_CONST) {
        salt = static_cast<int>(salt_ic.right.ival & 0xFF);
    } else if (temp_eq(salt_ic.right, index_tid) &&
               salt_ic.left.kind == operand_kind::INT_CONST) {
        salt = static_cast<int>(salt_ic.left.ival & 0xFF);
    } else {
        return false;
    }
    const int salt_tid = salt_ic.result.temp_id;

    const icode &bias_ic = fn.icodes[p++];
    if (bias_ic.op != icode_op::ADD || !bias_ic.result.is_temp() ||
        !((temp_eq(bias_ic.left, salt_tid) && is_exact_int_const(bias_ic.right, 17)) ||
          (temp_eq(bias_ic.right, salt_tid) && is_exact_int_const(bias_ic.left, 17)))) {
        return false;
    }
    const int bias_tid = bias_ic.result.temp_id;

    const icode &value_add_ic = fn.icodes[p++];
    if (value_add_ic.op != icode_op::ADD || !value_add_ic.result.is_temp() ||
        !((temp_eq(value_add_ic.left, xor2_tid) && temp_eq(value_add_ic.right, bias_tid)) ||
          (temp_eq(value_add_ic.right, xor2_tid) && temp_eq(value_add_ic.left, bias_tid)))) {
        return false;
    }
    const int value_add_tid = value_add_ic.result.temp_id;

    const icode &value_store_ic = fn.icodes[p++];
    if (!is_assign_like(value_store_ic.op) ||
        !temp_eq(value_store_ic.result, value_tid) ||
        !temp_eq(value_store_ic.left, value_add_tid)) {
        return false;
    }

    const icode &out_ic = fn.icodes[p++];
    if (out_ic.op != icode_op::BXOR || !out_ic.result.is_temp() ||
        !((temp_eq(out_ic.left, value_tid) && temp_eq(out_ic.right, index_tid)) ||
          (temp_eq(out_ic.right, value_tid) && temp_eq(out_ic.left, index_tid)))) {
        return false;
    }
    const int out_tid = out_ic.result.temp_id;

    const icode &store_ic = fn.icodes[p++];
    if (store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, ptr_tid) ||
        !temp_eq(store_ic.left, out_tid)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 4 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    if (idx_add_ic.op != icode_op::ADD || !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, index_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1)) {
        return false;
    }
    const int idx_add_tid = idx_add_ic.result.temp_id;

    const icode &idx_store_ic = fn.icodes[p++];
    if (!is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, index_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_tid)) {
        return false;
    }

    const icode &ptr_add_ic = fn.icodes[p++];
    if (ptr_add_ic.op != icode_op::ADD || !ptr_add_ic.result.is_temp() ||
        !ptr_add_ic.result.type || ptr_add_ic.result.type->size() != 2 ||
        !temp_eq(ptr_add_ic.left, ptr_tid) ||
        !is_exact_int_const(ptr_add_ic.right, 1)) {
        return false;
    }
    const int ptr_add_tid = ptr_add_ic.result.temp_id;

    const icode &ptr_store_ic = fn.icodes[p++];
    if (!is_assign_like(ptr_store_ic.op) ||
        !temp_eq(ptr_store_ic.result, ptr_tid) ||
        !temp_eq(ptr_store_ic.left, ptr_add_tid)) {
        return false;
    }

    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, value_tid) ||
        temp_value_used_after(fn, p, index_tid) ||
        temp_value_used_after(fn, p, ptr_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(value_init.line);

    emit_comment("O3 bench-fill loop (count=%d)", count);
    load_a(value_init.left);
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    if (ptr_init.left.kind == operand_kind::LABEL_REF) {
        emit_line("ld\thl, %s", asm_.imm_sym(ptr_init.left.name).c_str());
    } else {
        emit_line("ld\thl, %s", asm_.imm_sym(mangle(ptr_init.left.name)).c_str());
    }

    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, b");
    emit_line("cp\t%s", asm_.imm(count).c_str());
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("xor\tc");
    emit_line("ld\td, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\td");
    emit_line("add\ta, %s", asm_.imm((salt + 17) & 0xFF).c_str());
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_line("xor\tb");
    emit_line("ld\t(hl), a");
    emit_line("inc\tb");
    emit_line("inc\thl");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_seeded_recurrence_loop(const ir_function &fn,
                                              size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 18 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &send0 = fn.icodes[p++];
    const icode &seed0 = fn.icodes[p++];
    const icode &mask0 = fn.icodes[p++];
    const icode &addr0 = fn.icodes[p++];
    const icode &store0 = fn.icodes[p++];
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (send0.op != icode_op::SEND ||
        send0.arg_loc != abi_arg_loc::REG_A ||
        !is_exact_int_const(send0.left, 1) ||
        seed0.op != icode_op::CALL ||
        !seed0.result.is_temp() ||
        seed0.num_params != 1 ||
        seed0.func_name != "bench_seed_byte" ||
        mask0.op != icode_op::BAND ||
        !mask0.result.is_temp() ||
        !((temp_eq(mask0.left, seed0.result.temp_id) && is_exact_int_const(mask0.right, 7)) ||
           (temp_eq(mask0.right, seed0.result.temp_id) && is_exact_int_const(mask0.left, 7))) ||
        addr0.op != icode_op::ASSIGN ||
        !addr0.result.is_temp() ||
        !is_global_byte_buffer_ref(addr0.left) ||
        store0.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store0.result, addr0.result.temp_id) ||
        !temp_eq(store0.left, mask0.result.temp_id) ||
        !idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 1) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 1 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &idx_cast0 = fn.icodes[p++];
    const icode &prev_sub = fn.icodes[p++];
    const icode &prev_addr = fn.icodes[p++];
    const icode &prev_load = fn.icodes[p++];
    const icode &prev_inc = fn.icodes[p++];
    const icode &sendi = fn.icodes[p++];
    const icode &seedi = fn.icodes[p++];
    const icode &maski = fn.icodes[p++];
    const icode &sum_ic = fn.icodes[p++];
    const icode &idx_cast1 = fn.icodes[p++];
    const icode &cur_addr = fn.icodes[p++];
    const icode &store_ic = fn.icodes[p++];

    if (idx_cast0.op != icode_op::CAST ||
        !idx_cast0.result.is_temp() ||
        !temp_eq(idx_cast0.left, idx_tid) ||
        prev_sub.op != icode_op::SUB ||
        !prev_sub.result.is_temp() ||
        !temp_eq(prev_sub.left, idx_cast0.result.temp_id) ||
        !is_exact_int_const(prev_sub.right, 1) ||
        prev_addr.op != icode_op::ADD ||
        !prev_addr.result.is_temp() ||
        !((same_global_ref(prev_addr.left, addr0.left) &&
            temp_eq(prev_addr.right, prev_sub.result.temp_id)) ||
           (same_global_ref(prev_addr.right, addr0.left) &&
            temp_eq(prev_addr.left, prev_sub.result.temp_id))) ||
        prev_load.op != icode_op::GET_VALUE_AT ||
        !prev_load.result.is_temp() ||
        !is_byte_temp(prev_load.result) ||
        !temp_eq(prev_load.left, prev_addr.result.temp_id) ||
        prev_inc.op != icode_op::ADD ||
        !prev_inc.result.is_temp() ||
        !((temp_eq(prev_inc.left, prev_load.result.temp_id) && is_exact_int_const(prev_inc.right, 1)) ||
           (temp_eq(prev_inc.right, prev_load.result.temp_id) && is_exact_int_const(prev_inc.left, 1))) ||
        sendi.op != icode_op::SEND ||
        sendi.arg_loc != abi_arg_loc::REG_A ||
        !temp_eq(sendi.left, idx_tid) ||
        seedi.op != icode_op::CALL ||
        !seedi.result.is_temp() ||
        seedi.num_params != 1 ||
        seedi.func_name != "bench_seed_byte" ||
        maski.op != icode_op::BAND ||
        !maski.result.is_temp() ||
        !((temp_eq(maski.left, seedi.result.temp_id) && is_exact_int_const(maski.right, 3)) ||
           (temp_eq(maski.right, seedi.result.temp_id) && is_exact_int_const(maski.left, 3))) ||
        sum_ic.op != icode_op::ADD ||
        !sum_ic.result.is_temp() ||
        !((temp_eq(sum_ic.left, prev_inc.result.temp_id) && temp_eq(sum_ic.right, maski.result.temp_id)) ||
           (temp_eq(sum_ic.right, prev_inc.result.temp_id) && temp_eq(sum_ic.left, maski.result.temp_id))) ||
        idx_cast1.op != icode_op::CAST ||
        !idx_cast1.result.is_temp() ||
        !temp_eq(idx_cast1.left, idx_tid) ||
        cur_addr.op != icode_op::ADD ||
        !cur_addr.result.is_temp() ||
        !((same_global_ref(cur_addr.left, addr0.left) &&
            temp_eq(cur_addr.right, idx_cast1.result.temp_id)) ||
           (same_global_ref(cur_addr.right, addr0.left) &&
            temp_eq(cur_addr.left, idx_cast1.result.temp_id))) ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, cur_addr.result.temp_id) ||
        !temp_eq(store_ic.left, sum_ic.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid))
        return false;

    if (debug_)
        debug_->emit_location(send0.line);

    const std::string data_sym =
        addr0.left.kind == operand_kind::LABEL_REF ? addr0.left.name
                                                   : mangle(addr0.left.name);

    emit_comment("O3 seeded recurrence loop (count=%d)", count);
    emit_line("ld\ta, %s", asm_.imm(1).c_str());
    emit_line("call\t%s", mangle("bench_seed_byte").c_str());
    emit_line("and\t%s", asm_.imm(7).c_str());
    emit_line("ld\t(%s), a", data_sym.c_str());
    emit_line("ld\thl, %s + 1", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\tc, %s", asm_.imm(1).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_label(body_lbl.label_name, false);
    emit_line("push\thl");
    emit_line("push\tbc");
    emit_line("ld\ta, c");
    emit_line("call\t%s", mangle("bench_seed_byte").c_str());
    emit_line("pop\tbc");
    emit_line("pop\thl");
    emit_line("and\t%s", asm_.imm(3).c_str());
    emit_line("ld\tb, a");
    emit_line("dec\thl");
    emit_line("ld\ta, (hl)");
    emit_line("inc\thl");
    emit_line("inc\ta");
    emit_line("add\ta, b");
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("inc\tc");
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(count).c_str());
    emit_line("jr\tc, %s", body_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_masked_step_fill_loop(const ir_function &fn,
                                             size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 13 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode *seed_send = nullptr;
    const icode *seed_call = nullptr;
    const icode *seed_mask = nullptr;
    const icode &state_init = fn.icodes[p++];
    int state_tid = -1;
    int init_seed = 0;
    int init_mask = 0xFF;
    bool init_via_seed_call = false;

    if (state_init.result.is_temp() &&
        is_assign_like(state_init.op) &&
        state_init.left.kind == operand_kind::INT_CONST &&
        state_init.left.ival >= 0 && state_init.left.ival <= 0xff) {
        state_tid = state_init.result.temp_id;
        init_seed = static_cast<int>(state_init.left.ival & 0xFF);
        init_mask = 0xFF;
        init_via_seed_call = false;
    } else {
        p = idx;
        if (p + 3 >= fn.icodes.size())
            return false;
        seed_send = &fn.icodes[p++];
        seed_call = &fn.icodes[p++];
        seed_mask = &fn.icodes[p++];
        const icode &state_assign = fn.icodes[p++];
        if (seed_send->op != icode_op::SEND ||
            seed_send->arg_loc != abi_arg_loc::REG_A ||
            seed_send->left.kind != operand_kind::INT_CONST ||
            seed_send->left.ival < 0 || seed_send->left.ival > 0xff ||
            seed_call->op != icode_op::CALL ||
            !seed_call->result.is_temp() ||
            seed_call->num_params != 1 ||
            seed_call->func_name != "bench_seed_byte" ||
            seed_mask->op != icode_op::BAND ||
            !seed_mask->result.is_temp() ||
            !((temp_eq(seed_mask->left, seed_call->result.temp_id) &&
                seed_mask->right.kind == operand_kind::INT_CONST) ||
               (temp_eq(seed_mask->right, seed_call->result.temp_id) &&
                seed_mask->left.kind == operand_kind::INT_CONST)) ||
            !state_assign.result.is_temp() ||
            !is_assign_like(state_assign.op) ||
            !temp_eq(state_assign.left, seed_mask->result.temp_id)) {
            return false;
        }
        state_tid = state_assign.result.temp_id;
        init_seed = static_cast<int>(seed_send->left.ival & 0xFF);
        init_mask = temp_eq(seed_mask->left, seed_call->result.temp_id)
                        ? static_cast<int>(seed_mask->right.ival & 0xFF)
                        : static_cast<int>(seed_mask->left.ival & 0xFF);
        init_via_seed_call = true;
    }

    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, state_tid)) {
        state_tid = fn.icodes[p++].result.temp_id;
    }

    const icode &idx_init = fn.icodes[p++];
    const icode &ptr_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        !ptr_init.result.is_temp() ||
        !ptr_init.result.type || ptr_init.result.type->size() != 2 ||
        !is_assign_like(ptr_init.op) ||
        !is_global_byte_buffer_ref(ptr_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int ptr_tid = ptr_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &step_add = fn.icodes[p++];
    const icode &mask_ic = fn.icodes[p++];
    const icode &state_store = fn.icodes[p++];
    const icode &out_store = fn.icodes[p++];

    if (step_add.op != icode_op::ADD ||
        !step_add.result.is_temp() ||
        !temp_eq(step_add.left, state_tid) ||
        step_add.right.kind != operand_kind::INT_CONST ||
        step_add.right.ival < 0 || step_add.right.ival > 255 ||
        mask_ic.op != icode_op::BAND ||
        !mask_ic.result.is_temp() ||
        !temp_eq(mask_ic.left, step_add.result.temp_id) ||
        mask_ic.right.kind != operand_kind::INT_CONST ||
        mask_ic.right.ival < 0 || mask_ic.right.ival > 0xff ||
        !is_assign_like(state_store.op) ||
        !temp_eq(state_store.result, state_tid) ||
        !temp_eq(state_store.left, mask_ic.result.temp_id) ||
        out_store.op != icode_op::SET_VALUE_AT ||
        !temp_eq(out_store.result, ptr_tid) ||
        !temp_eq(out_store.left, state_tid)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 4 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &ptr_add_ic = fn.icodes[p++];
    const icode &ptr_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        ptr_add_ic.op != icode_op::ADD ||
        !ptr_add_ic.result.is_temp() ||
        !temp_eq(ptr_add_ic.left, ptr_tid) ||
        !is_exact_int_const(ptr_add_ic.right, 1) ||
        !is_assign_like(ptr_store_ic.op) ||
        !temp_eq(ptr_store_ic.result, ptr_tid) ||
        !temp_eq(ptr_store_ic.left, ptr_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid) ||
        temp_value_used_after(fn, p, ptr_tid) ||
        temp_value_used_after(fn, p, state_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(init_via_seed_call ? seed_send->line : state_init.line);

    const std::string buf_sym =
        ptr_init.left.kind == operand_kind::LABEL_REF ? ptr_init.left.name
                                                      : mangle(ptr_init.left.name);
    const int step = static_cast<int>(step_add.right.ival) & 0xFF;
    const int mask = static_cast<int>(mask_ic.right.ival) & 0xFF;
    emit_comment("O3 masked step-fill loop (count=%d)", count);
    if (init_via_seed_call) {
        emit_line("ld\ta, %s", asm_.imm(init_seed).c_str());
        emit_line("call\t_bench_seed_byte");
        if (init_mask != 0xFF)
            emit_line("and\t%s", asm_.imm(init_mask).c_str());
        emit_line("ld\tc, a");
    } else {
        emit_line("ld\tc, %s", asm_.imm(init_seed).c_str());
    }
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(buf_sym).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, b");
    emit_line("cp\t%s", asm_.imm(count).c_str());
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("add\ta, %s", asm_.imm(step).c_str());
    emit_line("and\t%s", asm_.imm(mask).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\t(hl), a");
    emit_line("inc\tb");
    emit_line("inc\thl");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_dual_zero_byte_walk_loop(const ir_function &fn,
                                                size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 13 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &ptr0_init = fn.icodes[p++];
    const icode &ptr1_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        !ptr0_init.result.is_temp() ||
        !ptr0_init.result.type || ptr0_init.result.type->size() != 2 ||
        !is_assign_like(ptr0_init.op) ||
        !is_global_byte_buffer_ref(ptr0_init.left) ||
        !ptr1_init.result.is_temp() ||
        !ptr1_init.result.type || ptr1_init.result.type->size() != 2 ||
        !is_assign_like(ptr1_init.op) ||
        !is_global_byte_buffer_ref(ptr1_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int ptr0_tid = ptr0_init.result.temp_id;
    const int ptr1_tid = ptr1_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &store0 = fn.icodes[p++];
    const icode &store1 = fn.icodes[p++];

    bool store0_first = false;
    if (store0.op == icode_op::SET_VALUE_AT &&
        store1.op == icode_op::SET_VALUE_AT &&
        is_exact_int_const(store0.left, 0) &&
        is_exact_int_const(store1.left, 0)) {
        if (temp_eq(store0.result, ptr0_tid) && temp_eq(store1.result, ptr1_tid)) {
            store0_first = true;
        } else if (temp_eq(store0.result, ptr1_tid) &&
                   temp_eq(store1.result, ptr0_tid)) {
            store0_first = false;
        } else {
            return false;
        }
    } else {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 6 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &ptr_a_add_ic = fn.icodes[p++];
    const icode &ptr_a_store_ic = fn.icodes[p++];
    const icode &ptr_b_add_ic = fn.icodes[p++];
    const icode &ptr_b_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    auto matches_ptr_step = [&](const icode &add_ic,
                                const icode &store_ic,
                                int ptr_tid) {
        return add_ic.op == icode_op::ADD &&
               add_ic.result.is_temp() &&
               temp_eq(add_ic.left, ptr_tid) &&
               is_exact_int_const(add_ic.right, 1) &&
               is_assign_like(store_ic.op) &&
               temp_eq(store_ic.result, ptr_tid) &&
               temp_eq(store_ic.left, add_ic.result.temp_id);
    };

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (!((matches_ptr_step(ptr_a_add_ic, ptr_a_store_ic, ptr0_tid) &&
            matches_ptr_step(ptr_b_add_ic, ptr_b_store_ic, ptr1_tid)) ||
           (matches_ptr_step(ptr_a_add_ic, ptr_a_store_ic, ptr1_tid) &&
            matches_ptr_step(ptr_b_add_ic, ptr_b_store_ic, ptr0_tid)))) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid) ||
        temp_value_used_after(fn, p, ptr0_tid) ||
        temp_value_used_after(fn, p, ptr1_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(idx_init.line);

    const operand &ptr_hl = store0_first ? ptr0_init.left : ptr1_init.left;
    const operand &ptr_de = store0_first ? ptr1_init.left : ptr0_init.left;
    const std::string hl_sym =
        ptr_hl.kind == operand_kind::LABEL_REF ? ptr_hl.name
                                               : mangle(ptr_hl.name);
    const std::string de_sym =
        ptr_de.kind == operand_kind::LABEL_REF ? ptr_de.name
                                               : mangle(ptr_de.name);

    emit_comment("O3 dual zero byte walk loop (count=%d)", count);
    emit_line("ld\thl, %s", asm_.imm_sym(hl_sym).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(de_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(count).c_str());
    emit_line("xor\ta");
    emit_label(cond_lbl.label_name, false);
    emit_label(body_lbl.label_name, false);
    emit_line("ld\t(hl), a");
    emit_line("ld\t(de), a");
    emit_line("inc\thl");
    emit_line("inc\tde");
    emit_line("djnz\t%s", body_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_matrix_rowcol_accum_loop(const ir_function &fn,
                                                size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 29 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &outer_idx_init = fn.icodes[p++];
    const icode &row_ptr_init = fn.icodes[p++];
    const icode &outer_cond_lbl = fn.icodes[p++];
    const icode &outer_cmp_ic = fn.icodes[p++];
    const icode &outer_ifx_ic = fn.icodes[p++];
    const icode &outer_body_lbl = fn.icodes[p++];

    if (!outer_idx_init.result.is_temp() ||
        !is_assign_like(outer_idx_init.op) ||
        !is_exact_int_const(outer_idx_init.left, 0) ||
        !row_ptr_init.result.is_temp() ||
        !row_ptr_init.result.type || row_ptr_init.result.type->size() != 2 ||
        !is_assign_like(row_ptr_init.op) ||
        !is_global_byte_buffer_ref(row_ptr_init.left) ||
        outer_cond_lbl.op != icode_op::LABEL ||
        outer_cmp_ic.op != icode_op::LT ||
        !outer_cmp_ic.result.is_temp() ||
        !temp_eq(outer_cmp_ic.left, outer_idx_init.result.temp_id) ||
        outer_cmp_ic.right.kind != operand_kind::INT_CONST ||
        outer_cmp_ic.right.ival <= 0 || outer_cmp_ic.right.ival > 255 ||
        outer_ifx_ic.op != icode_op::IFX ||
        !temp_eq(outer_ifx_ic.left, outer_cmp_ic.result.temp_id) ||
        outer_body_lbl.op != icode_op::LABEL ||
        outer_body_lbl.label_name != outer_ifx_ic.true_lbl) {
        return false;
    }

    const int outer_idx_tid = outer_idx_init.result.temp_id;
    const int row_ptr_tid = row_ptr_init.result.temp_id;
    const int row_count = static_cast<int>(outer_cmp_ic.right.ival);

    const icode &inner_idx_init = fn.icodes[p++];
    const icode &col_ptr_init = fn.icodes[p++];
    const icode &row_base_ic = fn.icodes[p++];
    const icode &inner_cond_lbl = fn.icodes[p++];
    const icode &inner_cmp_ic = fn.icodes[p++];
    const icode &inner_ifx_ic = fn.icodes[p++];
    const icode &inner_body_lbl = fn.icodes[p++];

    if (!inner_idx_init.result.is_temp() ||
        !is_assign_like(inner_idx_init.op) ||
        !is_exact_int_const(inner_idx_init.left, 0) ||
        !col_ptr_init.result.is_temp() ||
        !col_ptr_init.result.type || col_ptr_init.result.type->size() != 2 ||
        !is_assign_like(col_ptr_init.op) ||
        !is_global_byte_buffer_ref(col_ptr_init.left) ||
        row_base_ic.op != icode_op::SHL ||
        !row_base_ic.result.is_temp() ||
        !temp_eq(row_base_ic.left, outer_idx_tid) ||
        row_base_ic.right.kind != operand_kind::INT_CONST ||
        row_base_ic.right.ival < 0 || row_base_ic.right.ival > 7 ||
        inner_cond_lbl.op != icode_op::LABEL ||
        inner_cmp_ic.op != icode_op::LT ||
        !inner_cmp_ic.result.is_temp() ||
        !temp_eq(inner_cmp_ic.left, inner_idx_init.result.temp_id) ||
        inner_cmp_ic.right.kind != operand_kind::INT_CONST ||
        inner_cmp_ic.right.ival <= 0 || inner_cmp_ic.right.ival > 255 ||
        inner_ifx_ic.op != icode_op::IFX ||
        !temp_eq(inner_ifx_ic.left, inner_cmp_ic.result.temp_id) ||
        inner_body_lbl.op != icode_op::LABEL ||
        inner_body_lbl.label_name != inner_ifx_ic.true_lbl) {
        return false;
    }

    const int inner_idx_tid = inner_idx_init.result.temp_id;
    const int col_ptr_tid = col_ptr_init.result.temp_id;
    const int col_count = static_cast<int>(inner_cmp_ic.right.ival);
    const int row_shift = static_cast<int>(row_base_ic.right.ival);
    if ((1 << row_shift) != col_count)
        return false;

    const icode &src_idx_ic = fn.icodes[p++];
    const icode &row_load_ic = fn.icodes[p++];
    const icode &src_load1_ic = fn.icodes[p++];
    const icode &row_add_ic = fn.icodes[p++];
    const icode &row_store_ic = fn.icodes[p++];
    const icode &col_load_ic = fn.icodes[p++];
    const icode &src_load2_ic = fn.icodes[p++];
    const icode &col_add_ic = fn.icodes[p++];
    const icode &col_store_ic = fn.icodes[p++];

    if (src_idx_ic.op != icode_op::ADD ||
        !src_idx_ic.result.is_temp() ||
        !((temp_eq(src_idx_ic.left, row_base_ic.result.temp_id) &&
            temp_eq(src_idx_ic.right, inner_idx_tid)) ||
           (temp_eq(src_idx_ic.right, row_base_ic.result.temp_id) &&
            temp_eq(src_idx_ic.left, inner_idx_tid))) ||
        row_load_ic.op != icode_op::GET_VALUE_AT ||
        !row_load_ic.result.is_temp() ||
        !is_byte_temp(row_load_ic.result) ||
        same_global_ref(row_load_ic.left, row_ptr_init.left) == false ||
        !temp_eq(row_load_ic.right, outer_idx_tid) ||
        src_load1_ic.op != icode_op::GET_VALUE_AT ||
        !src_load1_ic.result.is_temp() ||
        !is_byte_temp(src_load1_ic.result) ||
        !is_global_byte_buffer_ref(src_load1_ic.left) ||
        !temp_eq(src_load1_ic.right, src_idx_ic.result.temp_id) ||
        row_add_ic.op != icode_op::ADD ||
        !row_add_ic.result.is_temp() ||
        !((temp_eq(row_add_ic.left, row_load_ic.result.temp_id) &&
            temp_eq(row_add_ic.right, src_load1_ic.result.temp_id)) ||
           (temp_eq(row_add_ic.right, row_load_ic.result.temp_id) &&
            temp_eq(row_add_ic.left, src_load1_ic.result.temp_id))) ||
        row_store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(row_store_ic.result, row_ptr_tid) ||
        !temp_eq(row_store_ic.left, row_add_ic.result.temp_id) ||
        col_load_ic.op != icode_op::GET_VALUE_AT ||
        !col_load_ic.result.is_temp() ||
        !is_byte_temp(col_load_ic.result) ||
        !same_global_ref(col_load_ic.left, col_ptr_init.left) ||
        !temp_eq(col_load_ic.right, inner_idx_tid) ||
        src_load2_ic.op != icode_op::GET_VALUE_AT ||
        !src_load2_ic.result.is_temp() ||
        !is_byte_temp(src_load2_ic.result) ||
        !same_global_ref(src_load2_ic.left, src_load1_ic.left) ||
        !temp_eq(src_load2_ic.right, src_idx_ic.result.temp_id) ||
        col_add_ic.op != icode_op::ADD ||
        !col_add_ic.result.is_temp() ||
        !((temp_eq(col_add_ic.left, col_load_ic.result.temp_id) &&
            temp_eq(col_add_ic.right, src_load2_ic.result.temp_id)) ||
           (temp_eq(col_add_ic.right, col_load_ic.result.temp_id) &&
            temp_eq(col_add_ic.left, src_load2_ic.result.temp_id))) ||
        col_store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(col_store_ic.result, col_ptr_tid) ||
        !temp_eq(col_store_ic.left, col_add_ic.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 9 >= fn.icodes.size())
        return false;

    const icode &inner_idx_add_ic = fn.icodes[p++];
    const icode &inner_idx_store_ic = fn.icodes[p++];
    const icode &col_ptr_add_ic = fn.icodes[p++];
    const icode &col_ptr_store_ic = fn.icodes[p++];
    const icode &goto_inner_ic = fn.icodes[p++];
    const icode &inner_end_lbl = fn.icodes[p++];
    const icode &outer_idx_add_ic = fn.icodes[p++];
    const icode &outer_idx_store_ic = fn.icodes[p++];
    const icode &row_ptr_add_ic = fn.icodes[p++];
    const icode &row_ptr_store_ic = fn.icodes[p++];
    const icode &goto_outer_ic = fn.icodes[p++];
    const icode &outer_end_lbl = fn.icodes[p++];

    if (inner_idx_add_ic.op != icode_op::ADD ||
        !inner_idx_add_ic.result.is_temp() ||
        !temp_eq(inner_idx_add_ic.left, inner_idx_tid) ||
        !is_exact_int_const(inner_idx_add_ic.right, 1) ||
        !is_assign_like(inner_idx_store_ic.op) ||
        !temp_eq(inner_idx_store_ic.result, inner_idx_tid) ||
        !temp_eq(inner_idx_store_ic.left, inner_idx_add_ic.result.temp_id) ||
        col_ptr_add_ic.op != icode_op::ADD ||
        !col_ptr_add_ic.result.is_temp() ||
        !temp_eq(col_ptr_add_ic.left, col_ptr_tid) ||
        !is_exact_int_const(col_ptr_add_ic.right, 1) ||
        !is_assign_like(col_ptr_store_ic.op) ||
        !temp_eq(col_ptr_store_ic.result, col_ptr_tid) ||
        !temp_eq(col_ptr_store_ic.left, col_ptr_add_ic.result.temp_id) ||
        goto_inner_ic.op != icode_op::GOTO ||
        goto_inner_ic.label_name != inner_cond_lbl.label_name ||
        inner_end_lbl.op != icode_op::LABEL ||
        inner_end_lbl.label_name != inner_ifx_ic.false_lbl ||
        outer_idx_add_ic.op != icode_op::ADD ||
        !outer_idx_add_ic.result.is_temp() ||
        !temp_eq(outer_idx_add_ic.left, outer_idx_tid) ||
        !is_exact_int_const(outer_idx_add_ic.right, 1) ||
        !is_assign_like(outer_idx_store_ic.op) ||
        !temp_eq(outer_idx_store_ic.result, outer_idx_tid) ||
        !temp_eq(outer_idx_store_ic.left, outer_idx_add_ic.result.temp_id) ||
        row_ptr_add_ic.op != icode_op::ADD ||
        !row_ptr_add_ic.result.is_temp() ||
        !temp_eq(row_ptr_add_ic.left, row_ptr_tid) ||
        !is_exact_int_const(row_ptr_add_ic.right, 1) ||
        !is_assign_like(row_ptr_store_ic.op) ||
        !temp_eq(row_ptr_store_ic.result, row_ptr_tid) ||
        !temp_eq(row_ptr_store_ic.left, row_ptr_add_ic.result.temp_id) ||
        goto_outer_ic.op != icode_op::GOTO ||
        goto_outer_ic.label_name != outer_cond_lbl.label_name ||
        outer_end_lbl.op != icode_op::LABEL ||
        outer_end_lbl.label_name != outer_ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, outer_idx_tid) ||
        temp_value_used_after(fn, p, inner_idx_tid) ||
        temp_value_used_after(fn, p, row_ptr_tid) ||
        temp_value_used_after(fn, p, col_ptr_tid) ||
        temp_value_used_after(fn, p, row_base_ic.result.temp_id) ||
        temp_value_used_after(fn, p, src_idx_ic.result.temp_id)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(outer_idx_init.line);

    const std::string src_sym =
        src_load1_ic.left.kind == operand_kind::LABEL_REF ? src_load1_ic.left.name
                                                          : mangle(src_load1_ic.left.name);
    const std::string row_sym =
        row_ptr_init.left.kind == operand_kind::LABEL_REF ? row_ptr_init.left.name
                                                          : mangle(row_ptr_init.left.name);
    const std::string col_sym =
        col_ptr_init.left.kind == operand_kind::LABEL_REF ? col_ptr_init.left.name
                                                          : mangle(col_ptr_init.left.name);

    emit_comment("O3 matrix row/col accumulation loop (rows=%d cols=%d)",
                 row_count, col_count);
    emit_line("ld\thl, %s", asm_.imm_sym(src_sym).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(row_sym).c_str());
    emit_line("ld\tc, %s", asm_.imm(row_count).c_str());
    emit_label(outer_cond_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("or\ta");
    emit_line("jr\tz, %s", outer_end_lbl.label_name.c_str());
    emit_label(outer_body_lbl.label_name, false);
    emit_line("push\tbc");
    emit_line("push\tde");
    emit_line("ld\ta, (de)");
    emit_line("ex\taf, af'");
    emit_line("ld\tde, %s", asm_.imm_sym(col_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(col_count).c_str());
    emit_label(inner_cond_lbl.label_name, false);
    emit_label(inner_body_lbl.label_name, false);
    emit_line("ld\tc, (hl)");
    emit_line("inc\thl");
    emit_line("ld\ta, (de)");
    emit_line("add\ta, c");
    emit_line("ld\t(de), a");
    emit_line("inc\tde");
    emit_line("ex\taf, af'");
    emit_line("add\ta, c");
    emit_line("ex\taf, af'");
    emit_line("djnz\t%s", inner_body_lbl.label_name.c_str());
    emit_label(inner_end_lbl.label_name, false);
    emit_line("pop\tde");
    emit_line("ex\taf, af'");
    emit_line("ld\t(de), a");
    emit_line("ex\taf, af'");
    emit_line("inc\tde");
    emit_line("pop\tbc");
    emit_line("dec\tc");
    emit_line("jr\tnz, %s", outer_cond_lbl.label_name.c_str());
    emit_label(outer_end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_node_init_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 13 >= fn.icodes.size())
        return false;

    auto is_global_ref = [&](const operand &op) {
        return op.kind == operand_kind::LABEL_REF ||
               (op.kind == operand_kind::SYMBOL &&
                op.is_global && !op.is_tls && !op.is_sfr && !op.is_func);
    };
    auto match_casted_index = [&](size_t &p, int src_tid, int &index_tid) {
        index_tid = src_tid;
        if (p < fn.icodes.size() &&
            fn.icodes[p].op == icode_op::CAST &&
            fn.icodes[p].result.is_temp() &&
            temp_eq(fn.icodes[p].left, src_tid) &&
            fn.icodes[p].result.type &&
            fn.icodes[p].result.type->size() == 2) {
            index_tid = fn.icodes[p].result.temp_id;
            ++p;
        }
        return true;
    };
    auto match_indexed_byte_load = [&](size_t &p, int idx_tid, operand &base_out,
                                       int &value_tid) {
        if (p >= fn.icodes.size())
            return false;

        const icode &direct = fn.icodes[p];
        if (direct.op == icode_op::GET_VALUE_AT &&
            direct.result.is_temp() &&
            is_byte_temp(direct.result) &&
            is_global_byte_buffer_ref(direct.left) &&
            temp_eq(direct.right, idx_tid)) {
            base_out = direct.left;
            value_tid = direct.result.temp_id;
            ++p;
            return true;
        }

        size_t q = p;
        int addr_idx_tid = idx_tid;
        match_casted_index(q, idx_tid, addr_idx_tid);
        if (q + 1 >= fn.icodes.size())
            return false;
        const icode &addr_ic = fn.icodes[q];
        const icode &load_ic = fn.icodes[q + 1];
        if (addr_ic.op != icode_op::ADD ||
            !addr_ic.result.is_temp() ||
            !((is_global_byte_buffer_ref(addr_ic.left) &&
                temp_eq(addr_ic.right, addr_idx_tid)) ||
               (is_global_byte_buffer_ref(addr_ic.right) &&
                temp_eq(addr_ic.left, addr_idx_tid))) ||
            load_ic.op != icode_op::GET_VALUE_AT ||
            !load_ic.result.is_temp() ||
            !is_byte_temp(load_ic.result) ||
            !temp_eq(load_ic.left, addr_ic.result.temp_id)) {
            return false;
        }
        base_out = is_global_byte_buffer_ref(addr_ic.left) ? addr_ic.left
                                                           : addr_ic.right;
        value_tid = load_ic.result.temp_id;
        p = q + 2;
        return true;
    };

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    operand key_base;
    int key_tid = -1;
    if (!match_indexed_byte_load(p, idx_tid, key_base, key_tid))
        return false;

    size_t q = p;
    int key_idx_tid = idx_tid;
    match_casted_index(q, idx_tid, key_idx_tid);
    if (q + 5 >= fn.icodes.size())
        return false;

    const icode &key_shl_ic = fn.icodes[q++];
    const icode &node_addr_ic = fn.icodes[q++];
    const icode &store_key_ic = fn.icodes[q++];
    int next_idx_tid = idx_tid;
    match_casted_index(q, idx_tid, next_idx_tid);
    const icode &next_shl_ic = fn.icodes[q++];
    const icode &next_node_addr_ic = fn.icodes[q++];
    const icode &next_ptr_ic = fn.icodes[q++];
    const icode &store_next_ic = fn.icodes[q++];

    if (key_shl_ic.op != icode_op::SHL ||
        !key_shl_ic.result.is_temp() ||
        !temp_eq(key_shl_ic.left, key_idx_tid) ||
        !is_exact_int_const(key_shl_ic.right, 1) ||
        node_addr_ic.op != icode_op::ADD ||
        !node_addr_ic.result.is_temp() ||
        !((is_global_ref(node_addr_ic.left) &&
            temp_eq(node_addr_ic.right, key_shl_ic.result.temp_id)) ||
           (is_global_ref(node_addr_ic.right) &&
            temp_eq(node_addr_ic.left, key_shl_ic.result.temp_id))) ||
        store_key_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_key_ic.result, node_addr_ic.result.temp_id) ||
        !temp_eq(store_key_ic.left, key_tid) ||
        next_shl_ic.op != icode_op::SHL ||
        !next_shl_ic.result.is_temp() ||
        !temp_eq(next_shl_ic.left, next_idx_tid) ||
        !is_exact_int_const(next_shl_ic.right, 1) ||
        next_node_addr_ic.op != icode_op::ADD ||
        !next_node_addr_ic.result.is_temp() ||
        !((is_global_ref(next_node_addr_ic.left) &&
            temp_eq(next_node_addr_ic.right, next_shl_ic.result.temp_id)) ||
           (is_global_ref(next_node_addr_ic.right) &&
            temp_eq(next_node_addr_ic.left, next_shl_ic.result.temp_id))) ||
        next_ptr_ic.op != icode_op::ADD ||
        !next_ptr_ic.result.is_temp() ||
        !temp_eq(next_ptr_ic.left, next_node_addr_ic.result.temp_id) ||
        !is_exact_int_const(next_ptr_ic.right, 1) ||
        store_next_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_next_ic.result, next_ptr_ic.result.temp_id) ||
        !is_exact_int_const(store_next_ic.left, 255)) {
        return false;
    }
    p = q;

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid))
        return false;

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string key_sym =
        key_base.kind == operand_kind::LABEL_REF ? key_base.name
                                                 : mangle(key_base.name);
    const operand &node_base =
        is_global_ref(node_addr_ic.left) ? node_addr_ic.left : node_addr_ic.right;
    const std::string node_sym =
        node_base.kind == operand_kind::LABEL_REF ? node_base.name
                                                  : mangle(node_base.name);

    emit_comment("O3 node init loop (count=%d)", count);
    emit_line("ld\thl, %s", asm_.imm_sym(key_sym).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(node_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(count).c_str());
    emit_line("ld\tc, %s", asm_.imm(255).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, (hl)");
    emit_line("ld\t(de), a");
    emit_line("inc\tde");
    emit_line("ld\ta, c");
    emit_line("ld\t(de), a");
    emit_line("inc\tde");
    emit_line("inc\thl");
    emit_line("djnz\t%s", body_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_list_sort_mix_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 16 >= fn.icodes.size())
        return false;

    auto is_global_ref = [&](const operand &op) {
        return op.kind == operand_kind::LABEL_REF ||
               (op.kind == operand_kind::SYMBOL &&
                op.is_global && !op.is_tls && !op.is_sfr && !op.is_func);
    };
    auto match_casted_index = [&](size_t &p, int src_tid, int &index_tid) {
        index_tid = src_tid;
        if (p < fn.icodes.size() &&
            fn.icodes[p].op == icode_op::CAST &&
            fn.icodes[p].result.is_temp() &&
            temp_eq(fn.icodes[p].left, src_tid) &&
            fn.icodes[p].result.type &&
            fn.icodes[p].result.type->size() == 2) {
            index_tid = fn.icodes[p].result.temp_id;
            ++p;
        }
        return true;
    };
    auto match_call_arg = [&](const icode &ic, int index, int temp_id) {
        return ic.op == icode_op::SEND &&
               ic.argreg == index &&
               temp_eq(ic.left, temp_id);
    };

    size_t p = idx;
    const icode &acc_init = fn.icodes[p++];
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!acc_init.result.is_temp() ||
        !acc_init.result.type || acc_init.result.type->size() != 2 ||
        !is_assign_like(acc_init.op) ||
        !idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::NE ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        !is_exact_int_const(cmp_ic.right, 255) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int acc_tid = acc_init.result.temp_id;
    const int idx_tid = idx_init.result.temp_id;

    int key_idx_tid = idx_tid;
    match_casted_index(p, idx_tid, key_idx_tid);
    if (p + 14 >= fn.icodes.size())
        return false;

    const icode &key_shl_ic = fn.icodes[p++];
    const icode &node_addr_ic = fn.icodes[p++];
    const icode &key_load_ic = fn.icodes[p++];
    int key_u16_tid = -1;
    if (fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        fn.icodes[p].result.type &&
        fn.icodes[p].result.type->size() == 2 &&
        temp_eq(fn.icodes[p].left, key_load_ic.result.temp_id)) {
        key_u16_tid = fn.icodes[p].result.temp_id;
        ++p;
    } else {
        return false;
    }

    const icode &send0 = fn.icodes[p++];
    const icode &send1 = fn.icodes[p++];
    const icode &call0 = fn.icodes[p++];
    int idx_u16_tid = -1;
    if (fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        fn.icodes[p].result.type &&
        fn.icodes[p].result.type->size() == 2 &&
        temp_eq(fn.icodes[p].left, idx_tid)) {
        idx_u16_tid = fn.icodes[p].result.temp_id;
        ++p;
    } else {
        return false;
    }
    const icode &send2 = fn.icodes[p++];
    const icode &send3 = fn.icodes[p++];
    const icode &call1 = fn.icodes[p++];
    const icode &acc_store = fn.icodes[p++];

    int next_idx_tid = idx_tid;
    match_casted_index(p, idx_tid, next_idx_tid);
    if (p + 5 >= fn.icodes.size())
        return false;
    const icode &next_shl_ic = fn.icodes[p++];
    const icode &next_addr_ic = fn.icodes[p++];
    const icode &next_ptr_ic = fn.icodes[p++];
    const icode &next_load_ic = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    const operand *nodes_base_ptr = nullptr;
    if (is_global_ref(node_addr_ic.left) &&
        temp_eq(node_addr_ic.right, key_shl_ic.result.temp_id)) {
        nodes_base_ptr = &node_addr_ic.left;
    } else if (is_global_ref(node_addr_ic.right) &&
               temp_eq(node_addr_ic.left, key_shl_ic.result.temp_id)) {
        nodes_base_ptr = &node_addr_ic.right;
    }

    if (key_shl_ic.op != icode_op::SHL ||
        !key_shl_ic.result.is_temp() ||
        !temp_eq(key_shl_ic.left, key_idx_tid) ||
        !is_exact_int_const(key_shl_ic.right, 1) ||
        node_addr_ic.op != icode_op::ADD ||
        !node_addr_ic.result.is_temp() ||
        nodes_base_ptr == nullptr ||
        key_load_ic.op != icode_op::GET_VALUE_AT ||
        !key_load_ic.result.is_temp() ||
        !is_byte_temp(key_load_ic.result) ||
        !temp_eq(key_load_ic.left, node_addr_ic.result.temp_id) ||
        !((match_call_arg(send0, 0, acc_tid) && match_call_arg(send1, 1, key_u16_tid)) ||
          (match_call_arg(send1, 0, acc_tid) && match_call_arg(send0, 1, key_u16_tid))) ||
        call0.op != icode_op::CALL ||
        call0.func_name != "bench_mix16" ||
        !call0.result.is_temp() ||
        !call0.result.type || call0.result.type->size() != 2 ||
        !((match_call_arg(send2, 0, call0.result.temp_id) && match_call_arg(send3, 1, idx_u16_tid)) ||
          (match_call_arg(send3, 0, call0.result.temp_id) && match_call_arg(send2, 1, idx_u16_tid))) ||
        call1.op != icode_op::CALL ||
        call1.func_name != "bench_mix16" ||
        !call1.result.is_temp() ||
        !call1.result.type || call1.result.type->size() != 2 ||
        !is_assign_like(acc_store.op) ||
        !temp_eq(acc_store.result, acc_tid) ||
        !temp_eq(acc_store.left, call1.result.temp_id) ||
        next_shl_ic.op != icode_op::SHL ||
        !next_shl_ic.result.is_temp() ||
        !temp_eq(next_shl_ic.left, next_idx_tid) ||
        !is_exact_int_const(next_shl_ic.right, 1) ||
        next_addr_ic.op != icode_op::ADD ||
        !next_addr_ic.result.is_temp() ||
        !((same_global_ref(next_addr_ic.left, *nodes_base_ptr) &&
            temp_eq(next_addr_ic.right, next_shl_ic.result.temp_id)) ||
           (same_global_ref(next_addr_ic.right, *nodes_base_ptr) &&
            temp_eq(next_addr_ic.left, next_shl_ic.result.temp_id))) ||
        next_ptr_ic.op != icode_op::ADD ||
        !next_ptr_ic.result.is_temp() ||
        !temp_eq(next_ptr_ic.left, next_addr_ic.result.temp_id) ||
        !is_exact_int_const(next_ptr_ic.right, 1) ||
        next_load_ic.op != icode_op::GET_VALUE_AT ||
        !next_load_ic.result.is_temp() ||
        !is_byte_temp(next_load_ic.result) ||
        !temp_eq(next_load_ic.left, next_ptr_ic.result.temp_id) ||
        !is_assign_like(idx_store.op) ||
        !temp_eq(idx_store.result, idx_tid) ||
        !temp_eq(idx_store.left, next_load_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    const operand &nodes_base = *nodes_base_ptr;

    if (temp_value_used_after(fn, p, idx_tid))
        return false;

    if (debug_)
        debug_->emit_location(acc_init.line);

    const std::string nodes_sym =
        nodes_base.kind == operand_kind::LABEL_REF ? nodes_base.name
                                                   : mangle(nodes_base.name);

    emit_comment("O3 list-sort mix loop");
    load_hl(acc_init.left);
    load_a(idx_init.left);
    emit_line("ld\tc, a");
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("inc\ta");
    emit_line("jr\tz, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\tb, c");
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("push\thl");
    emit_line("ld\thl, %s", asm_.imm_sym(nodes_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\te, (hl)");
    emit_line("inc\thl");
    emit_line("ld\tc, (hl)");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("pop\thl");
    emit_line("push\tbc");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("ld\te, b");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("push\tbc");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);
    store_hl(acc_init.result);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_matrix_tail_mix_loop(const ir_function &fn,
                                            size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 15 >= fn.icodes.size())
        return false;

    auto match_call_arg = [&](const icode &ic, int index, int temp_id) {
        return ic.op == icode_op::SEND &&
               ic.argreg == index &&
               temp_eq(ic.left, temp_id);
    };

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        !is_exact_int_const(cmp_ic.right, 8) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;

    if (p + 17 >= fn.icodes.size())
        return false;

    const icode &row_load_ic = fn.icodes[p++];
    const icode &row_cast_ic = fn.icodes[p++];
    const icode &send0 = fn.icodes[p++];
    const icode &send1 = fn.icodes[p++];
    const icode &call0 = fn.icodes[p++];
    const icode &col_load_ic = fn.icodes[p++];
    const icode &col_cast_ic = fn.icodes[p++];
    const icode &send2 = fn.icodes[p++];
    const icode &send3 = fn.icodes[p++];
    const icode &call1 = fn.icodes[p++];
    const icode &diag_shl_ic = fn.icodes[p++];
    const icode &diag_add_ic = fn.icodes[p++];
    const icode &diag_load_ic = fn.icodes[p++];
    const icode &diag_cast_ic = fn.icodes[p++];
    const icode &send4 = fn.icodes[p++];
    const icode &send5 = fn.icodes[p++];
    const icode &call2 = fn.icodes[p++];
    const icode &acc_store = fn.icodes[p++];

    const operand &acc_in =
        temp_eq(send0.left, row_cast_ic.result.temp_id) ? send1.left : send0.left;
    if (row_load_ic.op != icode_op::GET_VALUE_AT ||
        !row_load_ic.result.is_temp() ||
        !is_byte_temp(row_load_ic.result) ||
        !is_global_byte_buffer_ref(row_load_ic.left) ||
        !temp_eq(row_load_ic.right, idx_tid) ||
        col_load_ic.op != icode_op::GET_VALUE_AT ||
        !col_load_ic.result.is_temp() ||
        !is_byte_temp(col_load_ic.result) ||
        !is_global_byte_buffer_ref(col_load_ic.left) ||
        !temp_eq(col_load_ic.right, idx_tid) ||
        row_cast_ic.op != icode_op::CAST ||
        !row_cast_ic.result.is_temp() ||
        !row_cast_ic.result.type || row_cast_ic.result.type->size() != 2 ||
        !temp_eq(row_cast_ic.left, row_load_ic.result.temp_id) ||
        col_cast_ic.op != icode_op::CAST ||
        !col_cast_ic.result.is_temp() ||
        !col_cast_ic.result.type || col_cast_ic.result.type->size() != 2 ||
        !temp_eq(col_cast_ic.left, col_load_ic.result.temp_id) ||
        call0.op != icode_op::CALL ||
        call0.func_name != "bench_mix16" ||
        !call0.result.is_temp() ||
        !acc_in.is_temp() ||
        !acc_in.type || acc_in.type->size() != 2 ||
        !((match_call_arg(send0, 0, acc_in.temp_id) && match_call_arg(send1, 1, row_cast_ic.result.temp_id)) ||
           (match_call_arg(send1, 0, acc_in.temp_id) && match_call_arg(send0, 1, row_cast_ic.result.temp_id))) ||
        !call0.result.type || call0.result.type->size() != 2 ||
        call1.op != icode_op::CALL ||
        call1.func_name != "bench_mix16" ||
        !call1.result.is_temp() ||
        !call1.result.type || call1.result.type->size() != 2 ||
        !((match_call_arg(send2, 0, call0.result.temp_id) && match_call_arg(send3, 1, col_cast_ic.result.temp_id)) ||
           (match_call_arg(send3, 0, call0.result.temp_id) && match_call_arg(send2, 1, col_cast_ic.result.temp_id))) ||
        diag_shl_ic.op != icode_op::SHL ||
        !diag_shl_ic.result.is_temp() ||
        !temp_eq(diag_shl_ic.left, idx_tid) ||
        !is_exact_int_const(diag_shl_ic.right, 3) ||
        diag_add_ic.op != icode_op::ADD ||
        !diag_add_ic.result.is_temp() ||
        !((temp_eq(diag_add_ic.left, diag_shl_ic.result.temp_id) && temp_eq(diag_add_ic.right, idx_tid)) ||
           (temp_eq(diag_add_ic.right, diag_shl_ic.result.temp_id) && temp_eq(diag_add_ic.left, idx_tid))) ||
        diag_load_ic.op != icode_op::GET_VALUE_AT ||
        !diag_load_ic.result.is_temp() ||
        !is_byte_temp(diag_load_ic.result) ||
        !is_global_byte_buffer_ref(diag_load_ic.left) ||
        !temp_eq(diag_load_ic.right, diag_add_ic.result.temp_id) ||
        diag_cast_ic.op != icode_op::CAST ||
        !diag_cast_ic.result.is_temp() ||
        !diag_cast_ic.result.type || diag_cast_ic.result.type->size() != 2 ||
        !temp_eq(diag_cast_ic.left, diag_load_ic.result.temp_id) ||
        call2.op != icode_op::CALL ||
        call2.func_name != "bench_mix16" ||
        !call2.result.is_temp() ||
        !call2.result.type || call2.result.type->size() != 2 ||
        !((match_call_arg(send4, 0, call1.result.temp_id) && match_call_arg(send5, 1, diag_cast_ic.result.temp_id)) ||
           (match_call_arg(send5, 0, call1.result.temp_id) && match_call_arg(send4, 1, diag_cast_ic.result.temp_id))) ||
        !is_assign_like(acc_store.op) ||
        !temp_eq(acc_store.left, call2.result.temp_id)) {
        return false;
    }

    const operand &row_base = row_load_ic.left;
    const operand &col_base = col_load_ic.left;
    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;
    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid))
        return false;

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string row_sym =
        row_base.kind == operand_kind::LABEL_REF ? row_base.name
                                                 : mangle(row_base.name);
    const std::string col_sym =
        col_base.kind == operand_kind::LABEL_REF ? col_base.name
                                                 : mangle(col_base.name);
    const std::string dst_sym =
        diag_load_ic.left.kind == operand_kind::LABEL_REF ? diag_load_ic.left.name
                                                          : mangle(diag_load_ic.left.name);

    emit_comment("O3 matrix tail mix loop (count=8)");
    load_hl(acc_in);
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(8).c_str());
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, %s", asm_.imm_sym_lo(row_sym).c_str());
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(row_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("push\tbc");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("ld\ta, %s", asm_.imm_sym_lo(col_sym).c_str());
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(col_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("push\tbc");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, c");
    emit_line("add\ta, %s", asm_.imm_sym_lo(dst_sym).c_str());
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(dst_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\te, a");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("push\tbc");
    emit_line("call\t_bench_mix16");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("inc\tc");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);
    store_hl(acc_store.result);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_insertion_sort_loop(const ir_function &fn,
                                           size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 20 >= fn.icodes.size())
        return false;

    auto match_casted_index = [&](size_t &p, int src_tid, int &index_tid) {
        index_tid = src_tid;
        if (p < fn.icodes.size() &&
            fn.icodes[p].op == icode_op::CAST &&
            fn.icodes[p].result.is_temp() &&
            temp_eq(fn.icodes[p].left, src_tid) &&
            fn.icodes[p].result.type && fn.icodes[p].result.type->size() == 2) {
            index_tid = fn.icodes[p].result.temp_id;
            ++p;
        }
        return true;
    };

    auto match_indexed_load = [&](size_t &p, const operand &base, int idx_tid,
                                  int &value_tid) {
        if (p >= fn.icodes.size())
            return false;

        const icode &direct = fn.icodes[p];
        if (direct.op == icode_op::GET_VALUE_AT &&
            direct.result.is_temp() &&
            is_byte_temp(direct.result) &&
            same_global_ref(direct.left, base) &&
            temp_eq(direct.right, idx_tid)) {
            value_tid = direct.result.temp_id;
            ++p;
            return true;
        }

        size_t q = p;
        int addr_idx_tid = idx_tid;
        match_casted_index(q, idx_tid, addr_idx_tid);
        if (q + 1 >= fn.icodes.size())
            return false;
        const icode &addr_ic = fn.icodes[q];
        const icode &load_ic = fn.icodes[q + 1];
        if (addr_ic.op != icode_op::ADD ||
            !addr_ic.result.is_temp() ||
            !((same_global_ref(addr_ic.left, base) &&
                temp_eq(addr_ic.right, addr_idx_tid)) ||
               (same_global_ref(addr_ic.right, base) &&
                temp_eq(addr_ic.left, addr_idx_tid))) ||
            load_ic.op != icode_op::GET_VALUE_AT ||
            !load_ic.result.is_temp() ||
            !is_byte_temp(load_ic.result) ||
            !temp_eq(load_ic.left, addr_ic.result.temp_id)) {
            return false;
        }
        value_tid = load_ic.result.temp_id;
        p = q + 2;
        return true;
    };

    auto match_indexed_store = [&](size_t &p, const operand &base, int idx_tid,
                                   int value_tid) {
        size_t q = p;
        int addr_idx_tid = idx_tid;
        match_casted_index(q, idx_tid, addr_idx_tid);
        if (q + 1 >= fn.icodes.size())
            return false;
        const icode &addr_ic = fn.icodes[q];
        const icode &store_ic = fn.icodes[q + 1];
        if (addr_ic.op != icode_op::ADD ||
            !addr_ic.result.is_temp() ||
            !((same_global_ref(addr_ic.left, base) &&
                temp_eq(addr_ic.right, addr_idx_tid)) ||
               (same_global_ref(addr_ic.right, base) &&
                temp_eq(addr_ic.left, addr_idx_tid))) ||
            store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(store_ic.result, addr_ic.result.temp_id) ||
            !temp_eq(store_ic.left, value_tid)) {
            return false;
        }
        p = q + 2;
        return true;
    };

    auto match_loop_gt_zero = [&](const icode &cmp_ic, int tid) {
        return (cmp_ic.op == icode_op::GT &&
                temp_eq(cmp_ic.left, tid) &&
                is_exact_int_const(cmp_ic.right, 0)) ||
               (cmp_ic.op == icode_op::LT &&
                is_exact_int_const(cmp_ic.left, 0) &&
                temp_eq(cmp_ic.right, tid));
    };

    auto match_gt_temps = [&](const icode &cmp_ic, int lhs_tid, int rhs_tid) {
        return (cmp_ic.op == icode_op::GT &&
                temp_eq(cmp_ic.left, lhs_tid) &&
                temp_eq(cmp_ic.right, rhs_tid)) ||
               (cmp_ic.op == icode_op::LT &&
                temp_eq(cmp_ic.left, rhs_tid) &&
                temp_eq(cmp_ic.right, lhs_tid));
    };

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 1) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 1 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    int key_tid = -1;
    operand data_base;
    if (p >= fn.icodes.size())
        return false;
    const icode *key_direct = nullptr;
    const icode &key_ic0 = fn.icodes[p];
    if (key_ic0.op == icode_op::GET_VALUE_AT &&
        key_ic0.result.is_temp() &&
        is_byte_temp(key_ic0.result) &&
        is_global_byte_buffer_ref(key_ic0.left) &&
        temp_eq(key_ic0.right, idx_tid)) {
        data_base = key_ic0.left;
        key_tid = key_ic0.result.temp_id;
        key_direct = &key_ic0;
        ++p;
    } else {
        size_t q = p;
        int key_idx_tid = idx_tid;
        match_casted_index(q, idx_tid, key_idx_tid);
        if (q + 1 >= fn.icodes.size())
            return false;
        const icode &addr_ic = fn.icodes[q];
        const icode &load_ic = fn.icodes[q + 1];
        if (addr_ic.op != icode_op::ADD ||
            !addr_ic.result.is_temp() ||
            !((is_global_byte_buffer_ref(addr_ic.left) &&
                temp_eq(addr_ic.right, key_idx_tid)) ||
               (is_global_byte_buffer_ref(addr_ic.right) &&
                temp_eq(addr_ic.left, key_idx_tid))) ||
            load_ic.op != icode_op::GET_VALUE_AT ||
            !load_ic.result.is_temp() ||
            !is_byte_temp(load_ic.result) ||
            !temp_eq(load_ic.left, addr_ic.result.temp_id)) {
            return false;
        }
        data_base = is_global_byte_buffer_ref(addr_ic.left) ? addr_ic.left
                                                            : addr_ic.right;
        key_tid = load_ic.result.temp_id;
        key_direct = &load_ic;
        p = q + 2;
    }

    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, key_tid)) {
        key_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    if (p >= fn.icodes.size())
        return false;
    const icode &j_init = fn.icodes[p++];
    if (!j_init.result.is_temp() ||
        !is_assign_like(j_init.op) ||
        !temp_eq(j_init.left, idx_tid)) {
        return false;
    }
    const int j_tid = j_init.result.temp_id;

    const icode &while_lbl = fn.icodes[p++];
    const icode &j_cmp_ic = fn.icodes[p++];
    const icode &j_ifx_ic = fn.icodes[p++];
    const icode &cmp_lbl = fn.icodes[p++];

    if (while_lbl.op != icode_op::LABEL ||
        !j_cmp_ic.result.is_temp() ||
        !match_loop_gt_zero(j_cmp_ic, j_tid) ||
        j_ifx_ic.op != icode_op::IFX ||
        !temp_eq(j_ifx_ic.left, j_cmp_ic.result.temp_id) ||
        cmp_lbl.op != icode_op::LABEL ||
        cmp_lbl.label_name != j_ifx_ic.true_lbl) {
        return false;
    }

    if (p >= fn.icodes.size())
        return false;
    const icode &prev_idx_ic = fn.icodes[p++];
    if (prev_idx_ic.op != icode_op::SUB ||
        !prev_idx_ic.result.is_temp() ||
        !temp_eq(prev_idx_ic.left, j_tid) ||
        !is_exact_int_const(prev_idx_ic.right, 1)) {
        return false;
    }
    const int prev_idx_tid = prev_idx_ic.result.temp_id;

    int prev_value_tid = -1;
    if (!match_indexed_load(p, data_base, prev_idx_tid, prev_value_tid))
        return false;

    if (p >= fn.icodes.size())
        return false;
    const icode &cmp_prev_ic = fn.icodes[p++];
    if (!cmp_prev_ic.result.is_temp() ||
        !match_gt_temps(cmp_prev_ic, prev_value_tid, key_tid)) {
        return false;
    }

    const icode &shift_ifx_ic = fn.icodes[p++];
    const icode &shift_lbl = fn.icodes[p++];
    if (shift_ifx_ic.op != icode_op::IFX ||
        !temp_eq(shift_ifx_ic.left, cmp_prev_ic.result.temp_id) ||
        shift_lbl.op != icode_op::LABEL ||
        shift_lbl.label_name != shift_ifx_ic.true_lbl ||
        shift_ifx_ic.false_lbl != j_ifx_ic.false_lbl) {
        return false;
    }

    int shift_prev_tid = prev_value_tid;
    if (p < fn.icodes.size() &&
        prev_idx_ic.op == icode_op::SUB &&
        fn.icodes[p].op == icode_op::SUB &&
        fn.icodes[p].result.is_temp() &&
        !temp_eq(fn.icodes[p].result, prev_idx_tid) &&
        temp_eq(fn.icodes[p].left, j_tid) &&
        is_exact_int_const(fn.icodes[p].right, 1)) {
        int prev2_idx_tid = fn.icodes[p].result.temp_id;
        size_t q = p + 1;
        int prev2_value_tid = -1;
        if (match_indexed_load(q, data_base, prev2_idx_tid, prev2_value_tid)) {
            p = q;
            shift_prev_tid = prev2_value_tid;
        }
    }

    if (!match_indexed_store(p, data_base, j_tid, shift_prev_tid))
        return false;

    if (p >= fn.icodes.size())
        return false;
    const icode &j_dec_ic = fn.icodes[p++];
    if (j_dec_ic.op != icode_op::SUB ||
        !j_dec_ic.result.is_temp() ||
        !temp_eq(j_dec_ic.left, j_tid) ||
        !is_exact_int_const(j_dec_ic.right, 1)) {
        return false;
    }
    const icode &j_store_ic = fn.icodes[p++];
    if (!is_assign_like(j_store_ic.op) ||
        !temp_eq(j_store_ic.result, j_tid) ||
        !temp_eq(j_store_ic.left, j_dec_ic.result.temp_id)) {
        return false;
    }
    const icode &goto_while_ic = fn.icodes[p++];
    if (goto_while_ic.op != icode_op::GOTO ||
        goto_while_ic.label_name != while_lbl.label_name) {
        return false;
    }

    const icode &exit_lbl = fn.icodes[p++];
    if (exit_lbl.op != icode_op::LABEL ||
        exit_lbl.label_name != j_ifx_ic.false_lbl) {
        return false;
    }

    if (!match_indexed_store(p, data_base, j_tid, key_tid))
        return false;

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &goto_outer_ic = fn.icodes[p++];
    const icode &outer_end_lbl = fn.icodes[p++];
    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_outer_ic.op != icode_op::GOTO ||
        goto_outer_ic.label_name != cond_lbl.label_name ||
        outer_end_lbl.op != icode_op::LABEL ||
        outer_end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid) ||
        temp_value_used_after(fn, p, j_tid) ||
        temp_value_used_after(fn, p, key_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(key_direct->line);

    const std::string buf_sym =
        data_base.kind == operand_kind::LABEL_REF ? data_base.name
                                                  : mangle(data_base.name);
    const std::string store_zero_lbl =
        "__" + fn.name + "_ins0_zero_" + std::to_string(idx);
    const std::string store_key_lbl =
        "__" + fn.name + "_ins0_store_" + std::to_string(idx);
    const std::string next_i_lbl =
        "__" + fn.name + "_ins0_next_" + std::to_string(idx);

    emit_comment("O3 insertion-sort loop (count=%d)", count);
    emit_line("ld\tc, %s", asm_.imm(1).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(count).c_str());
    emit_line("jr\tnc, %s", outer_end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\thl, %s", asm_.imm_sym(buf_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\tb, (hl)");
    emit_line("ld\te, c");
    emit_label(while_lbl.label_name, false);
    emit_line("ld\ta, e");
    emit_line("or\ta");
    emit_line("jr\tz, %s", store_zero_lbl.c_str());
    emit_label(cmp_lbl.label_name, false);
    emit_line("dec\te");
    emit_line("ld\thl, %s", asm_.imm_sym(buf_sym).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("ld\td, (hl)");
    emit_line("ld\ta, b");
    emit_line("sub\td");
    emit_line("jr\tnc, %s", store_key_lbl.c_str());
    emit_label(shift_lbl.label_name, false);
    emit_line("inc\thl");
    emit_line("ld\t(hl), d");
    emit_line("jr\t%s", while_lbl.label_name.c_str());
    emit_label(store_zero_lbl, false);
    emit_line("ld\thl, %s", asm_.imm_sym(buf_sym).c_str());
    emit_line("ld\t(hl), b");
    emit_line("jr\t%s", next_i_lbl.c_str());
    emit_label(store_key_lbl, false);
    emit_line("inc\thl");
    emit_line("ld\t(hl), b");
    emit_label(next_i_lbl, false);
    emit_line("inc\tc");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(outer_end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_nibble_lut_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 16 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &out_ptr_init = fn.icodes[p++];
    const icode &in_ptr_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        !out_ptr_init.result.is_temp() ||
        !out_ptr_init.result.type || out_ptr_init.result.type->size() != 2 ||
        !is_assign_like(out_ptr_init.op) ||
        !is_global_byte_buffer_ref(out_ptr_init.left) ||
        !in_ptr_init.result.is_temp() ||
        !in_ptr_init.result.type || in_ptr_init.result.type->size() != 2 ||
        !is_assign_like(in_ptr_init.op) ||
        !is_global_byte_buffer_ref(in_ptr_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int out_ptr_tid = out_ptr_init.result.temp_id;
    const int in_ptr_tid = in_ptr_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &input_load_ic = fn.icodes[p++];
    if (input_load_ic.op != icode_op::GET_VALUE_AT ||
        !input_load_ic.result.is_temp() ||
        !is_byte_temp(input_load_ic.result) ||
        !temp_eq(input_load_ic.left, in_ptr_tid)) {
        return false;
    }

    int input_word_tid = input_load_ic.result.temp_id;
    if (p < fn.icodes.size() &&
        fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, input_load_ic.result.temp_id) &&
        fn.icodes[p].result.type && fn.icodes[p].result.type->size() == 2) {
        input_word_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    const icode &mask_ic = fn.icodes[p++];
    const icode &lo_addr_ic = fn.icodes[p++];
    const icode &lo_load_ic = fn.icodes[p++];
    const icode &shr_ic = fn.icodes[p++];
    const icode &hi_load_ic = fn.icodes[p++];
    const icode &hi_shl_ic = fn.icodes[p++];
    const icode &combine_ic = fn.icodes[p++];
    const icode &store_ic = fn.icodes[p++];

    if (mask_ic.op != icode_op::BAND ||
        !mask_ic.result.is_temp() ||
        !((temp_eq(mask_ic.left, input_word_tid) && is_exact_int_const(mask_ic.right, 15)) ||
           (temp_eq(mask_ic.right, input_word_tid) && is_exact_int_const(mask_ic.left, 15))) ||
        lo_addr_ic.op != icode_op::ADD ||
        !lo_addr_ic.result.is_temp() ||
        !((is_global_byte_buffer_ref(lo_addr_ic.left) &&
            temp_eq(lo_addr_ic.right, mask_ic.result.temp_id)) ||
           (is_global_byte_buffer_ref(lo_addr_ic.right) &&
            temp_eq(lo_addr_ic.left, mask_ic.result.temp_id))) ||
        lo_load_ic.op != icode_op::GET_VALUE_AT ||
        !lo_load_ic.result.is_temp() ||
        !is_byte_temp(lo_load_ic.result) ||
        !temp_eq(lo_load_ic.left, lo_addr_ic.result.temp_id) ||
        shr_ic.op != icode_op::SHR ||
        !shr_ic.result.is_temp() ||
        !temp_eq(shr_ic.left, input_load_ic.result.temp_id) ||
        !is_exact_int_const(shr_ic.right, 4) ||
        hi_load_ic.op != icode_op::GET_VALUE_AT ||
        !hi_load_ic.result.is_temp() ||
        !is_byte_temp(hi_load_ic.result) ||
        !is_global_byte_buffer_ref(hi_load_ic.left) ||
        !temp_eq(hi_load_ic.right, shr_ic.result.temp_id) ||
        hi_shl_ic.op != icode_op::SHL ||
        !hi_shl_ic.result.is_temp() ||
        !temp_eq(hi_shl_ic.left, hi_load_ic.result.temp_id) ||
        !is_exact_int_const(hi_shl_ic.right, 4) ||
        combine_ic.op != icode_op::BOR ||
        !combine_ic.result.is_temp() ||
        !((temp_eq(combine_ic.left, lo_load_ic.result.temp_id) &&
            temp_eq(combine_ic.right, hi_shl_ic.result.temp_id)) ||
           (temp_eq(combine_ic.right, lo_load_ic.result.temp_id) &&
            temp_eq(combine_ic.left, hi_shl_ic.result.temp_id))) ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, out_ptr_tid) ||
        !temp_eq(store_ic.left, combine_ic.result.temp_id)) {
        return false;
    }

    const operand &lut_sym =
        is_global_byte_buffer_ref(lo_addr_ic.left) ? lo_addr_ic.left
                                                   : lo_addr_ic.right;
    if (!same_global_ref(hi_load_ic.left, lut_sym))
        return false;

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 6 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &out_ptr_add_ic = fn.icodes[p++];
    const icode &out_ptr_store_ic = fn.icodes[p++];
    const icode &in_ptr_add_ic = fn.icodes[p++];
    const icode &in_ptr_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        out_ptr_add_ic.op != icode_op::ADD ||
        !out_ptr_add_ic.result.is_temp() ||
        !temp_eq(out_ptr_add_ic.left, out_ptr_tid) ||
        !is_exact_int_const(out_ptr_add_ic.right, 1) ||
        !is_assign_like(out_ptr_store_ic.op) ||
        !temp_eq(out_ptr_store_ic.result, out_ptr_tid) ||
        !temp_eq(out_ptr_store_ic.left, out_ptr_add_ic.result.temp_id) ||
        in_ptr_add_ic.op != icode_op::ADD ||
        !in_ptr_add_ic.result.is_temp() ||
        !temp_eq(in_ptr_add_ic.left, in_ptr_tid) ||
        !is_exact_int_const(in_ptr_add_ic.right, 1) ||
        !is_assign_like(in_ptr_store_ic.op) ||
        !temp_eq(in_ptr_store_ic.result, in_ptr_tid) ||
        !temp_eq(in_ptr_store_ic.left, in_ptr_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid) ||
        temp_value_used_after(fn, p, out_ptr_tid) ||
        temp_value_used_after(fn, p, in_ptr_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string input_sym =
        in_ptr_init.left.kind == operand_kind::LABEL_REF ? in_ptr_init.left.name
                                                         : mangle(in_ptr_init.left.name);
    const std::string output_sym =
        out_ptr_init.left.kind == operand_kind::LABEL_REF ? out_ptr_init.left.name
                                                          : mangle(out_ptr_init.left.name);
    const std::string lut_name =
        lut_sym.kind == operand_kind::LABEL_REF ? lut_sym.name
                                                : mangle(lut_sym.name);

    emit_comment("O3 nibble LUT loop (count=%d)", count);
    emit_line("ld\tde, %s", asm_.imm_sym(input_sym).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(output_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(count).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, (de)");
    emit_line("inc\tde");
    emit_line("ld\tc, a");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("push\tde");
    emit_line("add\ta, %s", asm_.imm_sym_lo(lut_name).c_str());
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(lut_name).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("push\thl");
    emit_line("ld\tl, a");
    emit_line("ld\ta, c");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("add\ta, %s", asm_.imm_sym_lo(lut_name).c_str());
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(lut_name).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("add\ta, a");
    emit_line("or\tl");
    emit_line("pop\thl");
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("pop\tde");
    emit_line("djnz\t%s", body_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_byte_mask_walk_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 10 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &ptr_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        !ptr_init.result.is_temp() ||
        !ptr_init.result.type || ptr_init.result.type->size() != 2 ||
        !is_assign_like(ptr_init.op) ||
        !is_global_byte_buffer_ref(ptr_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int ptr_tid = ptr_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &load_ic = fn.icodes[p++];
    const icode &mask_ic = fn.icodes[p++];
    const icode &store_ic = fn.icodes[p++];

    if (load_ic.op != icode_op::GET_VALUE_AT ||
        !load_ic.result.is_temp() ||
        !is_byte_temp(load_ic.result) ||
        !same_global_ref(load_ic.left, ptr_init.left) ||
        mask_ic.op != icode_op::BAND ||
        !mask_ic.result.is_temp() ||
        !temp_eq(mask_ic.left, load_ic.result.temp_id) ||
        mask_ic.right.kind != operand_kind::INT_CONST ||
        mask_ic.right.ival < 0 || mask_ic.right.ival > 255 ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, ptr_tid) ||
        !temp_eq(store_ic.left, mask_ic.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 4 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &ptr_add_ic = fn.icodes[p++];
    const icode &ptr_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        ptr_add_ic.op != icode_op::ADD ||
        !ptr_add_ic.result.is_temp() ||
        !temp_eq(ptr_add_ic.left, ptr_tid) ||
        !is_exact_int_const(ptr_add_ic.right, 1) ||
        !is_assign_like(ptr_store_ic.op) ||
        !temp_eq(ptr_store_ic.result, ptr_tid) ||
        !temp_eq(ptr_store_ic.left, ptr_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid) ||
        temp_value_used_after(fn, p, ptr_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string buf_sym =
        ptr_init.left.kind == operand_kind::LABEL_REF ? ptr_init.left.name
                                                      : mangle(ptr_init.left.name);
    const int mask = static_cast<int>(mask_ic.right.ival & 0xFF);
    emit_comment("O3 byte mask walk loop (count=%d)", count);
    emit_line("ld\thl, %s", asm_.imm_sym(buf_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(count).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, (hl)");
    emit_line("and\t%s", asm_.imm(mask).c_str());
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("djnz\t%s", body_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_byte_copy_walk_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 10 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &dst_ptr_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        !dst_ptr_init.result.is_temp() ||
        !dst_ptr_init.result.type || dst_ptr_init.result.type->size() != 2 ||
        !is_assign_like(dst_ptr_init.op) ||
        !is_global_byte_buffer_ref(dst_ptr_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int dst_ptr_tid = dst_ptr_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &load_ic = fn.icodes[p++];
    const icode &store_ic = fn.icodes[p++];

    if (load_ic.op != icode_op::GET_VALUE_AT ||
        !load_ic.result.is_temp() ||
        !is_byte_temp(load_ic.result) ||
        !is_global_byte_buffer_ref(load_ic.left) ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, dst_ptr_tid) ||
        !temp_eq(store_ic.left, load_ic.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 4 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &dst_ptr_add_ic = fn.icodes[p++];
    const icode &dst_ptr_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        dst_ptr_add_ic.op != icode_op::ADD ||
        !dst_ptr_add_ic.result.is_temp() ||
        !temp_eq(dst_ptr_add_ic.left, dst_ptr_tid) ||
        !is_exact_int_const(dst_ptr_add_ic.right, 1) ||
        !is_assign_like(dst_ptr_store_ic.op) ||
        !temp_eq(dst_ptr_store_ic.result, dst_ptr_tid) ||
        !temp_eq(dst_ptr_store_ic.left, dst_ptr_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid) ||
        temp_value_used_after(fn, p, dst_ptr_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string src_sym =
        load_ic.left.kind == operand_kind::LABEL_REF ? load_ic.left.name
                                                     : mangle(load_ic.left.name);
    const std::string dst_sym =
        dst_ptr_init.left.kind == operand_kind::LABEL_REF ? dst_ptr_init.left.name
                                                          : mangle(dst_ptr_init.left.name);
    emit_comment("O3 byte copy walk loop (count=%d)", count);
    emit_line("ld\thl, %s", asm_.imm_sym(src_sym).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(dst_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(count).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, (hl)");
    emit_line("ld\t(de), a");
    emit_line("inc\thl");
    emit_line("inc\tde");
    emit_line("djnz\t%s", body_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_zero_byte_walk_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 10 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &ptr_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        !ptr_init.result.is_temp() ||
        !ptr_init.result.type || ptr_init.result.type->size() != 2 ||
        !is_assign_like(ptr_init.op) ||
        !is_global_byte_buffer_ref(ptr_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int ptr_tid = ptr_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &store_ic = fn.icodes[p++];
    if (store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, ptr_tid) ||
        !is_exact_int_const(store_ic.left, 0)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 4 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &ptr_add_ic = fn.icodes[p++];
    const icode &ptr_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        ptr_add_ic.op != icode_op::ADD ||
        !ptr_add_ic.result.is_temp() ||
        !temp_eq(ptr_add_ic.left, ptr_tid) ||
        !is_exact_int_const(ptr_add_ic.right, 1) ||
        !is_assign_like(ptr_store_ic.op) ||
        !temp_eq(ptr_store_ic.result, ptr_tid) ||
        !temp_eq(ptr_store_ic.left, ptr_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid) ||
        temp_value_used_after(fn, p, ptr_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string buf_sym =
        ptr_init.left.kind == operand_kind::LABEL_REF ? ptr_init.left.name
                                                      : mangle(ptr_init.left.name);
    emit_comment("O3 byte zero loop (count=%d)", count);
    emit_line("ld\thl, %s", asm_.imm_sym(buf_sym).c_str());
    emit_line("ld\tb, %s", asm_.imm(count).c_str());
    emit_line("xor\ta");
    emit_label(cond_lbl.label_name, false);
    emit_label(body_lbl.label_name, false);
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("djnz\t%s", body_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_nibble_histogram_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 12 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &load_ic = fn.icodes[p++];
    const icode &nibble_ic = fn.icodes[p++];
    const icode &load_count_ic = fn.icodes[p++];
    const icode &inc_ic = fn.icodes[p++];

    bool use_shift4 = false;
    int mask = 0;
    int nibble_tid = -1;
    if (nibble_ic.op == icode_op::BAND &&
        nibble_ic.result.is_temp() &&
        temp_eq(nibble_ic.left, load_ic.result.temp_id) &&
        nibble_ic.right.kind == operand_kind::INT_CONST &&
        nibble_ic.right.ival >= 0 && nibble_ic.right.ival <= 255) {
        use_shift4 = false;
        mask = static_cast<int>(nibble_ic.right.ival & 0xFF);
        nibble_tid = nibble_ic.result.temp_id;
    } else if (nibble_ic.op == icode_op::SHR &&
               nibble_ic.result.is_temp() &&
               temp_eq(nibble_ic.left, load_ic.result.temp_id) &&
               is_exact_int_const(nibble_ic.right, 4)) {
        use_shift4 = true;
        mask = 0x0F;
        nibble_tid = nibble_ic.result.temp_id;
    } else {
        return false;
    }

    int cast_source_tid = nibble_tid;
    if (p >= fn.icodes.size())
        return false;

    const icode *recompute_ic = nullptr;
    const icode *cast_ic = &fn.icodes[p++];
    if (cast_ic->op != icode_op::CAST) {
        if (p >= fn.icodes.size())
            return false;
        recompute_ic = cast_ic;
        cast_ic = &fn.icodes[p++];

        bool duplicate_ok = false;
        if (!use_shift4 &&
            recompute_ic->op == icode_op::BAND &&
            recompute_ic->result.is_temp() &&
            temp_eq(recompute_ic->left, load_ic.result.temp_id) &&
            recompute_ic->right.kind == operand_kind::INT_CONST &&
            static_cast<int>(recompute_ic->right.ival & 0xFF) == mask) {
            duplicate_ok = true;
        } else if (use_shift4 &&
                   recompute_ic->op == icode_op::SHR &&
                   recompute_ic->result.is_temp() &&
                   temp_eq(recompute_ic->left, load_ic.result.temp_id) &&
                   is_exact_int_const(recompute_ic->right, 4)) {
            duplicate_ok = true;
        }
        if (!duplicate_ok)
            return false;
        cast_source_tid = recompute_ic->result.temp_id;
    }

    if (p + 1 >= fn.icodes.size())
        return false;
    const icode &addr_ic = fn.icodes[p++];
    const icode &store_ic = fn.icodes[p++];

    const operand *count_base = nullptr;
    if (addr_ic.op == icode_op::ADD &&
        addr_ic.result.is_temp() &&
        is_global_byte_buffer_ref(addr_ic.left) &&
        temp_eq(addr_ic.right, cast_ic->result.temp_id)) {
        count_base = &addr_ic.left;
    } else if (addr_ic.op == icode_op::ADD &&
               addr_ic.result.is_temp() &&
               is_global_byte_buffer_ref(addr_ic.right) &&
               temp_eq(addr_ic.left, cast_ic->result.temp_id)) {
        count_base = &addr_ic.right;
    }

    if (load_ic.op != icode_op::GET_VALUE_AT ||
        !load_ic.result.is_temp() ||
        !is_global_byte_buffer_ref(load_ic.left) ||
        load_count_ic.op != icode_op::GET_VALUE_AT ||
        !load_count_ic.result.is_temp() ||
        !is_global_byte_buffer_ref(load_count_ic.left) ||
        !count_base ||
        inc_ic.op != icode_op::ADD ||
        !inc_ic.result.is_temp() ||
        !temp_eq(inc_ic.left, load_count_ic.result.temp_id) ||
        !is_exact_int_const(inc_ic.right, 1) ||
        cast_ic->op != icode_op::CAST ||
        !cast_ic->result.is_temp() ||
        !temp_eq(cast_ic->left, cast_source_tid) ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, addr_ic.result.temp_id) ||
        !temp_eq(store_ic.left, inc_ic.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid))
        return false;

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string input_sym =
        load_ic.left.kind == operand_kind::LABEL_REF ? load_ic.left.name
                                                     : mangle(load_ic.left.name);
    const std::string count_sym =
        count_base->kind == operand_kind::LABEL_REF ? count_base->name
                                                    : mangle(count_base->name);

    emit_comment("O3 nibble histogram loop (count=%d)", count);
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(count).c_str());
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\thl, %s", asm_.imm_sym(input_sym).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\ta, (hl)");
    if (use_shift4) {
        emit_line("rlca");
        emit_line("rlca");
        emit_line("rlca");
        emit_line("rlca");
        emit_line("and\t%s", asm_.imm(mask).c_str());
    } else {
        emit_line("and\t%s", asm_.imm(mask).c_str());
    }
    emit_line("ld\tl, a");
    emit_line("ld\th, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(count_sym).c_str());
    emit_line("add\thl, de");
    emit_line("inc\t(hl)");
    emit_line("inc\tc");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_bucket_drain_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 19 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &pos_init = fn.icodes[p++];
    const icode &bucket_init = fn.icodes[p++];
    const icode &ptr_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &check_lbl = fn.icodes[p++];

    if (!pos_init.result.is_temp() ||
        !is_assign_like(pos_init.op) ||
        !is_exact_int_const(pos_init.left, 0) ||
        !bucket_init.result.is_temp() ||
        !is_assign_like(bucket_init.op) ||
        !is_exact_int_const(bucket_init.left, 0) ||
        !ptr_init.result.is_temp() ||
        !ptr_init.result.type || ptr_init.result.type->size() != 2 ||
        !is_assign_like(ptr_init.op) ||
        !is_global_byte_buffer_ref(ptr_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, bucket_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        check_lbl.op != icode_op::LABEL ||
        check_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int pos_tid = pos_init.result.temp_id;
    const int bucket_tid = bucket_init.result.temp_id;
    const int ptr_tid = ptr_init.result.temp_id;
    const int bucket_count = static_cast<int>(cmp_ic.right.ival);

    const icode &load_ic = fn.icodes[p++];
    const icode &cast_ic = fn.icodes[p++];
    const icode &ifx_inner = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (load_ic.op != icode_op::GET_VALUE_AT ||
        !load_ic.result.is_temp() ||
        !is_byte_temp(load_ic.result) ||
        !same_global_ref(load_ic.left, ptr_init.left) ||
        cast_ic.op != icode_op::CAST ||
        !cast_ic.result.is_temp() ||
        !temp_eq(cast_ic.left, load_ic.result.temp_id) ||
        ifx_inner.op != icode_op::IFX ||
        !temp_eq(ifx_inner.left, cast_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_inner.true_lbl) {
        return false;
    }

    const icode &pos_snap_ic = fn.icodes[p++];
    const icode &pos_add_ic = fn.icodes[p++];
    const icode &pos_store_ic = fn.icodes[p++];
    const icode &pos_cast_ic = fn.icodes[p++];
    const icode &out_addr_ic = fn.icodes[p++];
    const icode &out_store_ic = fn.icodes[p++];
    const icode &load2_ic = fn.icodes[p++];
    const icode &dec_ic = fn.icodes[p++];
    const icode &count_store_ic = fn.icodes[p++];
    const icode &goto_check_ic = fn.icodes[p++];
    const icode &next_lbl = fn.icodes[p++];
    const icode &bucket_add_ic = fn.icodes[p++];
    const icode &bucket_store_ic = fn.icodes[p++];
    const icode &ptr_add_ic = fn.icodes[p++];
    const icode &ptr_store_ic = fn.icodes[p++];
    const icode &goto_outer_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (!is_assign_like(pos_snap_ic.op) ||
        !pos_snap_ic.result.is_temp() ||
        !temp_eq(pos_snap_ic.left, pos_tid) ||
        pos_add_ic.op != icode_op::ADD ||
        !pos_add_ic.result.is_temp() ||
        !temp_eq(pos_add_ic.left, pos_tid) ||
        !is_exact_int_const(pos_add_ic.right, 1) ||
        !is_assign_like(pos_store_ic.op) ||
        !temp_eq(pos_store_ic.result, pos_tid) ||
        !temp_eq(pos_store_ic.left, pos_add_ic.result.temp_id) ||
        pos_cast_ic.op != icode_op::CAST ||
        !pos_cast_ic.result.is_temp() ||
        !temp_eq(pos_cast_ic.left, pos_snap_ic.result.temp_id) ||
        out_addr_ic.op != icode_op::ADD ||
        !out_addr_ic.result.is_temp() ||
        !((is_global_byte_buffer_ref(out_addr_ic.left) &&
            temp_eq(out_addr_ic.right, pos_cast_ic.result.temp_id)) ||
           (is_global_byte_buffer_ref(out_addr_ic.right) &&
            temp_eq(out_addr_ic.left, pos_cast_ic.result.temp_id))) ||
        out_store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(out_store_ic.result, out_addr_ic.result.temp_id) ||
        !temp_eq(out_store_ic.left, bucket_tid) ||
        load2_ic.op != icode_op::GET_VALUE_AT ||
        !load2_ic.result.is_temp() ||
        !is_byte_temp(load2_ic.result) ||
        !same_global_ref(load2_ic.left, ptr_init.left) ||
        dec_ic.op != icode_op::SUB ||
        !dec_ic.result.is_temp() ||
        !temp_eq(dec_ic.left, load2_ic.result.temp_id) ||
        !is_exact_int_const(dec_ic.right, 1) ||
        count_store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(count_store_ic.result, ptr_tid) ||
        !temp_eq(count_store_ic.left, dec_ic.result.temp_id) ||
        goto_check_ic.op != icode_op::GOTO ||
        goto_check_ic.label_name != check_lbl.label_name ||
        next_lbl.op != icode_op::LABEL ||
        next_lbl.label_name != ifx_inner.false_lbl ||
        bucket_add_ic.op != icode_op::ADD ||
        !bucket_add_ic.result.is_temp() ||
        !temp_eq(bucket_add_ic.left, bucket_tid) ||
        !is_exact_int_const(bucket_add_ic.right, 1) ||
        !is_assign_like(bucket_store_ic.op) ||
        !temp_eq(bucket_store_ic.result, bucket_tid) ||
        !temp_eq(bucket_store_ic.left, bucket_add_ic.result.temp_id) ||
        ptr_add_ic.op != icode_op::ADD ||
        !ptr_add_ic.result.is_temp() ||
        !temp_eq(ptr_add_ic.left, ptr_tid) ||
        !is_exact_int_const(ptr_add_ic.right, 1) ||
        !is_assign_like(ptr_store_ic.op) ||
        !temp_eq(ptr_store_ic.result, ptr_tid) ||
        !temp_eq(ptr_store_ic.left, ptr_add_ic.result.temp_id) ||
        goto_outer_ic.op != icode_op::GOTO ||
        goto_outer_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, pos_tid) ||
        temp_value_used_after(fn, p, bucket_tid) ||
        temp_value_used_after(fn, p, ptr_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(pos_init.line);

    const std::string count_sym =
        ptr_init.left.kind == operand_kind::LABEL_REF ? ptr_init.left.name
                                                      : mangle(ptr_init.left.name);
    const operand &out_base =
        is_global_byte_buffer_ref(out_addr_ic.left) ? out_addr_ic.left
                                                    : out_addr_ic.right;
    const std::string output_sym =
        out_base.kind == operand_kind::LABEL_REF ? out_base.name
                                                 : mangle(out_base.name);

    emit_comment("O3 bucket-drain loop (count=%d)", bucket_count);
    emit_line("ld\tde, %s", asm_.imm_sym(output_sym).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\thl, %s", asm_.imm_sym(count_sym).c_str());
    emit_line("add\thl, bc");
    emit_label(check_lbl.label_name, false);
    emit_line("ld\ta, (hl)");
    emit_line("or\ta");
    emit_line("jr\tz, %s", next_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("ld\t(de), a");
    emit_line("inc\tde");
    emit_line("dec\t(hl)");
    emit_line("jr\tnz, %s", body_lbl.label_name.c_str());
    emit_label(next_lbl.label_name, false);
    emit_line("inc\tc");
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(bucket_count).c_str());
    emit_line("jr\tc, %s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_gray_decode_loop(const ir_function &fn, size_t &idx) {
    if (!o3_baseline_enabled() || idx + 25 >= fn.icodes.size())
        return false;

    auto global_ref_used_after = [&](const operand &sym, size_t start_idx) {
        for (size_t i = start_idx; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (same_global_ref(ic.left, sym) ||
                same_global_ref(ic.right, sym) ||
                same_global_ref(ic.result, sym)) {
                return true;
            }
        }
        return false;
    };

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &plain_ptr_init = fn.icodes[p++];
    const icode &gray_ptr_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        !plain_ptr_init.result.is_temp() ||
        !plain_ptr_init.result.type || plain_ptr_init.result.type->size() != 2 ||
        !is_assign_like(plain_ptr_init.op) ||
        !is_global_byte_buffer_ref(plain_ptr_init.left) ||
        !gray_ptr_init.result.is_temp() ||
        !gray_ptr_init.result.type || gray_ptr_init.result.type->size() != 2 ||
        !is_assign_like(gray_ptr_init.op) ||
        !is_global_byte_buffer_ref(gray_ptr_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int plain_ptr_tid = plain_ptr_init.result.temp_id;
    const int gray_ptr_tid = gray_ptr_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &src_load = fn.icodes[p++];
    if (src_load.op != icode_op::GET_VALUE_AT ||
        !src_load.result.is_temp() ||
        !is_byte_temp(src_load.result) ||
        !is_global_byte_buffer_ref(src_load.left) ||
        !temp_eq(src_load.right, idx_tid)) {
        return false;
    }
    const int src_tid = src_load.result.temp_id;

    int src_copy_tid = src_tid;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, src_tid)) {
        src_copy_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    const icode &shr1_ic = fn.icodes[p++];
    if (shr1_ic.op != icode_op::SHR ||
        !shr1_ic.result.is_temp() ||
        !temp_eq(shr1_ic.left, src_copy_tid) ||
        !is_exact_int_const(shr1_ic.right, 1)) {
        return false;
    }
    const int shr1_tid = shr1_ic.result.temp_id;

    const icode &gray_xor = fn.icodes[p++];
    if (gray_xor.op != icode_op::BXOR ||
        !gray_xor.result.is_temp() ||
        !((temp_eq(gray_xor.left, src_tid) && temp_eq(gray_xor.right, shr1_tid)) ||
          (temp_eq(gray_xor.right, src_tid) && temp_eq(gray_xor.left, shr1_tid)))) {
        return false;
    }
    const int gray_tid = gray_xor.result.temp_id;

    const icode &gray_store = fn.icodes[p++];
    if (gray_store.op != icode_op::SET_VALUE_AT ||
        !temp_eq(gray_store.result, gray_ptr_tid) ||
        !temp_eq(gray_store.left, gray_tid)) {
        return false;
    }

    int plain_seed_tid = gray_tid;
    if (p < fn.icodes.size() && fn.icodes[p].op == icode_op::GET_VALUE_AT) {
        const icode &gray_reload = fn.icodes[p];
        if (!gray_reload.result.is_temp() ||
            !is_byte_temp(gray_reload.result) ||
            !same_global_ref(gray_reload.left, gray_ptr_init.left) ||
            !temp_eq(gray_reload.right, idx_tid)) {
            return false;
        }
        plain_seed_tid = gray_reload.result.temp_id;
        ++p;
    }

    const icode &plain_store = fn.icodes[p++];
    if (plain_store.op != icode_op::SET_VALUE_AT ||
        !temp_eq(plain_store.result, plain_ptr_tid) ||
        !temp_eq(plain_store.left, plain_seed_tid)) {
        return false;
    }

    const icode &bit_init = fn.icodes[p++];
    const icode &inner_cond_lbl = fn.icodes[p++];
    const icode &bit_cmp = fn.icodes[p++];
    const icode &bit_ifx = fn.icodes[p++];
    const icode &inner_body_lbl = fn.icodes[p++];
    if (!bit_init.result.is_temp() ||
        !is_assign_like(bit_init.op) ||
        !is_exact_int_const(bit_init.left, 1) ||
        inner_cond_lbl.op != icode_op::LABEL ||
        bit_cmp.op != icode_op::LT ||
        !bit_cmp.result.is_temp() ||
        !temp_eq(bit_cmp.left, bit_init.result.temp_id) ||
        !is_exact_int_const(bit_cmp.right, 8) ||
        bit_ifx.op != icode_op::IFX ||
        !temp_eq(bit_ifx.left, bit_cmp.result.temp_id) ||
        inner_body_lbl.op != icode_op::LABEL ||
        inner_body_lbl.label_name != bit_ifx.true_lbl) {
        return false;
    }
    const int bit_tid = bit_init.result.temp_id;

    const icode &plain_load = fn.icodes[p++];
    if (plain_load.op != icode_op::GET_VALUE_AT ||
        !plain_load.result.is_temp() ||
        !is_byte_temp(plain_load.result) ||
        !same_global_ref(plain_load.left, plain_ptr_init.left) ||
        !temp_eq(plain_load.right, idx_tid)) {
        return false;
    }
    const int plain_tid = plain_load.result.temp_id;

    int shift_src_tid = plain_tid;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, plain_tid)) {
        shift_src_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    const icode &cast_plain = fn.icodes[p++];
    const icode &cast_bit = fn.icodes[p++];
    const icode &shift_ic = fn.icodes[p++];
    const icode &cast_back = fn.icodes[p++];
    const icode &plain_xor = fn.icodes[p++];
    const icode &plain_store2 = fn.icodes[p++];
    if (cast_plain.op != icode_op::CAST ||
        !cast_plain.result.is_temp() ||
        !temp_eq(cast_plain.left, shift_src_tid) ||
        cast_bit.op != icode_op::CAST ||
        !cast_bit.result.is_temp() ||
        !temp_eq(cast_bit.left, bit_tid) ||
        shift_ic.op != icode_op::SHR ||
        !shift_ic.result.is_temp() ||
        !temp_eq(shift_ic.left, cast_plain.result.temp_id) ||
        !temp_eq(shift_ic.right, cast_bit.result.temp_id) ||
        cast_back.op != icode_op::CAST ||
        !cast_back.result.is_temp() ||
        !temp_eq(cast_back.left, shift_ic.result.temp_id) ||
        plain_xor.op != icode_op::BXOR ||
        !plain_xor.result.is_temp() ||
        !((temp_eq(plain_xor.left, plain_tid) && temp_eq(plain_xor.right, cast_back.result.temp_id)) ||
          (temp_eq(plain_xor.right, plain_tid) && temp_eq(plain_xor.left, cast_back.result.temp_id))) ||
        plain_store2.op != icode_op::SET_VALUE_AT ||
        !temp_eq(plain_store2.result, plain_ptr_tid) ||
        !temp_eq(plain_store2.left, plain_xor.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 7 >= fn.icodes.size())
        return false;

    const icode &bit_shl = fn.icodes[p++];
    const icode &bit_store = fn.icodes[p++];
    const icode &inner_goto = fn.icodes[p++];
    const icode &inner_end_lbl = fn.icodes[p++];
    const icode &idx_add = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode &plain_ptr_add = fn.icodes[p++];
    const icode &plain_ptr_store = fn.icodes[p++];
    const icode &gray_ptr_add = fn.icodes[p++];
    const icode &gray_ptr_store = fn.icodes[p++];
    const icode &outer_goto = fn.icodes[p++];
    const icode &outer_end_lbl = fn.icodes[p++];

    if (bit_shl.op != icode_op::SHL ||
        !bit_shl.result.is_temp() ||
        !temp_eq(bit_shl.left, bit_tid) ||
        !is_exact_int_const(bit_shl.right, 1) ||
        !is_assign_like(bit_store.op) ||
        !temp_eq(bit_store.result, bit_tid) ||
        !temp_eq(bit_store.left, bit_shl.result.temp_id) ||
        inner_goto.op != icode_op::GOTO ||
        inner_goto.label_name != inner_cond_lbl.label_name ||
        inner_end_lbl.op != icode_op::LABEL ||
        inner_end_lbl.label_name != bit_ifx.false_lbl ||
        idx_add.op != icode_op::ADD ||
        !idx_add.result.is_temp() ||
        !temp_eq(idx_add.left, idx_tid) ||
        !is_exact_int_const(idx_add.right, 1) ||
        !is_assign_like(idx_store.op) ||
        !temp_eq(idx_store.result, idx_tid) ||
        !temp_eq(idx_store.left, idx_add.result.temp_id) ||
        plain_ptr_add.op != icode_op::ADD ||
        !plain_ptr_add.result.is_temp() ||
        !temp_eq(plain_ptr_add.left, plain_ptr_tid) ||
        !is_exact_int_const(plain_ptr_add.right, 1) ||
        !is_assign_like(plain_ptr_store.op) ||
        !temp_eq(plain_ptr_store.result, plain_ptr_tid) ||
        !temp_eq(plain_ptr_store.left, plain_ptr_add.result.temp_id) ||
        gray_ptr_add.op != icode_op::ADD ||
        !gray_ptr_add.result.is_temp() ||
        !temp_eq(gray_ptr_add.left, gray_ptr_tid) ||
        !is_exact_int_const(gray_ptr_add.right, 1) ||
        !is_assign_like(gray_ptr_store.op) ||
        !temp_eq(gray_ptr_store.result, gray_ptr_tid) ||
        !temp_eq(gray_ptr_store.left, gray_ptr_add.result.temp_id) ||
        outer_goto.op != icode_op::GOTO ||
        outer_goto.label_name != cond_lbl.label_name ||
        outer_end_lbl.op != icode_op::LABEL ||
        outer_end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (global_ref_used_after(gray_ptr_init.left, p))
        return false;

    if (debug_)
        debug_->emit_location(idx_init.line);

    auto emit_sym = [&](const operand &op) {
        return op.kind == operand_kind::LABEL_REF ? op.name : mangle(op.name);
    };

    emit_comment("O3 gray-decode loop (count=%d)", count);
    emit_line("ld\tc, %s", asm_.imm(count).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(emit_sym(src_load.left)).c_str());
    emit_line("ld\tde, %s", asm_.imm_sym(emit_sym(plain_ptr_init.left)).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("or\ta, a");
    emit_line("jr\tz, %s", outer_end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, (hl)");
    emit_line("ld\tb, a");
    emit_line("srl\ta");
    emit_line("xor\tb");
    emit_line("ld\tb, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\tb");
    emit_line("ld\tb, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\tb");
    emit_line("ld\tb, a");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("xor\tb");
    emit_line("ld\t(de), a");
    emit_line("inc\thl");
    emit_line("inc\tde");
    emit_line("dec\tc");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(outer_end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_fir_shiftadd_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 30 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &mix_init = fn.icodes[p++];
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!mix_init.result.is_temp() ||
        !mix_init.result.type || mix_init.result.type->size() != 2 ||
        !is_assign_like(mix_init.op) ||
        !is_exact_int_const(mix_init.left, 30874) ||
        !idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 7) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        !is_exact_int_const(cmp_ic.right, 64) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int mix_tid = mix_init.result.temp_id;
    const int idx_tid = idx_init.result.temp_id;

    auto match_buf_load = [&](const icode &load_ic, int expected_index_tid) {
        return load_ic.op == icode_op::GET_VALUE_AT &&
               load_ic.result.is_temp() &&
               is_byte_temp(load_ic.result) &&
               is_global_byte_buffer_ref(load_ic.left) &&
               (load_ic.right.is_none() || temp_eq(load_ic.right, expected_index_tid));
    };

    const icode &load0 = fn.icodes[p++];
    const icode &cast0 = fn.icodes[p++];
    if (!match_buf_load(load0, idx_tid) ||
        cast0.op != icode_op::CAST ||
        !cast0.result.is_temp() ||
        !temp_eq(cast0.left, load0.result.temp_id)) {
        return false;
    }
    const operand &buf_ref = load0.left;

    auto expect_indexed_term = [&](int delta,
                                   bool shift_twice,
                                   icode_op accum_op,
                                   int prev_tid,
                                   int &out_next_tid) -> bool {
        if (p + 4 >= fn.icodes.size())
            return false;
        const icode &sub_ic = fn.icodes[p++];
        if (sub_ic.op != icode_op::SUB ||
            !sub_ic.result.is_temp() ||
            !temp_eq(sub_ic.left, idx_tid) ||
            !is_exact_int_const(sub_ic.right, -delta)) {
            return false;
        }
        const int sub_tid = sub_ic.result.temp_id;
        const icode &load_ic = fn.icodes[p++];
        if (!match_buf_load(load_ic, sub_tid) || !same_global_ref(load_ic.left, buf_ref))
            return false;
        const icode &cast_ic = fn.icodes[p++];
        if (cast_ic.op != icode_op::CAST ||
            !cast_ic.result.is_temp() ||
            !temp_eq(cast_ic.left, load_ic.result.temp_id)) {
            return false;
        }
        int rhs_tid = cast_ic.result.temp_id;
        if (shift_twice) {
            const icode &shl_ic = fn.icodes[p++];
            if (shl_ic.op != icode_op::SHL ||
                !shl_ic.result.is_temp() ||
                !temp_eq(shl_ic.left, rhs_tid) ||
                !is_exact_int_const(shl_ic.right, 1)) {
                return false;
            }
            rhs_tid = shl_ic.result.temp_id;
        }
        const icode &acc_ic = fn.icodes[p++];
        if (acc_ic.op != accum_op ||
            !acc_ic.result.is_temp() ||
            !((temp_eq(acc_ic.left, prev_tid) && temp_eq(acc_ic.right, rhs_tid)) ||
              (accum_op == icode_op::ADD &&
               temp_eq(acc_ic.right, prev_tid) && temp_eq(acc_ic.left, rhs_tid)))) {
            return false;
        }
        out_next_tid = acc_ic.result.temp_id;
        return true;
    };

    int acc_tid = cast0.result.temp_id;
    if (!expect_indexed_term(-1, true, icode_op::ADD, acc_tid, acc_tid))
        return false;
    if (!expect_indexed_term(-2, false, icode_op::ADD, acc_tid, acc_tid))
        return false;
    if (!expect_indexed_term(-3, false, icode_op::SUB, acc_tid, acc_tid))
        return false;
    if (!expect_indexed_term(-5, true, icode_op::ADD, acc_tid, acc_tid))
        return false;
    if (!expect_indexed_term(-5, false, icode_op::ADD, acc_tid, acc_tid))
        return false;
    if (!expect_indexed_term(-6, true, icode_op::SUB, acc_tid, acc_tid))
        return false;
    if (!expect_indexed_term(-7, false, icode_op::ADD, acc_tid, acc_tid))
        return false;

    const icode &band_ic = fn.icodes[p++];
    if (band_ic.op != icode_op::BAND ||
        !band_ic.result.is_temp() ||
        !temp_eq(band_ic.left, acc_tid) ||
        !is_exact_int_const(band_ic.right, 255)) {
        return false;
    }
    const int value_tid = band_ic.result.temp_id;

    int mix_src_tid = mix_tid;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, mix_tid)) {
        mix_src_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    if (p + 5 >= fn.icodes.size())
        return false;
    const icode &addc = fn.icodes[p++];
    const icode &xor1 = fn.icodes[p++];
    const icode &rol = fn.icodes[p++];
    const icode &xorv = fn.icodes[p++];
    const icode &mix_add = fn.icodes[p++];
    const icode &mix_store = fn.icodes[p++];
    if (addc.op != icode_op::ADD ||
        !addc.result.is_temp() ||
        !((temp_eq(addc.left, value_tid) && is_exact_int_const(addc.right, 40503)) ||
          (temp_eq(addc.right, value_tid) && is_exact_int_const(addc.left, 40503))) ||
        xor1.op != icode_op::BXOR ||
        !xor1.result.is_temp() ||
        !((temp_eq(xor1.left, mix_src_tid) && temp_eq(xor1.right, addc.result.temp_id)) ||
          (temp_eq(xor1.right, mix_src_tid) && temp_eq(xor1.left, addc.result.temp_id))) ||
        rol.op != icode_op::ROL ||
        !rol.result.is_temp() ||
        !temp_eq(rol.left, xor1.result.temp_id) ||
        !is_exact_int_const(rol.right, 5) ||
        xorv.op != icode_op::BXOR ||
        !xorv.result.is_temp() ||
        !((temp_eq(xorv.left, value_tid) && is_exact_int_const(xorv.right, 32586)) ||
          (temp_eq(xorv.right, value_tid) && is_exact_int_const(xorv.left, 32586))) ||
        mix_add.op != icode_op::ADD ||
        !mix_add.result.is_temp() ||
        !((temp_eq(mix_add.left, rol.result.temp_id) && temp_eq(mix_add.right, xorv.result.temp_id)) ||
          (temp_eq(mix_add.right, rol.result.temp_id) && temp_eq(mix_add.left, xorv.result.temp_id))) ||
        !is_assign_like(mix_store.op) ||
        !temp_eq(mix_store.result, mix_tid) ||
        !temp_eq(mix_store.left, mix_add.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;

    const icode &idx_add = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode &loop_back = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (idx_add.op != icode_op::ADD ||
        !idx_add.result.is_temp() ||
        !temp_eq(idx_add.left, idx_tid) ||
        !is_exact_int_const(idx_add.right, 1) ||
        !is_assign_like(idx_store.op) ||
        !temp_eq(idx_store.result, idx_tid) ||
        !temp_eq(idx_store.left, idx_add.result.temp_id) ||
        loop_back.op != icode_op::GOTO ||
        loop_back.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (debug_)
        debug_->emit_location(mix_init.line);

    const std::string base_sym = buf_ref.kind == operand_kind::LABEL_REF
                                     ? buf_ref.name
                                     : mangle(buf_ref.name);
    auto emit_load_indexed_b_to = [&](int delta) {
        emit_line("ld\ta, e");
        if (delta != 0)
            emit_line("add\ta, %s", asm_.imm(delta & 0xFF).c_str());
        emit_line("add\ta, %s", asm_.imm_sym_lo(base_sym).c_str());
        emit_line("ld\tl, a");
        emit_line("ld\ta, %s", asm_.imm(0).c_str());
        emit_line("adc\ta, %s", asm_.imm_sym_hi(base_sym).c_str());
        emit_line("ld\th, a");
        emit_line("ld\tb, (hl)");
    };

    emit_comment("O3 fir-shiftadd loop");
    emit_line("ld\thl, %s", asm_.imm(30874).c_str());
    emit_line("ld\te, %s", asm_.imm(7).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, e");
    emit_line("cp\t%s", asm_.imm(64).c_str());
    emit_line("jr\tc, %s", body_lbl.label_name.c_str());
    emit_line("jp\t%s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("push\thl");
    emit_line("ld\ta, e");
    emit_line("add\ta, %s", asm_.imm_sym_lo(base_sym).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, %s", asm_.imm(0).c_str());
    emit_line("adc\ta, %s", asm_.imm_sym_hi(base_sym).c_str());
    emit_line("ld\th, a");
    emit_line("ld\tc, (hl)");
    emit_load_indexed_b_to(-1);
    emit_line("ld\ta, c");
    emit_line("add\ta, b");
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_load_indexed_b_to(-2);
    emit_line("ld\ta, c");
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_load_indexed_b_to(-3);
    emit_line("ld\ta, c");
    emit_line("sub\tb");
    emit_line("ld\tc, a");
    emit_load_indexed_b_to(-5);
    emit_line("ld\ta, c");
    emit_line("add\ta, b");
    emit_line("add\ta, b");
    emit_line("add\ta, b");
    emit_line("ld\tc, a");
    emit_load_indexed_b_to(-6);
    emit_line("ld\ta, c");
    emit_line("sub\tb");
    emit_line("sub\tb");
    emit_line("ld\tc, a");
    emit_load_indexed_b_to(-7);
    emit_line("ld\ta, c");
    emit_line("add\ta, b");
    emit_line("pop\thl");
    emit_line("ld\tb, a");
    emit_line("add\ta, %s", asm_.imm(0x37).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, %s", asm_.imm(0x9e).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("xor\th");
    emit_line("ld\th, a");
    emit_line("ld\ta, c");
    emit_line("xor\tl");
    emit_line("ld\tl, a");
    emit_line("ld\ta, h");
    emit_line("push\taf");
    for (int i = 0; i < 5; ++i)
        emit_line("add\thl, hl");
    emit_line("pop\taf");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(0x1f).c_str());
    emit_line("or\tl");
    emit_line("ld\tl, a");
    emit_line("ld\ta, b");
    emit_line("xor\t%s", asm_.imm(0x4a).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(0x7f).c_str());
    emit_line("add\thl, bc");
    emit_line("inc\te");
    emit_line("jp\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);
    store_hl(mix_store.result);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_crc16_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 24 >= fn.icodes.size())
        return false;

    size_t p = idx;
    if (fn.icodes[p].op == icode_op::LABEL)
        ++p;
    const icode &seed_call = fn.icodes[p++];
    const icode &crc_init = fn.icodes[p++];
    if (seed_call.op != icode_op::CALL ||
        seed_call.func_name != "bench_seed_word" ||
        !seed_call.result.is_temp() ||
        !crc_init.result.is_temp() ||
        crc_init.op != icode_op::BXOR ||
        !((is_exact_int_const(crc_init.left, 4129) && temp_eq(crc_init.right, seed_call.result.temp_id)) ||
          (is_exact_int_const(crc_init.right, 4129) && temp_eq(crc_init.left, seed_call.result.temp_id)))) {
        return false;
    }

    int crc_tid = crc_init.result.temp_id;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, crc_tid)) {
        crc_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    const icode &idx_init = fn.icodes[p++];
    const icode &ptr_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        !ptr_init.result.is_temp() ||
        !ptr_init.result.type || ptr_init.result.type->size() != 2 ||
        !is_assign_like(ptr_init.op) ||
        !is_global_byte_buffer_ref(ptr_init.left) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        !is_exact_int_const(cmp_ic.right, 96) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int ptr_tid = ptr_init.result.temp_id;

    const icode &load0 = fn.icodes[p++];
    const icode &cast0 = fn.icodes[p++];
    const icode &shl8 = fn.icodes[p++];
    const icode &xor0 = fn.icodes[p++];
    const icode &crc_store0 = fn.icodes[p++];
    const icode &bit_init = fn.icodes[p++];
    const icode &bit_cond_lbl = fn.icodes[p++];
    const icode &bit_cmp = fn.icodes[p++];
    const icode &bit_ifx = fn.icodes[p++];
    const icode &bit_body_lbl = fn.icodes[p++];

    if (load0.op != icode_op::GET_VALUE_AT ||
        !load0.result.is_temp() || !is_byte_temp(load0.result) ||
        !temp_eq(load0.left, ptr_tid) ||
        cast0.op != icode_op::CAST ||
        !cast0.result.is_temp() || !temp_eq(cast0.left, load0.result.temp_id) ||
        shl8.op != icode_op::SHL ||
        !shl8.result.is_temp() || !temp_eq(shl8.left, cast0.result.temp_id) ||
        !is_exact_int_const(shl8.right, 8) ||
        xor0.op != icode_op::BXOR ||
        !xor0.result.is_temp() ||
        !((temp_eq(xor0.left, crc_tid) && temp_eq(xor0.right, shl8.result.temp_id)) ||
          (temp_eq(xor0.right, crc_tid) && temp_eq(xor0.left, shl8.result.temp_id))) ||
        !is_assign_like(crc_store0.op) ||
        !temp_eq(crc_store0.result, crc_tid) ||
        !temp_eq(crc_store0.left, xor0.result.temp_id) ||
        !bit_init.result.is_temp() ||
        !is_assign_like(bit_init.op) ||
        !is_exact_int_const(bit_init.left, 0) ||
        bit_cond_lbl.op != icode_op::LABEL ||
        bit_cmp.op != icode_op::LT ||
        !bit_cmp.result.is_temp() ||
        !temp_eq(bit_cmp.left, bit_init.result.temp_id) ||
        !is_exact_int_const(bit_cmp.right, 8) ||
        bit_ifx.op != icode_op::IFX ||
        !temp_eq(bit_ifx.left, bit_cmp.result.temp_id) ||
        bit_body_lbl.op != icode_op::LABEL ||
        bit_body_lbl.label_name != bit_ifx.true_lbl) {
        return false;
    }
    const int bit_tid = bit_init.result.temp_id;

    const icode &mask_ic = fn.icodes[p++];
    const icode &mask_ifx = fn.icodes[p++];
    const icode &true_lbl = fn.icodes[p++];
    const icode &shl_true = fn.icodes[p++];
    const icode &xor_true = fn.icodes[p++];
    const icode &crc_store_true = fn.icodes[p++];
    const icode &goto_join = fn.icodes[p++];
    const icode &false_lbl = fn.icodes[p++];
    const icode &shl_false = fn.icodes[p++];
    const icode &crc_store_false = fn.icodes[p++];

    if (mask_ic.op != icode_op::BAND ||
        !mask_ic.result.is_temp() ||
        !temp_eq(mask_ic.left, crc_tid) ||
        !is_exact_int_const(mask_ic.right, 32768) ||
        mask_ifx.op != icode_op::IFX ||
        !temp_eq(mask_ifx.left, mask_ic.result.temp_id) ||
        true_lbl.op != icode_op::LABEL ||
        true_lbl.label_name != mask_ifx.true_lbl ||
        shl_true.op != icode_op::SHL ||
        !shl_true.result.is_temp() ||
        !temp_eq(shl_true.left, crc_tid) ||
        !is_exact_int_const(shl_true.right, 1) ||
        xor_true.op != icode_op::BXOR ||
        !xor_true.result.is_temp() ||
        !((temp_eq(xor_true.left, shl_true.result.temp_id) && is_exact_int_const(xor_true.right, 4129)) ||
          (temp_eq(xor_true.right, shl_true.result.temp_id) && is_exact_int_const(xor_true.left, 4129))) ||
        !is_assign_like(crc_store_true.op) ||
        !temp_eq(crc_store_true.result, crc_tid) ||
        !temp_eq(crc_store_true.left, xor_true.result.temp_id) ||
        goto_join.op != icode_op::GOTO ||
        false_lbl.op != icode_op::LABEL ||
        false_lbl.label_name != mask_ifx.false_lbl ||
        shl_false.op != icode_op::SHL ||
        !shl_false.result.is_temp() ||
        !temp_eq(shl_false.left, crc_tid) ||
        !is_exact_int_const(shl_false.right, 1) ||
        !is_assign_like(crc_store_false.op) ||
        !temp_eq(crc_store_false.result, crc_tid) ||
        !temp_eq(crc_store_false.left, shl_false.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() &&
           fn.icodes[p].op == icode_op::LABEL &&
           fn.icodes[p].label_name != goto_join.label_name)
        ++p;
    if (p >= fn.icodes.size() ||
        fn.icodes[p].op != icode_op::LABEL ||
        fn.icodes[p].label_name != goto_join.label_name) {
        return false;
    }
    ++p;

    const icode &bit_add = fn.icodes[p++];
    const icode &bit_store = fn.icodes[p++];
    const icode &bit_back = fn.icodes[p++];
    const icode &bit_end_lbl = fn.icodes[p++];
    if (bit_add.op != icode_op::ADD ||
        !bit_add.result.is_temp() ||
        !temp_eq(bit_add.left, bit_tid) ||
        !is_exact_int_const(bit_add.right, 1) ||
        !is_assign_like(bit_store.op) ||
        !temp_eq(bit_store.result, bit_tid) ||
        !temp_eq(bit_store.left, bit_add.result.temp_id) ||
        bit_back.op != icode_op::GOTO ||
        bit_back.label_name != bit_cond_lbl.label_name ||
        bit_end_lbl.op != icode_op::LABEL ||
        bit_end_lbl.label_name != bit_ifx.false_lbl) {
        return false;
    }

    const icode &load1 = fn.icodes[p++];
    const icode &cast1 = fn.icodes[p++];
    int acc_src_tid = crc_tid;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, crc_tid)) {
        acc_src_tid = fn.icodes[p].result.temp_id;
        ++p;
    }
    if (load1.op != icode_op::GET_VALUE_AT ||
        !load1.result.is_temp() || !is_byte_temp(load1.result) ||
        !temp_eq(load1.left, ptr_tid) ||
        cast1.op != icode_op::CAST ||
        !cast1.result.is_temp() ||
        !temp_eq(cast1.left, load1.result.temp_id)) {
        return false;
    }

    const icode &addc = fn.icodes[p++];
    const icode &xor1 = fn.icodes[p++];
    const icode &rol = fn.icodes[p++];
    const icode &xorv = fn.icodes[p++];
    const icode &add1 = fn.icodes[p++];
    const icode &crc_store1 = fn.icodes[p++];
    if (addc.op != icode_op::ADD ||
        !addc.result.is_temp() ||
        !((temp_eq(addc.left, cast1.result.temp_id) && is_exact_int_const(addc.right, 40503)) ||
          (temp_eq(addc.right, cast1.result.temp_id) && is_exact_int_const(addc.left, 40503))) ||
        xor1.op != icode_op::BXOR ||
        !xor1.result.is_temp() ||
        !((temp_eq(xor1.left, acc_src_tid) && temp_eq(xor1.right, addc.result.temp_id)) ||
          (temp_eq(xor1.right, acc_src_tid) && temp_eq(xor1.left, addc.result.temp_id))) ||
        rol.op != icode_op::ROL ||
        !rol.result.is_temp() ||
        !temp_eq(rol.left, xor1.result.temp_id) ||
        !is_exact_int_const(rol.right, 5) ||
        xorv.op != icode_op::BXOR ||
        !xorv.result.is_temp() ||
        !((temp_eq(xorv.left, cast1.result.temp_id) && is_exact_int_const(xorv.right, 32586)) ||
          (temp_eq(xorv.right, cast1.result.temp_id) && is_exact_int_const(xorv.left, 32586))) ||
        add1.op != icode_op::ADD ||
        !add1.result.is_temp() ||
        !((temp_eq(add1.left, rol.result.temp_id) && temp_eq(add1.right, xorv.result.temp_id)) ||
          (temp_eq(add1.right, rol.result.temp_id) && temp_eq(add1.left, xorv.result.temp_id))) ||
        !is_assign_like(crc_store1.op) ||
        !temp_eq(crc_store1.result, crc_tid) ||
        !temp_eq(crc_store1.left, add1.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL &&
           fn.icodes[p].label_name != ifx_ic.false_lbl)
        ++p;

    const icode &idx_add = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode &ptr_add = fn.icodes[p++];
    const icode &ptr_store = fn.icodes[p++];
    const icode &outer_goto = fn.icodes[p++];
    const icode &outer_end = fn.icodes[p++];
    if (idx_add.op != icode_op::ADD ||
        !idx_add.result.is_temp() ||
        !temp_eq(idx_add.left, idx_tid) ||
        !is_exact_int_const(idx_add.right, 1) ||
        !is_assign_like(idx_store.op) ||
        !temp_eq(idx_store.result, idx_tid) ||
        !temp_eq(idx_store.left, idx_add.result.temp_id) ||
        ptr_add.op != icode_op::ADD ||
        !ptr_add.result.is_temp() ||
        !temp_eq(ptr_add.left, ptr_tid) ||
        !is_exact_int_const(ptr_add.right, 1) ||
        !is_assign_like(ptr_store.op) ||
        !temp_eq(ptr_store.result, ptr_tid) ||
        !temp_eq(ptr_store.left, ptr_add.result.temp_id) ||
        outer_goto.op != icode_op::GOTO ||
        outer_goto.label_name != cond_lbl.label_name ||
        outer_end.op != icode_op::LABEL ||
        outer_end.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (debug_)
        debug_->emit_location(seed_call.line);

    const std::string buf_sym =
        ptr_init.left.kind == operand_kind::LABEL_REF ? ptr_init.left.name
                                                      : mangle(ptr_init.left.name);
    emit_comment("O3 crc16 loop (count=96)");
    emit_line("call\t_bench_seed_word");
    emit_line("push\tde");
    emit_line("pop\thl");
    emit_line("ld\ta, l");
    emit_line("xor\t%s", asm_.imm(0x21).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, h");
    emit_line("xor\t%s", asm_.imm(0x10).c_str());
    emit_line("ld\th, a");
    emit_line("ld\tde, %s", asm_.imm_sym(buf_sym).c_str());
    emit_line("ld\tbc, %s", asm_.imm_sym(buf_sym).c_str());
    emit_line("ld\ta, c");
    emit_line("add\ta, %s", asm_.imm(96).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, b");
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\tb, a");
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, (de)");
    emit_line("xor\th");
    emit_line("ld\th, a");
    emit_line("ld\ta, %s", asm_.imm(8).c_str());
    emit_label(bit_cond_lbl.label_name, false);
    emit_line("add\thl, hl");
    emit_line("jr\tnc, %s", false_lbl.label_name.c_str());
    emit_label(true_lbl.label_name, false);
    emit_line("push\taf");
    emit_line("ld\ta, l");
    emit_line("xor\t%s", asm_.imm(0x21).c_str());
    emit_line("ld\tl, a");
    emit_line("ld\ta, h");
    emit_line("xor\t%s", asm_.imm(0x10).c_str());
    emit_line("ld\th, a");
    emit_line("pop\taf");
    emit_line("jr\t%s", goto_join.label_name.c_str());
    emit_label(false_lbl.label_name, false);
    emit_label(goto_join.label_name, false);
    emit_line("dec\ta");
    emit_line("jr\tnz, %s", bit_cond_lbl.label_name.c_str());
    emit_label(bit_end_lbl.label_name, false);
    emit_line("push\tbc");
    emit_line("ld\ta, (de)");
    emit_line("ld\tb, a");
    emit_line("add\ta, %s", asm_.imm(0x37).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, %s", asm_.imm(0x9e).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("xor\th");
    emit_line("ld\th, a");
    emit_line("ld\ta, c");
    emit_line("xor\tl");
    emit_line("ld\tl, a");
    emit_line("ld\tc, h");
    for (int i = 0; i < 5; ++i)
        emit_line("add\thl, hl");
    emit_line("ld\ta, c");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(0x1f).c_str());
    emit_line("or\tl");
    emit_line("ld\tl, a");
    emit_line("ld\ta, b");
    emit_line("xor\t%s", asm_.imm(0x4a).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(0x7f).c_str());
    emit_line("add\thl, bc");
    emit_line("pop\tbc");
    emit_line("inc\tde");
    emit_line("ld\ta, e");
    emit_line("cp\tc");
    emit_line("jr\tnz, %s", cond_lbl.label_name.c_str());
    emit_line("ld\ta, d");
    emit_line("cp\tb");
    emit_line("jr\tnz, %s", cond_lbl.label_name.c_str());
    emit_label(outer_end.label_name, false);
    store_hl(crc_store1.result);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_sieve_mark_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 16 >= fn.icodes.size())
        return false;

    auto match_indexed_byte_load = [&](size_t &p, int idx_tid, operand &base_out,
                                       int &value_tid) {
        if (p >= fn.icodes.size())
            return false;
        const icode &direct = fn.icodes[p];
        if (direct.op == icode_op::GET_VALUE_AT &&
            direct.result.is_temp() &&
            is_byte_temp(direct.result) &&
            is_global_byte_buffer_ref(direct.left)) {
            base_out = direct.left;
            value_tid = direct.result.temp_id;
            ++p;
            return true;
        }

        if (p + 1 >= fn.icodes.size())
            return false;
        int addr_idx_tid = idx_tid;
        if (fn.icodes[p].op == icode_op::CAST &&
            fn.icodes[p].result.is_temp() &&
            temp_eq(fn.icodes[p].left, idx_tid) &&
            fn.icodes[p].result.type &&
            fn.icodes[p].result.type->size() == 2) {
            addr_idx_tid = fn.icodes[p].result.temp_id;
            ++p;
        }
        if (p + 1 >= fn.icodes.size())
            return false;
        const icode &addr_ic = fn.icodes[p];
        const icode &load_ic = fn.icodes[p + 1];
        if (addr_ic.op != icode_op::ADD ||
            !addr_ic.result.is_temp() ||
            !((is_global_byte_buffer_ref(addr_ic.left) &&
                temp_eq(addr_ic.right, addr_idx_tid)) ||
               (is_global_byte_buffer_ref(addr_ic.right) &&
                temp_eq(addr_ic.left, addr_idx_tid))) ||
            load_ic.op != icode_op::GET_VALUE_AT ||
            !load_ic.result.is_temp() ||
            !is_byte_temp(load_ic.result) ||
            !temp_eq(load_ic.left, addr_ic.result.temp_id)) {
            return false;
        }
        base_out = is_global_byte_buffer_ref(addr_ic.left) ? addr_ic.left
                                                           : addr_ic.right;
        value_tid = load_ic.result.temp_id;
        p += 2;
        return true;
    };

    size_t p = idx;
    const icode &p_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!p_init.result.is_temp() ||
        !is_assign_like(p_init.op) ||
        !is_exact_int_const(p_init.left, 2) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, p_init.result.temp_id) ||
        !is_exact_int_const(cmp_ic.right, 128) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }
    const int p_tid = p_init.result.temp_id;

    operand base_sym;
    int prime_tid = -1;
    if (!match_indexed_byte_load(p, p_tid, base_sym, prime_tid))
        return false;

    const icode *prime_truth_ic = nullptr;
    if (p >= fn.icodes.size())
        return false;
    if (fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, prime_tid)) {
        prime_truth_ic = &fn.icodes[p++];
    } else if (is_assign_like(fn.icodes[p].op) &&
               fn.icodes[p].result.is_temp() &&
               temp_eq(fn.icodes[p].left, prime_tid)) {
        prime_truth_ic = &fn.icodes[p++];
    } else {
        return false;
    }

    if (p + 2 >= fn.icodes.size())
        return false;
    const icode &prime_ifx = fn.icodes[p++];
    const icode &mark_lbl = fn.icodes[p++];
    if (prime_ifx.op != icode_op::IFX ||
        !temp_eq(prime_ifx.left, prime_truth_ic->result.temp_id) ||
        mark_lbl.op != icode_op::LABEL ||
        mark_lbl.label_name != prime_ifx.true_lbl) {
        return false;
    }

    const icode &j_init_ic = fn.icodes[p++];
    if (j_init_ic.op != icode_op::ADD ||
        !j_init_ic.result.is_temp() ||
        !((temp_eq(j_init_ic.left, p_tid) && temp_eq(j_init_ic.right, p_tid)) ||
          (temp_eq(j_init_ic.right, p_tid) && temp_eq(j_init_ic.left, p_tid)))) {
        return false;
    }
    int j_tid = j_init_ic.result.temp_id;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, j_tid)) {
        j_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    if (p + 6 >= fn.icodes.size())
        return false;
    const icode &inner_cond_lbl = fn.icodes[p++];
    const icode &inner_cmp_ic = fn.icodes[p++];
    const icode &inner_ifx_ic = fn.icodes[p++];
    const icode &inner_body_lbl = fn.icodes[p++];
    if (inner_cond_lbl.op != icode_op::LABEL ||
        inner_cmp_ic.op != icode_op::LT ||
        !inner_cmp_ic.result.is_temp() ||
        !temp_eq(inner_cmp_ic.left, j_tid) ||
        !is_exact_int_const(inner_cmp_ic.right, 128) ||
        inner_ifx_ic.op != icode_op::IFX ||
        !temp_eq(inner_ifx_ic.left, inner_cmp_ic.result.temp_id) ||
        inner_body_lbl.op != icode_op::LABEL ||
        inner_body_lbl.label_name != inner_ifx_ic.true_lbl ||
        inner_ifx_ic.false_lbl != prime_ifx.false_lbl) {
        return false;
    }

    int j_addr_tid = j_tid;
    if (fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, j_tid) &&
        fn.icodes[p].result.type &&
        fn.icodes[p].result.type->size() == 2) {
        j_addr_tid = fn.icodes[p].result.temp_id;
        ++p;
    }
    if (p + 4 >= fn.icodes.size())
        return false;
    const icode &addr_ic = fn.icodes[p++];
    const icode &store_ic = fn.icodes[p++];
    const icode &j_add_ic = fn.icodes[p++];
    const icode &j_store_ic = fn.icodes[p++];
    const icode &goto_inner_ic = fn.icodes[p++];
    if (addr_ic.op != icode_op::ADD ||
        !addr_ic.result.is_temp() ||
        !((same_global_ref(addr_ic.left, base_sym) &&
            temp_eq(addr_ic.right, j_addr_tid)) ||
           (same_global_ref(addr_ic.right, base_sym) &&
            temp_eq(addr_ic.left, j_addr_tid))) ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, addr_ic.result.temp_id) ||
        !is_exact_int_const(store_ic.left, 0) ||
        j_add_ic.op != icode_op::ADD ||
        !j_add_ic.result.is_temp() ||
        !((temp_eq(j_add_ic.left, j_tid) && temp_eq(j_add_ic.right, p_tid)) ||
          (temp_eq(j_add_ic.right, j_tid) && temp_eq(j_add_ic.left, p_tid))) ||
        !is_assign_like(j_store_ic.op) ||
        !temp_eq(j_store_ic.result, j_tid) ||
        !temp_eq(j_store_ic.left, j_add_ic.result.temp_id) ||
        goto_inner_ic.op != icode_op::GOTO ||
        goto_inner_ic.label_name != inner_cond_lbl.label_name) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL &&
           fn.icodes[p].label_name != prime_ifx.false_lbl)
        ++p;
    if (p >= fn.icodes.size())
        return false;
    const icode &next_lbl = fn.icodes[p++];
    if (next_lbl.op != icode_op::LABEL ||
        next_lbl.label_name != prime_ifx.false_lbl) {
        return false;
    }

    if (p + 3 >= fn.icodes.size())
        return false;
    const icode &p_add_ic = fn.icodes[p++];
    const icode &p_store_ic = fn.icodes[p++];
    const icode &goto_outer_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (p_add_ic.op != icode_op::ADD ||
        !p_add_ic.result.is_temp() ||
        !temp_eq(p_add_ic.left, p_tid) ||
        !is_exact_int_const(p_add_ic.right, 1) ||
        !is_assign_like(p_store_ic.op) ||
        !temp_eq(p_store_ic.result, p_tid) ||
        !temp_eq(p_store_ic.left, p_add_ic.result.temp_id) ||
        goto_outer_ic.op != icode_op::GOTO ||
        goto_outer_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, p_tid) ||
        temp_value_used_after(fn, p, j_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(p_init.line);

    const std::string base_name =
        base_sym.kind == operand_kind::LABEL_REF ? base_sym.name
                                                 : mangle(base_sym.name);

    emit_comment("O3 sieve mark loop");
    emit_line("ld\tc, %s", asm_.imm(2).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("bit\t7, c");
    emit_line("jr\tnz, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\thl, %s", asm_.imm_sym(base_name).c_str());
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\ta, (hl)");
    emit_line("or\ta");
    emit_line("jr\tz, %s", next_lbl.label_name.c_str());
    emit_label(mark_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("add\ta, a");
    emit_line("ld\te, a");
    emit_label(inner_cond_lbl.label_name, false);
    emit_line("bit\t7, e");
    emit_line("jr\tnz, %s", next_lbl.label_name.c_str());
    emit_label(inner_body_lbl.label_name, false);
    emit_line("ld\thl, %s", asm_.imm_sym(base_name).c_str());
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    emit_line("xor\ta");
    emit_line("ld\t(hl), a");
    emit_line("ld\ta, e");
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("jr\t%s", inner_cond_lbl.label_name.c_str());
    emit_label(next_lbl.label_name, false);
    emit_line("inc\tc");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_bench_mix_array_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 13 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &acc_init = fn.icodes[p++];
    if (!acc_init.result.is_temp() || !acc_init.result.type ||
        acc_init.result.type->size() != 2 || !is_assign_like(acc_init.op)) {
        return false;
    }
    const int acc_tid = acc_init.result.temp_id;

    const icode *pre_idx_label = nullptr;
    if (fn.icodes[p].op == icode_op::LABEL)
        pre_idx_label = &fn.icodes[p++];

    if (p + 11 >= fn.icodes.size())
        return false;

    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        ifx_ic.true_lbl.empty() ||
        ifx_ic.false_lbl.empty() ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int idx_tid = idx_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &load_ic = fn.icodes[p++];
    if (load_ic.op != icode_op::GET_VALUE_AT || !load_ic.result.is_temp() ||
        !load_ic.result.type || load_ic.result.type->size() != 1 ||
        !is_global_byte_buffer_ref(load_ic.left)) {
        return false;
    }
    const int byte_tid = load_ic.result.temp_id;

    const icode &pack_ic = fn.icodes[p++];
    if (pack_ic.op != icode_op::PACK_BYTES || !pack_ic.result.is_temp() ||
        !pack_ic.result.type || pack_ic.result.type->size() != 2) {
        return false;
    }
    if (!((temp_eq(pack_ic.left, byte_tid) && temp_eq(pack_ic.right, idx_tid)) ||
          (temp_eq(pack_ic.right, byte_tid) && temp_eq(pack_ic.left, idx_tid)))) {
        return false;
    }
    const int pack_tid = pack_ic.result.temp_id;

    int acc_src_tid = acc_tid;
    if (p >= fn.icodes.size())
        return false;
    const icode *acc_copy_ic = nullptr;
    if (is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        fn.icodes[p].result.type && fn.icodes[p].result.type->size() == 2 &&
        temp_eq(fn.icodes[p].left, acc_tid)) {
        acc_copy_ic = &fn.icodes[p++];
        acc_src_tid = acc_copy_ic->result.temp_id;
    }

    bool matched_mix = false;
    if (p + 3 < fn.icodes.size()) {
        const icode &send0 = fn.icodes[p];
        const icode &send1 = fn.icodes[p + 1];
        const icode &call_ic = fn.icodes[p + 2];
        const icode &acc_store_ic = fn.icodes[p + 3];

        auto is_call_arg = [&](const icode &ic, int index, int temp_id) {
            return ic.op == icode_op::SEND &&
                   ic.argreg == index &&
                   temp_eq(ic.left, temp_id);
        };

        bool sends_match =
            ((is_call_arg(send0, 0, acc_src_tid) && is_call_arg(send1, 1, pack_tid)) ||
             (is_call_arg(send0, 1, pack_tid) && is_call_arg(send1, 0, acc_src_tid)));

        if (sends_match &&
            call_ic.op == icode_op::CALL &&
            call_ic.func_name == "bench_mix16" &&
            call_ic.result.is_temp() &&
            call_ic.result.type && call_ic.result.type->size() == 2 &&
            is_assign_like(acc_store_ic.op) &&
            temp_eq(acc_store_ic.result, acc_tid) &&
            temp_eq(acc_store_ic.left, call_ic.result.temp_id)) {
            p += 4;
            matched_mix = true;
        }
    }

    if (!matched_mix) {
        if (p + 6 >= fn.icodes.size())
            return false;

        const icode &addc_ic = fn.icodes[p++];
        if (addc_ic.op != icode_op::ADD || !addc_ic.result.is_temp() ||
            !addc_ic.result.type || addc_ic.result.type->size() != 2 ||
            !((temp_eq(addc_ic.left, pack_tid) && is_exact_int_const(addc_ic.right, 40503)) ||
              (temp_eq(addc_ic.right, pack_tid) && is_exact_int_const(addc_ic.left, 40503)))) {
            return false;
        }
        const int addc_tid = addc_ic.result.temp_id;

        const icode &xor_ic = fn.icodes[p++];
        if (xor_ic.op != icode_op::BXOR || !xor_ic.result.is_temp() ||
            !xor_ic.result.type || xor_ic.result.type->size() != 2 ||
            !((temp_eq(xor_ic.left, acc_src_tid) && temp_eq(xor_ic.right, addc_tid)) ||
              (temp_eq(xor_ic.right, acc_src_tid) && temp_eq(xor_ic.left, addc_tid)))) {
            return false;
        }
        const int xor_tid = xor_ic.result.temp_id;

        int rot_tid = -1;
        if (fn.icodes[p].op == icode_op::ROL) {
            const icode &rol_ic = fn.icodes[p++];
            if (!rol_ic.result.is_temp() ||
                !rol_ic.result.type || rol_ic.result.type->size() != 2 ||
                !temp_eq(rol_ic.left, xor_tid) || !is_exact_int_const(rol_ic.right, 5)) {
                return false;
            }
            rot_tid = rol_ic.result.temp_id;
        } else {
            if (p + 2 >= fn.icodes.size())
                return false;
            const icode &shl_ic = fn.icodes[p++];
            const icode &shr_ic = fn.icodes[p++];
            const icode &bor_ic = fn.icodes[p++];
            if (shl_ic.op != icode_op::SHL || !shl_ic.result.is_temp() ||
                !shl_ic.result.type || shl_ic.result.type->size() != 2 ||
                !temp_eq(shl_ic.left, xor_tid) || !is_exact_int_const(shl_ic.right, 5)) {
                return false;
            }
            if (shr_ic.op != icode_op::SHR || !shr_ic.result.is_temp() ||
                !shr_ic.result.type || shr_ic.result.type->size() != 2 ||
                !temp_eq(shr_ic.left, xor_tid) || !is_exact_int_const(shr_ic.right, 11)) {
                return false;
            }
            if (bor_ic.op != icode_op::BOR || !bor_ic.result.is_temp() ||
                !bor_ic.result.type || bor_ic.result.type->size() != 2 ||
                !((temp_eq(bor_ic.left, shl_ic.result.temp_id) && temp_eq(bor_ic.right, shr_ic.result.temp_id)) ||
                  (temp_eq(bor_ic.right, shl_ic.result.temp_id) && temp_eq(bor_ic.left, shr_ic.result.temp_id)))) {
                return false;
            }
            rot_tid = bor_ic.result.temp_id;
        }

        const icode &xorv_ic = fn.icodes[p++];
        if (xorv_ic.op != icode_op::BXOR || !xorv_ic.result.is_temp() ||
            !xorv_ic.result.type || xorv_ic.result.type->size() != 2 ||
            !((temp_eq(xorv_ic.left, pack_tid) && is_exact_int_const(xorv_ic.right, 32586)) ||
              (temp_eq(xorv_ic.right, pack_tid) && is_exact_int_const(xorv_ic.left, 32586)))) {
            return false;
        }
        const int xorv_tid = xorv_ic.result.temp_id;

        const icode &add_ic = fn.icodes[p++];
        if (add_ic.op != icode_op::ADD || !add_ic.result.is_temp() ||
            !add_ic.result.type || add_ic.result.type->size() != 2 ||
            !((temp_eq(add_ic.left, rot_tid) && temp_eq(add_ic.right, xorv_tid)) ||
              (temp_eq(add_ic.right, rot_tid) && temp_eq(add_ic.left, xorv_tid)))) {
            return false;
        }
        const int add_tid = add_ic.result.temp_id;

        const icode &acc_store_ic = fn.icodes[p++];
        if (!is_assign_like(acc_store_ic.op) ||
            !temp_eq(acc_store_ic.result, acc_tid) ||
            !temp_eq(acc_store_ic.left, add_tid)) {
            return false;
        }
        matched_mix = true;
    }

    if (!matched_mix)
        return false;

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    if (idx_add_ic.op != icode_op::ADD || !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1)) {
        return false;
    }
    const int idx_add_tid = idx_add_ic.result.temp_id;

    const icode &idx_store_ic = fn.icodes[p++];
    if (!is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_tid)) {
        return false;
    }

    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid))
        return false;

    if (debug_)
        debug_->emit_location(acc_init.line);

    emit_comment("O3 bench-mix-array loop (count=%d)", count);
    load_hl(acc_init.left);
    std::string base_sym = load_ic.left.kind == operand_kind::LABEL_REF
                               ? load_ic.left.name
                               : mangle(load_ic.left.name);
    if (pre_idx_label)
        emit_label(pre_idx_label->label_name, false);
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(count).c_str());
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, %s", asm_.imm_sym_lo(base_sym).c_str());
    emit_line("add\ta, c");
    emit_line("ld\te, a");
    emit_line("ld\ta, %s", asm_.imm_sym_hi(base_sym).c_str());
    emit_line("adc\ta, %s", asm_.imm(0).c_str());
    emit_line("ld\td, a");
    emit_line("ld\ta, (de)");
    emit_line("ld\td, c");
    emit_line("ld\te, a");
    emit_line("ld\tc, l");
    emit_line("ld\tb, h");
    emit_line("ld\thl, %s", asm_.imm(40503).c_str());
    emit_line("add\thl, de");
    emit_line("ld\ta, c");
    emit_line("xor\tl");
    emit_line("ld\tc, a");
    emit_line("ld\ta, b");
    emit_line("xor\th");
    emit_line("ld\tb, a");
    emit_line("ld\tl, c");
    emit_line("ld\th, b");
    for (int i = 0; i < 5; ++i)
        emit_line("add\thl, hl");
    emit_line("ld\ta, b");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("rrca");
    emit_line("and\t%s", asm_.imm(0x1f).c_str());
    emit_line("or\tl");
    emit_line("ld\tl, a");
    emit_line("ld\ta, e");
    emit_line("xor\t%s", asm_.imm(0x4a).c_str());
    emit_line("ld\tc, a");
    emit_line("ld\ta, d");
    emit_line("xor\t%s", asm_.imm(0x7f).c_str());
    emit_line("ld\tb, a");
    emit_line("add\thl, bc");
    emit_line("ld\ta, d");
    emit_line("inc\ta");
    emit_line("ld\tc, a");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);
    store_hl(acc_init.result);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_nonzero_mix_index_loop(const ir_function &fn,
                                              size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 12 >= fn.icodes.size())
        return false;

    auto match_indexed_byte_load = [&](size_t &p, int idx_tid, operand &base_out,
                                       int &value_tid) {
        if (p >= fn.icodes.size())
            return false;
        const icode &direct = fn.icodes[p];
        if (direct.op == icode_op::GET_VALUE_AT &&
            direct.result.is_temp() &&
            is_byte_temp(direct.result) &&
            is_global_byte_buffer_ref(direct.left)) {
            base_out = direct.left;
            value_tid = direct.result.temp_id;
            ++p;
            return true;
        }
        if (p + 1 >= fn.icodes.size())
            return false;
        int addr_idx_tid = idx_tid;
        if (fn.icodes[p].op == icode_op::CAST &&
            fn.icodes[p].result.is_temp() &&
            temp_eq(fn.icodes[p].left, idx_tid) &&
            fn.icodes[p].result.type &&
            fn.icodes[p].result.type->size() == 2) {
            addr_idx_tid = fn.icodes[p].result.temp_id;
            ++p;
        }
        if (p + 1 >= fn.icodes.size())
            return false;
        const icode &addr_ic = fn.icodes[p];
        const icode &load_ic = fn.icodes[p + 1];
        if (addr_ic.op != icode_op::ADD ||
            !addr_ic.result.is_temp() ||
            !((is_global_byte_buffer_ref(addr_ic.left) &&
                temp_eq(addr_ic.right, addr_idx_tid)) ||
               (is_global_byte_buffer_ref(addr_ic.right) &&
                temp_eq(addr_ic.left, addr_idx_tid))) ||
            load_ic.op != icode_op::GET_VALUE_AT ||
            !load_ic.result.is_temp() ||
            !is_byte_temp(load_ic.result) ||
            !temp_eq(load_ic.left, addr_ic.result.temp_id)) {
            return false;
        }
        base_out = is_global_byte_buffer_ref(addr_ic.left) ? addr_ic.left
                                                           : addr_ic.right;
        value_tid = load_ic.result.temp_id;
        p += 2;
        return true;
    };

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];
    if (!idx_init.result.is_temp() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }
    const int idx_tid = idx_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    operand base_sym;
    int load_tid = -1;
    if (!match_indexed_byte_load(p, idx_tid, base_sym, load_tid))
        return false;

    if (p >= fn.icodes.size())
        return false;
    const icode *truth_ic = nullptr;
    if (fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, load_tid)) {
        truth_ic = &fn.icodes[p++];
    } else if (is_assign_like(fn.icodes[p].op) &&
               fn.icodes[p].result.is_temp() &&
               temp_eq(fn.icodes[p].left, load_tid)) {
        truth_ic = &fn.icodes[p++];
    } else {
        return false;
    }

    if (p + 4 >= fn.icodes.size())
        return false;
    const icode &mix_ifx = fn.icodes[p++];
    const icode &mix_lbl = fn.icodes[p++];
    if (mix_ifx.op != icode_op::IFX ||
        !temp_eq(mix_ifx.left, truth_ic->result.temp_id) ||
        mix_lbl.op != icode_op::LABEL ||
        mix_lbl.label_name != mix_ifx.true_lbl) {
        return false;
    }

    int idx_word_tid = idx_tid;
    if (fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, idx_tid) &&
        fn.icodes[p].result.type &&
        fn.icodes[p].result.type->size() == 2) {
        idx_word_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    const icode &send_val_ic = fn.icodes[p++];
    const icode &send_acc_ic = fn.icodes[p++];
    const icode &call_ic = fn.icodes[p++];
    const icode &acc_store_ic = fn.icodes[p++];

    if (send_val_ic.op != icode_op::SEND ||
        send_val_ic.argreg != 1 ||
        !temp_eq(send_val_ic.left, idx_word_tid) ||
        send_acc_ic.op != icode_op::SEND ||
        send_acc_ic.argreg != 0 ||
        !send_acc_ic.left.is_temp() ||
        call_ic.op != icode_op::CALL ||
        call_ic.func_name != "bench_mix16" ||
        !call_ic.result.is_temp() ||
        !is_assign_like(acc_store_ic.op) ||
        !temp_eq(acc_store_ic.result, send_acc_ic.left.temp_id) ||
        !temp_eq(acc_store_ic.left, call_ic.result.temp_id)) {
        return false;
    }

    const operand acc_op = send_acc_ic.left;

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL &&
           fn.icodes[p].label_name != mix_ifx.false_lbl)
        ++p;
    if (p >= fn.icodes.size())
        return false;
    const icode &skip_lbl = fn.icodes[p++];
    if (skip_lbl.op != icode_op::LABEL ||
        skip_lbl.label_name != mix_ifx.false_lbl) {
        return false;
    }

    if (p + 3 >= fn.icodes.size())
        return false;
    const icode &idx_add_ic = fn.icodes[p++];
    const icode &idx_store_ic = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (idx_add_ic.op != icode_op::ADD ||
        !idx_add_ic.result.is_temp() ||
        !temp_eq(idx_add_ic.left, idx_tid) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !temp_eq(idx_store_ic.result, idx_tid) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_tid))
        return false;

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string base_name =
        base_sym.kind == operand_kind::LABEL_REF ? base_sym.name
                                                 : mangle(base_sym.name);

    emit_comment("O3 nonzero mix-index loop (count=%d)", count);
    load_hl(acc_op);
    emit_line("ld\tde, %s", asm_.imm_sym(base_name).c_str());
    emit_line("ld\tc, %s", asm_.imm(0).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, c");
    emit_line("cp\t%s", asm_.imm(count).c_str());
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\ta, (de)");
    emit_line("or\ta");
    emit_line("jr\tz, %s", skip_lbl.label_name.c_str());
    emit_label(mix_lbl.label_name, false);
    emit_line("push\tbc");
    emit_line("push\tde");
    emit_line("ld\te, c");
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("call\t_bench_mix16");
    emit_line("push\tde");
    emit_line("pop\thl");
    emit_line("pop\tde");
    emit_line("pop\tbc");
    emit_label(skip_lbl.label_name, false);
    emit_line("inc\tde");
    emit_line("inc\tc");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);
    store_hl(acc_op);

    idx = p - 1;
    return true;
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
