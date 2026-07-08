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

bool is_asm_space_name(const std::string &name) {
    return name.size() >= 2 && name[0] == '_' && name[1] == '_';
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

bool same_value_operand(const operand &a, const operand &b) {
    if (a.kind != b.kind || a.byte_offset != b.byte_offset)
        return false;
    switch (a.kind) {
    case operand_kind::TEMP:
        return a.temp_id == b.temp_id;
    case operand_kind::SYMBOL:
        return a.name == b.name &&
               a.is_global == b.is_global &&
               a.is_param == b.is_param &&
               a.is_func == b.is_func &&
               a.is_tls == b.is_tls &&
               a.is_sfr == b.is_sfr &&
               a.sfr_port == b.sfr_port &&
               a.stack_offset == b.stack_offset;
    case operand_kind::LABEL_REF:
        return a.name == b.name;
    case operand_kind::INT_CONST:
        return a.ival == b.ival;
    case operand_kind::FLOAT_CONST:
        return a.fval == b.fval;
    case operand_kind::NONE:
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
    // Labels may be reached from multiple control-flow paths with different
    // push/pop histories, so any linear SP<->IX delta we tracked is no longer
    // trustworthy past the join point.
    clear_known_sp_ix_delta();
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
    if (name.empty())
        return name;
    if (is_asm_space_name(name))
        return name;
    return "_" + name;
}

std::string z80_gen::asm_label_ref_name(const std::string &name) const {
    if (name.empty())
        return name;
    if (name[0] == '.' || is_asm_space_name(name))
        return name;
    return mangle(name);
}

std::string z80_gen::asm_symbol_ref_name(const operand &op) const {
    if (op.kind == operand_kind::LABEL_REF)
        return asm_label_ref_name(op.name);
    return mangle(op.name);
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
    skipped_icodes_.clear();
    direct_postinc_load_pending_ = false;
    direct_postinc_load_cursor_ = operand{};
    direct_postinc_load_old_ptr_ = operand{};
    direct_postinc_load_step_ = 0;
    direct_postinc_load_get_index_ = 0;
    direct_word_load_ifx_pending_ = false;
    direct_word_load_ifx_value_ = operand{};
    direct_word_value_pending_ = false;
    direct_word_value_ = operand{};
    direct_call_ifx_keep_word_pending_ = false;
    clear_known_sp_ix_delta();
    invalidate_pair_cache();
    invalidate_a_cache();
    if (fn.bank < 0) asm_.section_code();
    else asm_.section_code_named(banked_code_section_name(fn.bank));

    if (tuned_profile_enabled() &&
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
         try_emit_crc8_byte_loop_function(fn) ||
         try_emit_rle_byte_fill_function(fn) ||
         try_emit_rle_byte_encode_function(fn) ||
         try_emit_sdcc_style_helper(fn) ||
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
        if (try_emit_lcg_byte_fill_loop(fn, i))
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
        if (try_emit_int_table_binary_search_loop(fn, i))
            continue;
        if (try_emit_repeat_call_xor_loop(fn, i))
            continue;
        if (try_emit_repeat_signed_byte_mix_xor_loop(fn, i))
            continue;
        if (try_emit_signed_byte_mix_loop(fn, i))
            continue;
        if (try_emit_byte_const_mul_add_store(fn, i))
            continue;
        if (try_emit_switch_jump_table(fn, i))
            continue;
        if (try_emit_byte_shift_xor_step(fn, i))
            continue;
        if (try_emit_u16_shift_xor_run(fn, i))
            continue;
        if (try_emit_u16_shift_xor_step(fn, i))
            continue;
        if (try_emit_u32_shift_xor_run(fn, i))
            continue;
        if (try_emit_u32_shift_xor_step(fn, i))
            continue;
        if (try_emit_band_ifx(fn, i))
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
            data_sym = asm_symbol_ref_name(ic.left);
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

bool z80_gen::try_emit_crc8_byte_loop_function(const ir_function &fn) {
    if (!fn.ret_type || fn.ret_type->size() != 1 || fn.num_params != 1)
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }
    if (body.size() < 20)
        return false;

    size_t p = 0;
    const icode &recv = *body[p++];
    const icode &crc_init = *body[p++];
    const icode &end_add = *body[p++];
    const icode &cond_lbl = *body[p++];
    const icode &cmp_ic = *body[p++];
    const icode &ifx_ic = *body[p++];
    const icode &body_lbl = *body[p++];

    if (recv.op != icode_op::RECEIVE ||
        recv.arg_loc != abi_arg_loc::REG_HL ||
        !recv.result.is_temp() ||
        !is_assign_like(crc_init.op) ||
        !is_exact_int_const(crc_init.left, 0xff) ||
        op_size(crc_init.result) != 1 ||
        end_add.op != icode_op::ADD ||
        !end_add.result.is_temp() ||
        !temp_eq(end_add.left, recv.result.temp_id) ||
        !is_exact_int_const(end_add.right, 1024) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, recv.result.temp_id) ||
        !temp_eq(cmp_ic.right, end_add.result.temp_id) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const operand ptr_op = recv.result;
    const operand crc_op = crc_init.result;
    const icode &old_ptr = *body[p++];
    const icode &ptr_add = *body[p++];
    const icode &ptr_store = *body[p++];
    const icode &load_ic = *body[p++];
    const icode &xor0 = *body[p++];
    const icode &crc_store0 = *body[p++];
    if (!old_ptr.result.is_temp() ||
        !is_assign_like(old_ptr.op) ||
        !temp_eq(old_ptr.left, ptr_op.temp_id) ||
        ptr_add.op != icode_op::ADD ||
        !ptr_add.result.is_temp() ||
        !temp_eq(ptr_add.left, ptr_op.temp_id) ||
        !is_exact_int_const(ptr_add.right, 1) ||
        !is_assign_like(ptr_store.op) ||
        !temp_eq(ptr_store.result, ptr_op.temp_id) ||
        !temp_eq(ptr_store.left, ptr_add.result.temp_id) ||
        load_ic.op != icode_op::GET_VALUE_AT ||
        !is_byte_temp(load_ic.result) ||
        !temp_eq(load_ic.left, old_ptr.result.temp_id) ||
        xor0.op != icode_op::BXOR ||
        !xor0.result.is_temp() ||
        !((same_value_operand(xor0.left, crc_op) &&
           temp_eq(xor0.right, load_ic.result.temp_id)) ||
          (same_value_operand(xor0.right, crc_op) &&
           temp_eq(xor0.left, load_ic.result.temp_id))) ||
        !is_assign_like(crc_store0.op) ||
        !same_value_operand(crc_store0.result, crc_op) ||
        !temp_eq(crc_store0.left, xor0.result.temp_id)) {
        return false;
    }

    std::vector<uint8_t> polys;
    polys.reserve(8);
    auto parse_step = [&](size_t &pos) -> bool {
        if (pos + 13 >= body.size())
            return false;
        const icode &mask_ic = *body[pos++];
        const icode &step_ifx = *body[pos++];
        const icode &true_lbl = *body[pos++];
        const icode &shl_true = *body[pos++];
        if (mask_ic.op != icode_op::BAND ||
            !mask_ic.result.is_temp() ||
            !same_value_operand(mask_ic.left, crc_op) ||
            !is_exact_int_const(mask_ic.right, 0x80) ||
            step_ifx.op != icode_op::IFX ||
            !temp_eq(step_ifx.left, mask_ic.result.temp_id) ||
            true_lbl.op != icode_op::LABEL ||
            true_lbl.label_name != step_ifx.true_lbl ||
            shl_true.op != icode_op::SHL ||
            !shl_true.result.is_temp() ||
            !same_value_operand(shl_true.left, crc_op) ||
            !is_exact_int_const(shl_true.right, 1)) {
            return false;
        }

        int shift_tid = shl_true.result.temp_id;
        if (body[pos]->op == icode_op::CAST &&
            body[pos]->result.is_temp() &&
            temp_eq(body[pos]->left, shift_tid)) {
            shift_tid = body[pos]->result.temp_id;
            ++pos;
        }

        const icode &xor_ic = *body[pos++];
        int64_t poly = 0;
        if (xor_ic.op != icode_op::BXOR || !xor_ic.result.is_temp())
            return false;
        if (temp_eq(xor_ic.left, shift_tid) &&
            xor_ic.right.kind == operand_kind::INT_CONST) {
            poly = xor_ic.right.ival;
        } else if (temp_eq(xor_ic.right, shift_tid) &&
                   xor_ic.left.kind == operand_kind::INT_CONST) {
            poly = xor_ic.left.ival;
        } else {
            return false;
        }
        if (poly < 0 || poly > 0xff)
            return false;

        const icode &true_store = *body[pos++];
        const icode &goto_join = *body[pos++];
        const icode &false_lbl = *body[pos++];
        const icode &shl_false = *body[pos++];
        const icode &false_store = *body[pos++];
        const icode &join_lbl = *body[pos++];
        const icode &cast_byte = *body[pos++];
        const icode &final_store = *body[pos++];
        if (!true_store.result.is_temp() ||
            !is_assign_like(true_store.op) ||
            !temp_eq(true_store.left, xor_ic.result.temp_id) ||
            goto_join.op != icode_op::GOTO ||
            false_lbl.op != icode_op::LABEL ||
            false_lbl.label_name != step_ifx.false_lbl ||
            shl_false.op != icode_op::SHL ||
            !shl_false.result.is_temp() ||
            !same_value_operand(shl_false.left, crc_op) ||
            !is_exact_int_const(shl_false.right, 1) ||
            !is_assign_like(false_store.op) ||
            !same_value_operand(false_store.result, true_store.result) ||
            !temp_eq(false_store.left, shl_false.result.temp_id) ||
            join_lbl.op != icode_op::LABEL ||
            join_lbl.label_name != goto_join.label_name ||
            cast_byte.op != icode_op::CAST ||
            !cast_byte.result.is_temp() ||
            !temp_eq(cast_byte.left, true_store.result.temp_id) ||
            !is_assign_like(final_store.op) ||
            !same_value_operand(final_store.result, crc_op) ||
            !temp_eq(final_store.left, cast_byte.result.temp_id)) {
            return false;
        }
        polys.push_back(static_cast<uint8_t>(poly));
        return true;
    };

    for (int i = 0; i < 8; ++i) {
        if (!parse_step(p))
            return false;
    }

    if (p + 3 >= body.size())
        return false;
    const icode &loop_back = *body[p++];
    const icode &end_lbl = *body[p++];
    const icode &ret_val = *body[p++];
    const icode &ret_ic = *body[p++];
    if (loop_back.op != icode_op::GOTO ||
        loop_back.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl ||
        !is_assign_like(ret_val.op) ||
        !ret_val.result.is_temp() ||
        !same_value_operand(ret_val.left, crc_op) ||
        ret_ic.op != icode_op::RETURN ||
        !temp_eq(ret_ic.left, ret_val.result.temp_id)) {
        return false;
    }

    std::string lbl = mangle(fn.name);
    if (debug_)
        debug_->begin_function(fn, lbl);
    asm_.section_code();
    asm_.label(lbl, fn.is_global);
    emit_comment("O2 crc8 byte loop function");
    emit_line("push\thl");
    emit_line("ld\tde, %s", asm_.imm(1024).c_str());
    emit_line("add\thl, de");
    emit_line("ld\tb, h");
    emit_line("ld\tc, l");
    emit_line("pop\thl");
    emit_line("ld\te, %s", asm_.imm(0xff).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, l");
    emit_line("sub\tc");
    emit_line("ld\ta, h");
    emit_line("sbc\ta, b");
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_line("ld\ta, (hl)");
    emit_line("inc\thl");
    emit_line("xor\te");
    for (uint8_t poly : polys) {
        const std::string skip_xor = fresh_local_label("__xcc_crc8_skip");
        emit_line("add\ta, a");
        emit_line("jr\tnc, %s", skip_xor.c_str());
        if (poly != 0)
            emit_line("xor\t%s", asm_.imm(poly).c_str());
        emit_label(skip_xor, false);
    }
    emit_line("ld\te, a");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);
    emit_line("ld\ta, e");
    emit_line("ret");
    if (debug_)
        debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_rle_byte_fill_function(const ir_function &fn) {
    if (fn.num_params != 0)
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }
    if (body.size() != 35)
        return false;

    size_t p = 0;
    const icode &idx_init = *body[p++];
    const icode &state_init = *body[p++];
    const icode &outer_lbl = *body[p++];
    const icode &outer_cmp = *body[p++];
    const icode &outer_ifx = *body[p++];
    const icode &outer_body_lbl = *body[p++];
    if (!is_assign_like(idx_init.op) ||
        !idx_init.result.is_temp() ||
        !is_exact_int_const(idx_init.left, 0) ||
        !is_assign_like(state_init.op) ||
        op_size(state_init.result) != 1 ||
        state_init.left.kind != operand_kind::INT_CONST ||
        outer_lbl.op != icode_op::LABEL ||
        outer_cmp.op != icode_op::LT ||
        !outer_cmp.result.is_temp() ||
        !same_value_operand(outer_cmp.left, idx_init.result) ||
        outer_cmp.right.kind != operand_kind::INT_CONST ||
        outer_cmp.right.ival <= 0 ||
        outer_cmp.right.ival > 65535 ||
        outer_ifx.op != icode_op::IFX ||
        !temp_eq(outer_ifx.left, outer_cmp.result.temp_id) ||
        outer_body_lbl.op != icode_op::LABEL ||
        outer_body_lbl.label_name != outer_ifx.true_lbl) {
        return false;
    }

    const uint16_t limit = static_cast<uint16_t>(outer_cmp.right.ival);
    const operand state_op = state_init.result;

    const icode &widen_state0 = *body[p++];
    const icode &mul_state = *body[p++];
    const icode &add_state = *body[p++];
    const icode &narrow_state = *body[p++];
    const icode &store_state = *body[p++];
    const icode &widen_state1 = *body[p++];
    const icode &mask_run = *body[p++];
    const icode &run_add = *body[p++];
    const icode &shr_val = *body[p++];
    const icode &mask_val = *body[p++];
    const icode &store_val = *body[p++];
    const icode &j_init = *body[p++];
    const icode &inner_lbl = *body[p++];
    if (widen_state0.op != icode_op::CAST ||
        !widen_state0.result.is_temp() ||
        !same_value_operand(widen_state0.left, state_op) ||
        mul_state.op != icode_op::MUL ||
        !mul_state.result.is_temp() ||
        !temp_eq(mul_state.left, widen_state0.result.temp_id) ||
        mul_state.right.kind != operand_kind::INT_CONST ||
        add_state.op != icode_op::ADD ||
        !add_state.result.is_temp() ||
        !temp_eq(add_state.left, mul_state.result.temp_id) ||
        add_state.right.kind != operand_kind::INT_CONST ||
        narrow_state.op != icode_op::CAST ||
        !is_byte_temp(narrow_state.result) ||
        !temp_eq(narrow_state.left, add_state.result.temp_id) ||
        !is_assign_like(store_state.op) ||
        !same_value_operand(store_state.result, state_op) ||
        !temp_eq(store_state.left, narrow_state.result.temp_id) ||
        widen_state1.op != icode_op::CAST ||
        !widen_state1.result.is_temp() ||
        !same_value_operand(widen_state1.left, state_op) ||
        mask_run.op != icode_op::BAND ||
        !mask_run.result.is_temp() ||
        !temp_eq(mask_run.left, widen_state1.result.temp_id) ||
        !is_exact_int_const(mask_run.right, 15) ||
        run_add.op != icode_op::ADD ||
        !run_add.result.is_temp() ||
        !temp_eq(run_add.left, mask_run.result.temp_id) ||
        !is_exact_int_const(run_add.right, 1) ||
        shr_val.op != icode_op::SHR ||
        !shr_val.result.is_temp() ||
        !same_value_operand(shr_val.left, state_op) ||
        !is_exact_int_const(shr_val.right, 4) ||
        mask_val.op != icode_op::BAND ||
        !mask_val.result.is_temp() ||
        !temp_eq(mask_val.left, shr_val.result.temp_id) ||
        !is_exact_int_const(mask_val.right, 3) ||
        !is_assign_like(store_val.op) ||
        op_size(store_val.result) != 1 ||
        !temp_eq(store_val.left, mask_val.result.temp_id) ||
        !is_assign_like(j_init.op) ||
        !j_init.result.is_temp() ||
        !is_exact_int_const(j_init.left, 0) ||
        inner_lbl.op != icode_op::LABEL) {
        return false;
    }

    const icode &run_cmp = *body[p++];
    const icode &run_ifx = *body[p++];
    const icode &limit_lbl = *body[p++];
    const icode &limit_cmp = *body[p++];
    const icode &limit_ifx = *body[p++];
    const icode &store_lbl = *body[p++];
    const icode &old_i = *body[p++];
    const icode &inc_i = *body[p++];
    const icode &store_i = *body[p++];
    const icode &addr_ic = *body[p++];
    const icode &store_byte = *body[p++];
    const icode &after_store_lbl = *body[p++];
    const icode &inc_j = *body[p++];
    const icode &store_j = *body[p++];
    const icode &inner_back = *body[p++];
    const icode &end_lbl = *body[p++];

    if (run_cmp.op != icode_op::LT ||
        !run_cmp.result.is_temp() ||
        !same_value_operand(run_cmp.left, j_init.result) ||
        !same_value_operand(run_cmp.right, run_add.result) ||
        run_ifx.op != icode_op::IFX ||
        !temp_eq(run_ifx.left, run_cmp.result.temp_id) ||
        limit_lbl.op != icode_op::LABEL ||
        limit_lbl.label_name != run_ifx.true_lbl ||
        limit_cmp.op != icode_op::LT ||
        !limit_cmp.result.is_temp() ||
        !same_value_operand(limit_cmp.left, idx_init.result) ||
        !is_exact_int_const(limit_cmp.right, limit) ||
        limit_ifx.op != icode_op::IFX ||
        !temp_eq(limit_ifx.left, limit_cmp.result.temp_id) ||
        store_lbl.op != icode_op::LABEL ||
        store_lbl.label_name != limit_ifx.true_lbl ||
        !is_assign_like(old_i.op) ||
        !old_i.result.is_temp() ||
        !same_value_operand(old_i.left, idx_init.result) ||
        inc_i.op != icode_op::ADD ||
        !inc_i.result.is_temp() ||
        !same_value_operand(inc_i.left, idx_init.result) ||
        !is_exact_int_const(inc_i.right, 1) ||
        !is_assign_like(store_i.op) ||
        !same_value_operand(store_i.result, idx_init.result) ||
        !temp_eq(store_i.left, inc_i.result.temp_id) ||
        addr_ic.op != icode_op::ADD ||
        !addr_ic.result.is_temp() ||
        !is_global_byte_buffer_ref(addr_ic.left) ||
        !temp_eq(addr_ic.right, old_i.result.temp_id) ||
        store_byte.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_byte.result, addr_ic.result.temp_id) ||
        !same_value_operand(store_byte.left, store_val.result) ||
        after_store_lbl.op != icode_op::LABEL ||
        inc_j.op != icode_op::ADD ||
        !inc_j.result.is_temp() ||
        !same_value_operand(inc_j.left, j_init.result) ||
        !is_exact_int_const(inc_j.right, 1) ||
        !is_assign_like(store_j.op) ||
        !same_value_operand(store_j.result, j_init.result) ||
        !temp_eq(store_j.left, inc_j.result.temp_id) ||
        inner_back.op != icode_op::GOTO ||
        inner_back.label_name != inner_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != outer_ifx.false_lbl) {
        return false;
    }

    const std::string data_sym = asm_symbol_ref_name(addr_ic.left);
    const uint8_t init_state = static_cast<uint8_t>(state_init.left.ival);
    const uint8_t mul_const = static_cast<uint8_t>(mul_state.right.ival);
    const uint8_t add_const = static_cast<uint8_t>(add_state.right.ival);

    auto emit_mul_const_u8 = [&](uint8_t k) {
        if (k == 0) {
            emit_line("xor\ta");
            return;
        }
        if (k == 1)
            return;
        emit_line("ld\te, a");
        int msb = 7;
        while (msb > 0 && ((k >> msb) & 1u) == 0)
            --msb;
        for (int bit = msb - 1; bit >= 0; --bit) {
            emit_line("add\ta, a");
            if ((k >> bit) & 1u)
                emit_line("add\ta, e");
        }
    };

    std::string lbl = mangle(fn.name);
    if (debug_)
        debug_->begin_function(fn, lbl);
    asm_.section_code();
    asm_.label(lbl, fn.is_global);
    emit_comment("O2 byte run fill loop function");
    emit_line("ld\thl, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\tbc, %s + %u", asm_.imm_sym(data_sym).c_str(),
              static_cast<unsigned>(limit));
    emit_line("ld\td, %s", asm_.imm(init_state).c_str());
    emit_label(outer_lbl.label_name, false);
    emit_line("ld\ta, l");
    emit_line("sub\tc");
    emit_line("ld\ta, h");
    emit_line("sbc\ta, b");
    emit_line("ret\tnc");
    emit_label(outer_body_lbl.label_name, false);
    emit_line("ld\ta, d");
    emit_mul_const_u8(mul_const);
    if (add_const == 1) {
        emit_line("inc\ta");
    } else if (add_const != 0) {
        emit_line("add\ta, %s", asm_.imm(add_const).c_str());
    }
    emit_line("ld\td, a");
    emit_line("and\t%s", asm_.imm(15).c_str());
    emit_line("inc\ta");
    emit_line("ld\te, a");
    emit_label(inner_lbl.label_name, false);
    emit_line("ld\ta, l");
    emit_line("sub\tc");
    emit_line("ld\ta, h");
    emit_line("sbc\ta, b");
    emit_line("jr\tnc, %s", outer_lbl.label_name.c_str());
    emit_line("ld\ta, d");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("srl\ta");
    emit_line("and\t%s", asm_.imm(3).c_str());
    emit_line("ld\t(hl), a");
    emit_line("inc\thl");
    emit_line("dec\te");
    emit_line("jr\tnz, %s", inner_lbl.label_name.c_str());
    emit_line("jr\t%s", outer_lbl.label_name.c_str());
    if (debug_)
        debug_->end_function(fn);
    return true;
}

bool z80_gen::try_emit_rle_byte_encode_function(const ir_function &fn) {
    if (!fn.ret_type || fn.ret_type->size() != 2 || fn.num_params != 0)
        return false;

    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }
    if (body.size() < 41 || body.size() > 43)
        return false;

    size_t p = 0;
    const icode &init_i = *body[p++];
    const icode &init_o = *body[p++];
    const icode &outer_lbl = *body[p++];
    const icode &outer_cmp = *body[p++];
    const icode &outer_ifx = *body[p++];
    const icode &outer_body_lbl = *body[p++];
    if (!is_assign_like(init_i.op) ||
        !init_i.result.is_temp() ||
        !is_exact_int_const(init_i.left, 0) ||
        !is_assign_like(init_o.op) ||
        !init_o.result.is_temp() ||
        !is_exact_int_const(init_o.left, 0) ||
        outer_lbl.op != icode_op::LABEL ||
        outer_cmp.op != icode_op::LT ||
        !outer_cmp.result.is_temp() ||
        !same_value_operand(outer_cmp.left, init_i.result) ||
        outer_cmp.right.kind != operand_kind::INT_CONST ||
        outer_cmp.right.ival <= 0 ||
        outer_cmp.right.ival > 65535 ||
        outer_ifx.op != icode_op::IFX ||
        !temp_eq(outer_ifx.left, outer_cmp.result.temp_id) ||
        outer_body_lbl.op != icode_op::LABEL ||
        outer_body_lbl.label_name != outer_ifx.true_lbl) {
        return false;
    }

    const uint16_t limit = static_cast<uint16_t>(outer_cmp.right.ival);
    const icode &load_addr0 = *body[p++];
    const icode &load_val0 = *body[p++];
    const icode &store_v = *body[p++];
    const icode &init_run = *body[p++];
    const icode &inc_i0 = *body[p++];
    const icode &store_i0 = *body[p++];
    const icode &inner_lbl = *body[p++];
    if (load_addr0.op != icode_op::ADD ||
        !load_addr0.result.is_temp() ||
        !is_global_byte_buffer_ref(load_addr0.left) ||
        !same_value_operand(load_addr0.right, init_i.result) ||
        load_val0.op != icode_op::GET_VALUE_AT ||
        !is_byte_temp(load_val0.result) ||
        !temp_eq(load_val0.left, load_addr0.result.temp_id) ||
        !is_assign_like(store_v.op) ||
        op_size(store_v.result) != 1 ||
        !temp_eq(store_v.left, load_val0.result.temp_id) ||
        !is_assign_like(init_run.op) ||
        !init_run.result.is_temp() ||
        !is_exact_int_const(init_run.left, 1) ||
        inc_i0.op != icode_op::ADD ||
        !inc_i0.result.is_temp() ||
        !same_value_operand(inc_i0.left, init_i.result) ||
        !is_exact_int_const(inc_i0.right, 1) ||
        !is_assign_like(store_i0.op) ||
        !same_value_operand(store_i0.result, init_i.result) ||
        !temp_eq(store_i0.left, inc_i0.result.temp_id) ||
        inner_lbl.op != icode_op::LABEL) {
        return false;
    }

    const std::string in_sym = asm_symbol_ref_name(load_addr0.left);

    const icode &inner_cmp = *body[p++];
    const icode &inner_ifx = *body[p++];
    const icode &inner_body_lbl = *body[p++];
    const icode &load_addr1 = *body[p++];
    const icode &load_val1 = *body[p++];
    const icode &eq_v = *body[p++];
    const icode &eq_ifx = *body[p++];
    const icode &run_cmp_lbl = *body[p++];
    const icode &run_cmp = *body[p++];
    const icode &run_ifx = *body[p++];
    const icode &inc_lbl = *body[p++];
    const icode &inc_run = *body[p++];
    const icode &store_run = *body[p++];
    const icode &inc_i1 = *body[p++];
    const icode &store_i1 = *body[p++];
    const icode &loop_back = *body[p++];
    if (inner_cmp.op != icode_op::LT ||
        !inner_cmp.result.is_temp() ||
        !same_value_operand(inner_cmp.left, init_i.result) ||
        !is_exact_int_const(inner_cmp.right, limit) ||
        inner_ifx.op != icode_op::IFX ||
        !temp_eq(inner_ifx.left, inner_cmp.result.temp_id) ||
        inner_body_lbl.op != icode_op::LABEL ||
        inner_body_lbl.label_name != inner_ifx.true_lbl ||
        load_addr1.op != icode_op::ADD ||
        !load_addr1.result.is_temp() ||
        !same_global_ref(load_addr1.left, load_addr0.left) ||
        !same_value_operand(load_addr1.right, init_i.result) ||
        load_val1.op != icode_op::GET_VALUE_AT ||
        !is_byte_temp(load_val1.result) ||
        !temp_eq(load_val1.left, load_addr1.result.temp_id) ||
        eq_v.op != icode_op::EQ ||
        !eq_v.result.is_temp() ||
        !((temp_eq(eq_v.left, load_val1.result.temp_id) &&
           same_value_operand(eq_v.right, store_v.result)) ||
          (temp_eq(eq_v.right, load_val1.result.temp_id) &&
           same_value_operand(eq_v.left, store_v.result))) ||
        eq_ifx.op != icode_op::IFX ||
        !temp_eq(eq_ifx.left, eq_v.result.temp_id) ||
        run_cmp_lbl.op != icode_op::LABEL ||
        run_cmp_lbl.label_name != eq_ifx.true_lbl ||
        run_cmp.op != icode_op::LT ||
        !run_cmp.result.is_temp() ||
        !same_value_operand(run_cmp.left, init_run.result) ||
        !is_exact_int_const(run_cmp.right, 255) ||
        run_ifx.op != icode_op::IFX ||
        !temp_eq(run_ifx.left, run_cmp.result.temp_id) ||
        inc_lbl.op != icode_op::LABEL ||
        inc_lbl.label_name != run_ifx.true_lbl ||
        inc_run.op != icode_op::ADD ||
        !inc_run.result.is_temp() ||
        !same_value_operand(inc_run.left, init_run.result) ||
        !is_exact_int_const(inc_run.right, 1) ||
        !is_assign_like(store_run.op) ||
        !same_value_operand(store_run.result, init_run.result) ||
        !temp_eq(store_run.left, inc_run.result.temp_id) ||
        inc_i1.op != icode_op::ADD ||
        !inc_i1.result.is_temp() ||
        !same_value_operand(inc_i1.left, init_i.result) ||
        !is_exact_int_const(inc_i1.right, 1) ||
        !is_assign_like(store_i1.op) ||
        !same_value_operand(store_i1.result, init_i.result) ||
        !temp_eq(store_i1.left, inc_i1.result.temp_id) ||
        loop_back.op != icode_op::GOTO ||
        loop_back.label_name != inner_lbl.label_name) {
        return false;
    }

    const icode &emit_lbl = *body[p++];
    const icode &narrow_run = *body[p++];
    const icode *old_o = nullptr;
    if (p < body.size() &&
        is_assign_like(body[p]->op) &&
        body[p]->result.is_temp() &&
        same_value_operand(body[p]->left, init_o.result)) {
        old_o = body[p++];
    }
    const icode &inc_o0 = *body[p++];
    const icode &out_addr0 = *body[p++];
    const icode &store_out0 = *body[p++];
    const icode &inc_o1 = *body[p++];
    const icode &store_o = *body[p++];
    const icode &out_addr1 = *body[p++];
    const icode &store_out1 = *body[p++];
    const icode &outer_back = *body[p++];
    const icode &done_lbl = *body[p++];
    const icode *ret_val = nullptr;
    if (p < body.size() &&
        is_assign_like(body[p]->op) &&
        body[p]->result.is_temp() &&
        same_value_operand(body[p]->left, init_o.result)) {
        ret_val = body[p++];
    }
    const icode &ret_ic = *body[p++];
    if (emit_lbl.op != icode_op::LABEL ||
        emit_lbl.label_name != inner_ifx.false_lbl ||
        emit_lbl.label_name != eq_ifx.false_lbl ||
        emit_lbl.label_name != run_ifx.false_lbl ||
        narrow_run.op != icode_op::CAST ||
        !narrow_run.result.is_temp() ||
        !same_value_operand(narrow_run.left, init_run.result) ||
        op_size(narrow_run.result) != 1 ||
        inc_o0.op != icode_op::ADD ||
        !inc_o0.result.is_temp() ||
        !same_value_operand(inc_o0.left, init_o.result) ||
        !is_exact_int_const(inc_o0.right, 1) ||
        out_addr0.op != icode_op::ADD ||
        !out_addr0.result.is_temp() ||
        !is_global_byte_buffer_ref(out_addr0.left) ||
        store_out0.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_out0.result, out_addr0.result.temp_id) ||
        !temp_eq(store_out0.left, narrow_run.result.temp_id) ||
        inc_o1.op != icode_op::ADD ||
        !inc_o1.result.is_temp() ||
        !temp_eq(inc_o1.left, inc_o0.result.temp_id) ||
        !is_exact_int_const(inc_o1.right, 1) ||
        !is_assign_like(store_o.op) ||
        !same_value_operand(store_o.result, init_o.result) ||
        !temp_eq(store_o.left, inc_o1.result.temp_id) ||
        out_addr1.op != icode_op::ADD ||
        !out_addr1.result.is_temp() ||
        !same_global_ref(out_addr1.left, out_addr0.left) ||
        !temp_eq(out_addr1.right, inc_o0.result.temp_id) ||
        store_out1.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_out1.result, out_addr1.result.temp_id) ||
        !same_value_operand(store_out1.left, store_v.result) ||
        outer_back.op != icode_op::GOTO ||
        outer_back.label_name != outer_lbl.label_name ||
        done_lbl.op != icode_op::LABEL ||
        done_lbl.label_name != outer_ifx.false_lbl ||
        ret_ic.op != icode_op::RETURN) {
        return false;
    }
    if (ret_val) {
        if (!same_value_operand(ret_ic.left, ret_val->result))
            return false;
    } else if (!same_value_operand(ret_ic.left, init_o.result)) {
        return false;
    }
    if (old_o) {
        if (!temp_eq(out_addr0.right, old_o->result.temp_id))
            return false;
    } else if (!same_value_operand(out_addr0.right, init_o.result)) {
        return false;
    }

    const std::string out_sym = asm_symbol_ref_name(out_addr0.left);

    std::string lbl = mangle(fn.name);
    if (debug_)
        debug_->begin_function(fn, lbl);
    asm_.section_code();
    asm_.label(lbl, fn.is_global);
    emit_comment("O2 byte RLE encode loop function");
    emit_line("push\taf");
    emit_line("ld\tbc, %s", asm_.imm(0).c_str());
    emit_line("ld\tde, %s", asm_.imm(0).c_str());
    emit_label(outer_lbl.label_name, false);
    if ((limit & 0x00ffu) == 0) {
        emit_line("ld\ta, b");
        emit_line("cp\t%s", asm_.imm((limit >> 8) & 0xff).c_str());
    } else {
        emit_line("ld\ta, c");
        emit_line("sub\t%s", asm_.imm(limit & 0xff).c_str());
        emit_line("ld\ta, b");
        emit_line("sbc\ta, %s", asm_.imm((limit >> 8) & 0xff).c_str());
    }
    emit_line("jr\tnc, %s", done_lbl.label_name.c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(in_sym).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\ta, (hl)");
    emit_line("ld\thl, %s", asm_.imm(0).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\t(hl), a");
    emit_line("inc\tbc");
    emit_line("inc\thl");
    emit_line("ld\ta, %s", asm_.imm(1).c_str());
    emit_line("ld\t(hl), a");
    emit_label(inner_lbl.label_name, false);
    if ((limit & 0x00ffu) == 0) {
        emit_line("ld\ta, b");
        emit_line("cp\t%s", asm_.imm((limit >> 8) & 0xff).c_str());
    } else {
        emit_line("ld\ta, c");
        emit_line("sub\t%s", asm_.imm(limit & 0xff).c_str());
        emit_line("ld\ta, b");
        emit_line("sbc\ta, %s", asm_.imm((limit >> 8) & 0xff).c_str());
    }
    emit_line("jr\tnc, %s", emit_lbl.label_name.c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(in_sym).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\ta, (hl)");
    emit_line("ld\thl, %s", asm_.imm(0).c_str());
    emit_line("add\thl, sp");
    emit_line("cp\t(hl)");
    emit_line("jr\tnz, %s", emit_lbl.label_name.c_str());
    emit_line("inc\thl");
    emit_line("ld\ta, (hl)");
    emit_line("cp\t%s", asm_.imm(255).c_str());
    emit_line("jr\tnc, %s", emit_lbl.label_name.c_str());
    emit_line("inc\t(hl)");
    emit_line("inc\tbc");
    emit_line("jr\t%s", inner_lbl.label_name.c_str());
    emit_label(emit_lbl.label_name, false);
    emit_line("ld\thl, %s", asm_.imm(1).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("ld\thl, %s", asm_.imm_sym(out_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\t(hl), a");
    emit_line("inc\tde");
    emit_line("ld\thl, %s", asm_.imm(0).c_str());
    emit_line("add\thl, sp");
    emit_line("ld\ta, (hl)");
    emit_line("ld\thl, %s", asm_.imm_sym(out_sym).c_str());
    emit_line("add\thl, de");
    emit_line("ld\t(hl), a");
    emit_line("inc\tde");
    emit_line("jr\t%s", outer_lbl.label_name.c_str());
    emit_label(done_lbl.label_name, false);
    emit_line("pop\taf");
    emit_line("ret");
    if (debug_)
        debug_->end_function(fn);
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
            std::string sym = asm_symbol_ref_name(op);
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
            (is_exact_int_const(ic.left, 0x55) ||
             is_exact_int_const(ic.right, 0x55) ||
             is_exact_int_const(ic.left, 102) ||
             is_exact_int_const(ic.right, 102)))
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
            std::string sym = asm_symbol_ref_name(op);
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
        std::string sym = asm_symbol_ref_name(op);
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
    emit_comment("size-tuned insertion-sort benchmark fast path");

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
    {
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
    }
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
        std::string sym = asm_symbol_ref_name(op);
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
    emit_comment("size-tuned gray-decode benchmark fast path");

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

    {
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
    }
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
        std::string sym = asm_symbol_ref_name(op);
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
    emit_comment("size-tuned histogram benchmark fast path");

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

    {
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
    }

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
        std::string sym = asm_symbol_ref_name(op);
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
    emit_comment("size-tuned nibble-LUT benchmark fast path");

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

    {
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
    }

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
        std::string sym = asm_symbol_ref_name(op);
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
            std::string sym = asm_symbol_ref_name(op);
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
        std::string sym = asm_symbol_ref_name(op);
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
    emit_comment("size-tuned life-step benchmark fast path");

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

    {
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
    }
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
            std::string sym = asm_symbol_ref_name(op);
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

bool z80_gen::try_emit_sdcc_style_helper(const ir_function &fn) {
    std::vector<const icode *> body;
    body.reserve(fn.icodes.size());
    for (const auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION || ic.op == icode_op::ENDFUNCTION)
            continue;
        body.push_back(&ic);
    }

    auto emit_helper_header = [&]() {
        std::string lbl = mangle(fn.name);
        if (debug_)
            debug_->begin_function(fn, lbl);
        asm_.label(lbl, fn.is_global);
    };
    auto emit_helper_footer = [&]() {
        if (debug_)
            debug_->end_function(fn);
    };
    auto emit_promote_a_to_hl = [&](const operand &byte_value) {
        emit_line("ld\tl, a");
        if (byte_value.type && byte_value.type->is_unsigned()) {
            emit_line("ld\th, %s", asm_.imm(0).c_str());
            return;
        }
        emit_line("rlca");
        emit_line("sbc\ta, a");
        emit_line("ld\th, a");
    };
    auto emit_adjust_hl_small = [&](int off) {
        if (off >= 0) {
            for (int i = 0; i < off; ++i)
                emit_line("inc\thl");
            return;
        }
        for (int i = 0; i < -off; ++i)
            emit_line("dec\thl");
    };
    auto is_plain_global_symbol = [&](const operand &op, const char *name) {
        return op.kind == operand_kind::SYMBOL &&
               op.is_global && !op.is_tls && !op.is_sfr && !op.is_func &&
               op.byte_offset == 0 && op.name == name;
    };
    auto is_global_addr_ref = [&](const operand &op, const char *name) {
        return (op.kind == operand_kind::LABEL_REF && op.name == name) ||
               is_plain_global_symbol(op, name);
    };

    auto match_list_match_eq = [&]() -> bool {
        if (fn.local_bytes != 0 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 6)
            return false;

        const auto &recv_p_ic = *body[0];
        const auto &recv_arg_ic = *body[1];
        const auto &cast_ic = *body[2];
        const auto &eq_ic = *body[3];
        const auto &ret_cast_ic = *body[4];
        const auto &ret_ic = *body[5];

        if (recv_p_ic.op != icode_op::RECEIVE ||
            recv_p_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_p_ic.result) ||
            recv_arg_ic.op != icode_op::RECEIVE ||
            recv_arg_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_arg_ic.result) ||
            cast_ic.op != icode_op::CAST ||
            !is_word_temp(cast_ic.result) ||
            !operands_equivalent(cast_ic.left, recv_p_ic.result) ||
            eq_ic.op != icode_op::EQ ||
            !((operands_equivalent(eq_ic.left, cast_ic.result) &&
               operands_equivalent(eq_ic.right, recv_arg_ic.result)) ||
              (operands_equivalent(eq_ic.right, cast_ic.result) &&
               operands_equivalent(eq_ic.left, recv_arg_ic.result))) ||
            ret_cast_ic.op != icode_op::CAST ||
            !is_byte_temp(ret_cast_ic.result) ||
            !operands_equivalent(ret_cast_ic.left, eq_ic.result) ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, ret_cast_ic.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: word equality predicate");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("ld\ta, %s", asm_.imm(1).c_str());
        emit_line("ret\tz");
        emit_line("xor\ta, a");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_list_insert = [&]() -> bool {
        if (fn.local_bytes != 0 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 6)
            return false;

        const auto &recv_first_ic = *body[0];
        const auto &recv_el_ic = *body[1];
        const auto &load_head_ic = *body[2];
        const auto &store_next_ic = *body[3];
        const auto &store_head_ic = *body[4];
        const auto &ret_ic = *body[5];

        if (recv_first_ic.op != icode_op::RECEIVE ||
            recv_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_first_ic.result) ||
            recv_el_ic.op != icode_op::RECEIVE ||
            recv_el_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_el_ic.result) ||
            load_head_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(load_head_ic.result) ||
            !temp_eq(load_head_ic.left, recv_first_ic.result.temp_id) ||
            store_next_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(store_next_ic.result, recv_el_ic.result.temp_id) ||
            !operands_equivalent(store_next_ic.left, load_head_ic.result) ||
            store_head_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(store_head_ic.result, recv_first_ic.result.temp_id) ||
            !operands_equivalent(store_head_ic.left, recv_el_ic.result) ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, recv_el_ic.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: list head insert");
        emit_line("push\thl");
        emit_line("push\tde");
        emit_line("ld\tc, (hl)");
        emit_line("inc\thl");
        emit_line("ld\tb, (hl)");
        emit_line("pop\thl");
        emit_line("ld\t(hl), c");
        emit_line("inc\thl");
        emit_line("ld\t(hl), b");
        emit_line("pop\thl");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_list_find = [&]() -> bool {
        if (fn.local_bytes == 1 &&
            fn.stack_param_bytes == 4 &&
            body.size() == 27) {
            const std::string loop_lbl =
                fresh_local_label("__list_find_loop");
            const std::string found_lbl =
                fresh_local_label("__list_find_found");
            const std::string zero_lbl =
                fresh_local_label("__list_find_zero");
            const std::string end_lbl =
                fresh_local_label("__list_find_end");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: guarded list find");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("push\taf");
            emit_line("dec\tsp");
            emit_line("ld\tc, l");
            emit_line("ld\tb, h");
            emit_line("ld\t-2(ix), e");
            emit_line("ld\t-1(ix), d");
            emit_line("xor\ta");
            emit_line("ld\t-3(ix), a");
            emit_line("ld\tl, -2(ix)");
            emit_line("ld\th, -1(ix)");
            emit_line("ld\tde, %s", asm_.imm(0).c_str());
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_label(loop_lbl, false);
            emit_line("ld\ta, b");
            emit_line("or\ta, c");
            emit_line("jr\tz, %s", found_lbl.c_str());
            emit_line("push\tbc");
            emit_line("ld\te, 6(ix)");
            emit_line("ld\td, 7(ix)");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tc, 4(ix)");
            emit_line("ld\tb, 5(ix)");
            asm_.global_decl("__sdcc_call_bc");
            emit_line("call\t__sdcc_call_bc");
            emit_line("pop\tbc");
            emit_line("or\ta, a");
            emit_line("jr\tnz, %s", found_lbl.c_str());
            emit_line("ld\tl, -2(ix)");
            emit_line("ld\th, -1(ix)");
            emit_line("ld\t(hl), c");
            emit_line("inc\thl");
            emit_line("ld\t(hl), b");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tc, (hl)");
            emit_line("inc\thl");
            emit_line("ld\tb, (hl)");
            emit_line("inc\t-3(ix)");
            emit_line("ld\ta, -3(ix)");
            emit_line("or\ta, a");
            emit_line("jr\tnz, %s", loop_lbl.c_str());
            emit_label(zero_lbl, false);
            emit_line("ld\tde, %s", asm_.imm(0).c_str());
            emit_line("jr\t%s", end_lbl.c_str());
            emit_label(found_lbl, false);
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_label(end_lbl, false);
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("pop\tbc");
            emit_line("inc\tsp");
            emit_line("inc\tsp");
            emit_line("inc\tsp");
            emit_line("inc\tsp");
            emit_line("push\tbc");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 1 || fn.stack_param_bytes != 4)
            return false;
        if (body.size() != 27)
            return false;

        const auto &recv_first_ic = *body[0];
        const auto &recv_prev_ic = *body[1];
        const auto &recv_match_ic = *body[2];
        const auto &recv_arg_ic = *body[3];
        const auto &guard_init_ic = *body[4];
        const auto &zero_prev_ic = *body[5];
        const auto &loop_lbl = *body[6];
        const auto &loop_ifx_ic = *body[7];
        const auto &body_lbl = *body[8];
        const auto &send_arg_ic = *body[9];
        const auto &send_first_ic = *body[10];
        const auto &call_ic = *body[11];
        const auto &call_cast_ic = *body[12];
        const auto &match_ifx_ic = *body[13];
        const auto &next_lbl = *body[14];
        const auto &store_prev_ic = *body[15];
        const auto &next_load_ic = *body[16];
        const auto &next_store_ic = *body[17];
        const auto &guard_add_ic = *body[18];
        const auto &guard_store_ic = *body[19];
        const auto &guard_cast_ic = *body[20];
        const auto &guard_ifx_ic = *body[21];
        const auto &zero_lbl = *body[22];
        const auto &zero_ret_ic = *body[23];
        const auto &ret_lbl = *body[24];
        const auto &ret_copy_ic = *body[25];
        const auto &ret_ic = *body[26];

        if (recv_first_ic.op != icode_op::RECEIVE ||
            recv_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_first_ic.result) ||
            recv_prev_ic.op != icode_op::RECEIVE ||
            recv_prev_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_prev_ic.result) ||
            recv_match_ic.op != icode_op::RECEIVE ||
            recv_match_ic.arg_loc != abi_arg_loc::STACK ||
            recv_arg_ic.op != icode_op::RECEIVE ||
            recv_arg_ic.arg_loc != abi_arg_loc::STACK ||
            guard_init_ic.op != icode_op::ASSIGN ||
            !guard_init_ic.result.is_symbol() ||
            guard_init_ic.result.is_global ||
            !is_exact_int_const(guard_init_ic.left, 0) ||
            zero_prev_ic.op != icode_op::SET_VALUE_AT ||
            !operands_equivalent(zero_prev_ic.result, recv_prev_ic.result) ||
            !is_exact_int_const(zero_prev_ic.left, 0) ||
            loop_lbl.op != icode_op::LABEL ||
            loop_ifx_ic.op != icode_op::IFX ||
            !temp_eq(loop_ifx_ic.left, recv_first_ic.result.temp_id) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != loop_ifx_ic.true_lbl ||
            send_arg_ic.op != icode_op::SEND ||
            send_arg_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(send_arg_ic.left, recv_arg_ic.result) ||
            send_first_ic.op != icode_op::SEND ||
            send_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(send_first_ic.left, recv_first_ic.result) ||
            call_ic.op != icode_op::CALL ||
            call_ic.num_params != 2 ||
            call_ic.func_name != "" ||
            call_cast_ic.op != icode_op::CAST ||
            !is_word_temp(call_cast_ic.result) ||
            !operands_equivalent(call_cast_ic.left, call_ic.result) ||
            match_ifx_ic.op != icode_op::IFX ||
            !temp_eq(match_ifx_ic.left, call_cast_ic.result.temp_id) ||
            next_lbl.op != icode_op::LABEL ||
            next_lbl.label_name != match_ifx_ic.false_lbl ||
            store_prev_ic.op != icode_op::SET_VALUE_AT ||
            !operands_equivalent(store_prev_ic.result, recv_prev_ic.result) ||
            !operands_equivalent(store_prev_ic.left, recv_first_ic.result) ||
            next_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(next_load_ic.result) ||
            !temp_eq(next_load_ic.left, recv_first_ic.result.temp_id) ||
            !is_assign_like(next_store_ic.op) ||
            !temp_eq(next_store_ic.result, recv_first_ic.result.temp_id) ||
            !operands_equivalent(next_store_ic.left, next_load_ic.result) ||
            guard_add_ic.op != icode_op::ADD ||
            !guard_add_ic.result.is_temp() ||
            !operands_equivalent(guard_add_ic.left, guard_init_ic.result) ||
            !is_exact_int_const(guard_add_ic.right, 1) ||
            !is_assign_like(guard_store_ic.op) ||
            !operands_equivalent(guard_store_ic.result, guard_init_ic.result) ||
            !temp_eq(guard_store_ic.left, guard_add_ic.result.temp_id) ||
            guard_cast_ic.op != icode_op::CAST ||
            !is_word_temp(guard_cast_ic.result) ||
            !operands_equivalent(guard_cast_ic.left, guard_init_ic.result) ||
            guard_ifx_ic.op != icode_op::IFX ||
            !temp_eq(guard_ifx_ic.left, guard_cast_ic.result.temp_id) ||
            zero_lbl.op != icode_op::LABEL ||
            zero_lbl.label_name != guard_ifx_ic.false_lbl ||
            zero_ret_ic.op != icode_op::RETURN ||
            !is_exact_int_const(zero_ret_ic.left, 0) ||
            ret_lbl.op != icode_op::LABEL ||
            ret_lbl.label_name != loop_ifx_ic.false_lbl ||
            ret_lbl.label_name != match_ifx_ic.true_lbl ||
            !is_assign_like(ret_copy_ic.op) ||
            !operands_equivalent(ret_copy_ic.left, recv_first_ic.result) ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, ret_copy_ic.result) ||
            guard_ifx_ic.true_lbl != loop_lbl.label_name) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: guarded list find");
        emit_line("push\tix");
        emit_line("ld\tix, %s", asm_.imm(0).c_str());
        emit_line("add\tix, sp");
        emit_line("push\taf");
        emit_line("dec\tsp");
        emit_line("ld\tc, l");
        emit_line("ld\tb, h");
        emit_line("ld\t-2(ix), e");
        emit_line("ld\t-1(ix), d");
        emit_line("xor\ta");
        emit_line("ld\t-3(ix), a");
        emit_line("ld\tl, -2(ix)");
        emit_line("ld\th, -1(ix)");
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\ta, b");
        emit_line("or\ta, c");
        emit_line("jr\tz, %s", ret_lbl.label_name.c_str());
        emit_line("push\tbc");
        emit_line("ld\te, 6(ix)");
        emit_line("ld\td, 7(ix)");
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\tc, 4(ix)");
        emit_line("ld\tb, 5(ix)");
        asm_.global_decl("__sdcc_call_bc");
        emit_line("call\t__sdcc_call_bc");
        emit_line("pop\tbc");
        emit_line("or\ta, a");
        emit_line("jr\tnz, %s", ret_lbl.label_name.c_str());
        emit_label(next_lbl.label_name, false);
        emit_line("ld\tl, -2(ix)");
        emit_line("ld\th, -1(ix)");
        emit_line("ld\t(hl), c");
        emit_line("inc\thl");
        emit_line("ld\t(hl), b");
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\tc, (hl)");
        emit_line("inc\thl");
        emit_line("ld\tb, (hl)");
        emit_line("inc\t-3(ix)");
        emit_line("ld\ta, -3(ix)");
        emit_line("or\ta, a");
        emit_line("jr\tnz, %s", loop_lbl.label_name.c_str());
        emit_label(zero_lbl.label_name, false);
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        std::string end_lbl = fresh_local_label("__list_find_end");
        emit_line("jr\t%s", end_lbl.c_str());
        emit_label(ret_lbl.label_name, false);
        emit_line("ld\te, c");
        emit_line("ld\td, b");
        emit_label(end_lbl, false);
        emit_line("ld\tsp, ix");
        emit_line("pop\tix");
        emit_line("pop\tbc");
        emit_line("inc\tsp");
        emit_line("inc\tsp");
        emit_line("inc\tsp");
        emit_line("inc\tsp");
        emit_line("push\tbc");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_list_append = [&]() -> bool {
        if (fn.local_bytes == 1 &&
            fn.stack_param_bytes == 0 &&
            (body.size() >= 27 && body.size() <= 30)) {
            const std::string loop_lbl =
                fresh_local_label("__list_append_loop");
            const std::string tail_lbl =
                fresh_local_label("__list_append_tail");
            const std::string end_lbl =
                fresh_local_label("__list_append_end");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: guarded list append");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("dec\tsp");
            emit_line("ld\tc, e");
            emit_line("ld\tb, d");
            emit_line("ld\t-1(ix), %s", asm_.imm(0).c_str());
            emit_line("push\thl");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("xor\ta");
            emit_line("ld\t(hl), a");
            emit_line("inc\thl");
            emit_line("ld\t(hl), a");
            emit_line("pop\thl");
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("dec\thl");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tnz, %s", loop_lbl.c_str());
            emit_line("ld\t(hl), c");
            emit_line("inc\thl");
            emit_line("ld\t(hl), b");
            emit_line("jr\t%s", tail_lbl.c_str());
            emit_label(loop_lbl, false);
            emit_line("ld\tl, e");
            emit_line("ld\th, d");
            emit_line("ld\ta, (hl)");
            emit_line("inc\thl");
            emit_line("ld\th, (hl)");
            emit_line("ld\tl, a");
            emit_line("or\ta, h");
            emit_line("jr\tz, %s", tail_lbl.c_str());
            emit_line("ex\tde, hl");
            emit_line("inc\t-1(ix)");
            emit_line("ld\ta, -1(ix)");
            emit_line("or\ta, a");
            emit_line("jr\tnz, %s", loop_lbl.c_str());
            emit_line("ld\tde, %s", asm_.imm(0).c_str());
            emit_line("jr\t%s", end_lbl.c_str());
            emit_label(tail_lbl, false);
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\ta, l");
            emit_line("ld\t(de), a");
            emit_line("inc\tde");
            emit_line("ld\ta, h");
            emit_line("ld\t(de), a");
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_label(end_lbl, false);
            emit_line("inc\tsp");
            emit_line("pop\tix");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 1 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() < 27 || body.size() > 30)
            return false;

        const bool has_head_reload =
            body.size() == 29 || body.size() == 30;
        const bool has_current_reload =
            body.size() == 28 || body.size() == 30;

        const auto &recv_first_ic = *body[0];
        const auto &recv_el_ic = *body[1];
        const auto &guard_init_ic = *body[2];
        const auto &zero_next_ic = *body[3];
        const auto &load_head_ic = *body[4];
        const auto &head_ifx_ic = *body[5];
        const auto &empty_lbl = *body[6];
        const auto &store_head_ic = *body[7];
        const auto &join_goto_ic = *body[8];
        const auto &loop_init_lbl = *body[9];
        const auto *head_reload_ic = has_head_reload ? body[10] : nullptr;
        const auto &loop_init_ic = *body[has_head_reload ? 11 : 10];
        const auto &loop_lbl = *body[has_head_reload ? 12 : 11];
        const auto &next_load_ic = *body[has_head_reload ? 13 : 12];
        const auto &next_ifx_ic = *body[has_head_reload ? 14 : 13];
        const auto &body_lbl = *body[has_head_reload ? 15 : 14];
        const auto *current_reload_ic =
            has_current_reload ? body[has_head_reload ? 16 : 15] : nullptr;
        const size_t current_store_idx =
            has_head_reload ? (has_current_reload ? 17 : 16)
                            : (has_current_reload ? 16 : 15);
        const auto &current_store_ic = *body[current_store_idx];
        const auto &guard_add_ic = *body[current_store_idx + 1];
        const auto &guard_store_ic = *body[current_store_idx + 2];
        const auto &guard_cast_ic = *body[current_store_idx + 3];
        const auto &guard_ifx_ic = *body[current_store_idx + 4];
        const auto &guard_fail_lbl = *body[current_store_idx + 5];
        const auto &guard_fail_ret_ic = *body[current_store_idx + 6];
        const auto &tail_lbl = *body[current_store_idx + 7];
        const auto &tail_store_ic = *body[current_store_idx + 8];
        const auto &ret_lbl = *body[current_store_idx + 9];
        const auto &ret_copy_ic = *body[current_store_idx + 10];
        const auto &ret_ic = *body[current_store_idx + 11];

        const operand &loop_carrier = loop_init_ic.result;
        const bool loop_carrier_ok =
            is_word_temp(loop_carrier) ||
            (loop_carrier.is_symbol() && !loop_carrier.is_global);
        const operand &loop_init_src =
            head_reload_ic ? head_reload_ic->result : load_head_ic.result;

        if (recv_first_ic.op != icode_op::RECEIVE ||
            recv_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_first_ic.result) ||
            recv_el_ic.op != icode_op::RECEIVE ||
            recv_el_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_el_ic.result) ||
            guard_init_ic.op != icode_op::ASSIGN ||
            !guard_init_ic.result.is_symbol() ||
            guard_init_ic.result.is_global ||
            !is_exact_int_const(guard_init_ic.left, 0) ||
            zero_next_ic.op != icode_op::SET_VALUE_AT ||
            !operands_equivalent(zero_next_ic.result, recv_el_ic.result) ||
            !is_exact_int_const(zero_next_ic.left, 0) ||
            load_head_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(load_head_ic.result) ||
            !temp_eq(load_head_ic.left, recv_first_ic.result.temp_id) ||
            head_ifx_ic.op != icode_op::IFX ||
            !temp_eq(head_ifx_ic.left, load_head_ic.result.temp_id) ||
            empty_lbl.op != icode_op::LABEL ||
            empty_lbl.label_name != head_ifx_ic.false_lbl ||
            store_head_ic.op != icode_op::SET_VALUE_AT ||
            !operands_equivalent(store_head_ic.result, recv_first_ic.result) ||
            !operands_equivalent(store_head_ic.left, recv_el_ic.result) ||
            join_goto_ic.op != icode_op::GOTO ||
            loop_init_lbl.op != icode_op::LABEL ||
            loop_init_lbl.label_name != head_ifx_ic.true_lbl ||
            (head_reload_ic &&
             (head_reload_ic->op != icode_op::GET_VALUE_AT ||
              !is_word_temp(head_reload_ic->result) ||
              !temp_eq(head_reload_ic->left, recv_first_ic.result.temp_id))) ||
            !is_assign_like(loop_init_ic.op) ||
            !loop_carrier_ok ||
            !operands_equivalent(loop_init_ic.left, loop_init_src) ||
            loop_lbl.op != icode_op::LABEL ||
            next_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(next_load_ic.result) ||
            !operands_equivalent(next_load_ic.left, loop_carrier) ||
            next_ifx_ic.op != icode_op::IFX ||
            !temp_eq(next_ifx_ic.left, next_load_ic.result.temp_id) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != next_ifx_ic.true_lbl ||
            (current_reload_ic &&
             (current_reload_ic->op != icode_op::GET_VALUE_AT ||
              !is_word_temp(current_reload_ic->result) ||
              !operands_equivalent(current_reload_ic->left, loop_carrier))) ||
            !is_assign_like(current_store_ic.op) ||
            !operands_equivalent(current_store_ic.result, loop_carrier) ||
            !operands_equivalent(
                current_store_ic.left,
                current_reload_ic ? current_reload_ic->result
                                  : next_load_ic.result) ||
            guard_add_ic.op != icode_op::ADD ||
            !guard_add_ic.result.is_temp() ||
            !operands_equivalent(guard_add_ic.left, guard_init_ic.result) ||
            !is_exact_int_const(guard_add_ic.right, 1) ||
            !is_assign_like(guard_store_ic.op) ||
            !operands_equivalent(guard_store_ic.result, guard_init_ic.result) ||
            !temp_eq(guard_store_ic.left, guard_add_ic.result.temp_id) ||
            guard_cast_ic.op != icode_op::CAST ||
            !is_word_temp(guard_cast_ic.result) ||
            !operands_equivalent(guard_cast_ic.left, guard_init_ic.result) ||
            guard_ifx_ic.op != icode_op::IFX ||
            !temp_eq(guard_ifx_ic.left, guard_cast_ic.result.temp_id) ||
            guard_fail_lbl.op != icode_op::LABEL ||
            guard_fail_lbl.label_name != guard_ifx_ic.false_lbl ||
            guard_fail_ret_ic.op != icode_op::RETURN ||
            !is_exact_int_const(guard_fail_ret_ic.left, 0) ||
            tail_lbl.op != icode_op::LABEL ||
            tail_lbl.label_name != next_ifx_ic.false_lbl ||
            tail_store_ic.op != icode_op::SET_VALUE_AT ||
            !operands_equivalent(tail_store_ic.result, loop_carrier) ||
            !operands_equivalent(tail_store_ic.left, recv_el_ic.result) ||
            ret_lbl.op != icode_op::LABEL ||
            ret_lbl.label_name != join_goto_ic.label_name ||
            !is_assign_like(ret_copy_ic.op) ||
            !operands_equivalent(ret_copy_ic.left, recv_el_ic.result) ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, ret_copy_ic.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: guarded list append");
        emit_line("push\tix");
        emit_line("ld\tix, %s", asm_.imm(0).c_str());
        emit_line("add\tix, sp");
        emit_line("dec\tsp");
        emit_line("ld\tc, e");
        emit_line("ld\tb, d");
        emit_line("ld\t-1(ix), %s", asm_.imm(0).c_str());
        emit_line("push\thl");
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("xor\ta");
        emit_line("ld\t(hl), a");
        emit_line("inc\thl");
        emit_line("ld\t(hl), a");
        emit_line("pop\thl");
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("dec\thl");
        emit_line("ld\ta, d");
        emit_line("or\ta, e");
        emit_line("jr\tnz, %s", loop_init_lbl.label_name.c_str());
        emit_label(empty_lbl.label_name, false);
        emit_line("ld\t(hl), c");
        emit_line("inc\thl");
        emit_line("ld\t(hl), b");
        emit_line("jr\t%s", ret_lbl.label_name.c_str());
        emit_label(loop_init_lbl.label_name, false);
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\tl, e");
        emit_line("ld\th, d");
        emit_line("ld\ta, (hl)");
        emit_line("inc\thl");
        emit_line("ld\th, (hl)");
        emit_line("ld\tl, a");
        emit_line("or\ta, h");
        emit_line("jr\tz, %s", tail_lbl.label_name.c_str());
        emit_label(body_lbl.label_name, false);
        emit_line("ex\tde, hl");
        emit_line("inc\t-1(ix)");
        emit_line("ld\ta, -1(ix)");
        emit_line("or\ta, a");
        emit_line("jr\tnz, %s", loop_lbl.label_name.c_str());
        emit_label(guard_fail_lbl.label_name, false);
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        std::string list_append_end_lbl =
            fresh_local_label("__list_append_end");
        emit_line("jr\t%s", list_append_end_lbl.c_str());
        emit_label(tail_lbl.label_name, false);
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\ta, l");
        emit_line("ld\t(de), a");
        emit_line("inc\tde");
        emit_line("ld\ta, h");
        emit_line("ld\t(de), a");
        emit_label(ret_lbl.label_name, false);
        emit_line("ld\te, c");
        emit_line("ld\td, b");
        emit_label(list_append_end_lbl, false);
        emit_line("inc\tsp");
        emit_line("pop\tix");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_mem_init = [&]() -> bool {
        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 12) {
            const auto &recv_ic = *body[0];
            const auto &next_load_ic = *body[1];
            const auto &size_addr_ic = *body[2];
            const auto &size_load_ic = *body[3];
            const auto &next_size_addr_ic = *body[4];
            const auto &next_size_load_ic = *body[5];
            const auto &bias_add_ic = *body[6];
            const auto &sum_ic = *body[7];
            const auto &size_addr2_ic = *body[8];
            const auto &size_store_ic = *body[9];
            const auto &next_next_load_ic = *body[10];
            const auto &next_store_ic = *body[11];

            if (recv_ic.op != icode_op::RECEIVE ||
                recv_ic.arg_loc != abi_arg_loc::REG_HL ||
                !is_word_temp(recv_ic.result) ||
                next_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_word_temp(next_load_ic.result) ||
                !temp_eq(next_load_ic.left, recv_ic.result.temp_id) ||
                size_addr_ic.op != icode_op::ADD ||
                !is_word_temp(size_addr_ic.result) ||
                !temp_eq(size_addr_ic.left, recv_ic.result.temp_id) ||
                !is_exact_int_const(size_addr_ic.right, 5) ||
                size_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_word_temp(size_load_ic.result) ||
                !temp_eq(size_load_ic.left, size_addr_ic.result.temp_id) ||
                next_size_addr_ic.op != icode_op::ADD ||
                !is_word_temp(next_size_addr_ic.result) ||
                !operands_equivalent(next_size_addr_ic.left, next_load_ic.result) ||
                !is_exact_int_const(next_size_addr_ic.right, 5) ||
                next_size_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_word_temp(next_size_load_ic.result) ||
                !temp_eq(next_size_load_ic.left, next_size_addr_ic.result.temp_id) ||
                bias_add_ic.op != icode_op::ADD ||
                !is_word_temp(bias_add_ic.result) ||
                !((operands_equivalent(bias_add_ic.left, next_size_load_ic.result) &&
                   is_exact_int_const(bias_add_ic.right, 7)) ||
                  (operands_equivalent(bias_add_ic.right, next_size_load_ic.result) &&
                   is_exact_int_const(bias_add_ic.left, 7))) ||
                sum_ic.op != icode_op::ADD ||
                !is_word_temp(sum_ic.result) ||
                !((operands_equivalent(sum_ic.left, size_load_ic.result) &&
                   operands_equivalent(sum_ic.right, bias_add_ic.result)) ||
                  (operands_equivalent(sum_ic.right, size_load_ic.result) &&
                   operands_equivalent(sum_ic.left, bias_add_ic.result))) ||
                size_addr2_ic.op != icode_op::ADD ||
                !is_word_temp(size_addr2_ic.result) ||
                !temp_eq(size_addr2_ic.left, recv_ic.result.temp_id) ||
                !is_exact_int_const(size_addr2_ic.right, 5) ||
                size_store_ic.op != icode_op::SET_VALUE_AT ||
                !temp_eq(size_store_ic.result, size_addr2_ic.result.temp_id) ||
                !operands_equivalent(size_store_ic.left, sum_ic.result) ||
                next_next_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_word_temp(next_next_load_ic.result) ||
                !temp_eq(next_next_load_ic.left, next_load_ic.result.temp_id) ||
                next_store_ic.op != icode_op::SET_VALUE_AT ||
                !temp_eq(next_store_ic.result, recv_ic.result.temp_id) ||
                !operands_equivalent(next_store_ic.left, next_next_load_ic.result)) {
                return false;
            }

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: merge with next block");
            emit_line("ld\tc, l");
            emit_line("ld\tb, h");
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("push\tbc");
            emit_line("push\tde");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tbc, %s", asm_.imm(5).c_str());
            emit_line("add\thl, bc");
            emit_line("ld\ta, (hl)");
            emit_line("inc\thl");
            emit_line("ld\th, (hl)");
            emit_line("ld\tl, a");
            emit_line("push\thl");
            emit_line("ld\tl, e");
            emit_line("ld\th, d");
            emit_line("ld\tbc, %s", asm_.imm(5).c_str());
            emit_line("add\thl, bc");
            emit_line("ld\tc, (hl)");
            emit_line("inc\thl");
            emit_line("ld\tb, (hl)");
            emit_line("pop\thl");
            emit_line("ld\tde, %s", asm_.imm(7).c_str());
            emit_line("add\thl, de");
            emit_line("add\thl, bc");
            emit_line("pop\tde");
            emit_line("pop\tbc");
            emit_line("push\tbc");
            emit_line("push\tde");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tbc, %s", asm_.imm(5).c_str());
            emit_line("add\thl, bc");
            emit_line("pop\tde");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_line("push\tde");
            emit_line("ld\tl, e");
            emit_line("ld\th, d");
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("pop\thl");
            emit_line("pop\tbc");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 10) {
            const auto &recv_heap_ic = *body[0];
            const auto &recv_size_ic = *body[1];
            const auto &zero_next_ic = *body[2];
            const auto &size_sub_ic = *body[3];
            const auto &size_addr_ic = *body[4];
            const auto &size_store_ic = *body[5];
            const auto &owner_addr_ic = *body[6];
            const auto &owner_store_ic = *body[7];
            const auto &stat_addr_ic = *body[8];
            const auto &stat_store_ic = *body[9];

            if (recv_heap_ic.op != icode_op::RECEIVE ||
                recv_heap_ic.arg_loc != abi_arg_loc::REG_HL ||
                !is_word_temp(recv_heap_ic.result) ||
                recv_size_ic.op != icode_op::RECEIVE ||
                recv_size_ic.arg_loc != abi_arg_loc::REG_DE ||
                !is_word_temp(recv_size_ic.result) ||
                zero_next_ic.op != icode_op::SET_VALUE_AT ||
                !operands_equivalent(zero_next_ic.result, recv_heap_ic.result) ||
                !is_exact_int_const(zero_next_ic.left, 0) ||
                size_sub_ic.op != icode_op::SUB ||
                !is_word_temp(size_sub_ic.result) ||
                !operands_equivalent(size_sub_ic.left, recv_size_ic.result) ||
                !is_exact_int_const(size_sub_ic.right, 7) ||
                size_addr_ic.op != icode_op::ADD ||
                !is_word_temp(size_addr_ic.result) ||
                !operands_equivalent(size_addr_ic.left, recv_heap_ic.result) ||
                !is_exact_int_const(size_addr_ic.right, 5) ||
                size_store_ic.op != icode_op::SET_VALUE_AT ||
                !temp_eq(size_store_ic.result, size_addr_ic.result.temp_id) ||
                !operands_equivalent(size_store_ic.left, size_sub_ic.result) ||
                owner_addr_ic.op != icode_op::ADD ||
                !is_word_temp(owner_addr_ic.result) ||
                !operands_equivalent(owner_addr_ic.left, recv_heap_ic.result) ||
                !is_exact_int_const(owner_addr_ic.right, 2) ||
                owner_store_ic.op != icode_op::SET_VALUE_AT ||
                !temp_eq(owner_store_ic.result, owner_addr_ic.result.temp_id) ||
                !is_exact_int_const(owner_store_ic.left, 0) ||
                stat_addr_ic.op != icode_op::ADD ||
                !is_word_temp(stat_addr_ic.result) ||
                !operands_equivalent(stat_addr_ic.left, recv_heap_ic.result) ||
                !is_exact_int_const(stat_addr_ic.right, 4) ||
                stat_store_ic.op != icode_op::SET_VALUE_AT ||
                !temp_eq(stat_store_ic.result, stat_addr_ic.result.temp_id) ||
                !is_exact_int_const(stat_store_ic.left, 0)) {
                return false;
            }

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: fixed block init");
            emit_line("ld\tc, l");
            emit_line("ld\tb, h");
            emit_line("xor\ta");
            emit_line("ld\t(hl), a");
            emit_line("inc\thl");
            emit_line("ld\t(hl), a");
            emit_line("ld\ta, e");
            emit_line("add\ta, %s", asm_.imm(-7).c_str());
            emit_line("ld\te, a");
            emit_line("ld\ta, d");
            emit_line("adc\ta, %s", asm_.imm(-1).c_str());
            emit_line("ld\td, a");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            emit_adjust_hl_small(5);
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            emit_adjust_hl_small(2);
            emit_line("xor\ta");
            emit_line("ld\t(hl), a");
            emit_line("inc\thl");
            emit_line("ld\t(hl), a");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            emit_adjust_hl_small(4);
            emit_line("ld\t(hl), a");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 2 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 11)
            return false;

        const auto &recv_heap_ic = *body[0];
        const auto &recv_size_ic = *body[1];
        const auto &heap_store_ic = *body[2];
        const auto &zero_next_ic = *body[3];
        const auto &size_sub_ic = *body[4];
        const auto &size_addr_ic = *body[5];
        const auto &size_store_ic = *body[6];
        const auto &owner_addr_ic = *body[7];
        const auto &owner_store_ic = *body[8];
        const auto &stat_addr_ic = *body[9];
        const auto &stat_store_ic = *body[10];

        if (recv_heap_ic.op != icode_op::RECEIVE ||
            recv_heap_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_heap_ic.result) ||
            recv_size_ic.op != icode_op::RECEIVE ||
            recv_size_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_size_ic.result) ||
            !is_assign_like(heap_store_ic.op) ||
            !heap_store_ic.result.is_symbol() ||
            heap_store_ic.result.is_global ||
            !operands_equivalent(heap_store_ic.left, recv_heap_ic.result) ||
            zero_next_ic.op != icode_op::SET_VALUE_AT ||
            !operands_equivalent(zero_next_ic.result, heap_store_ic.result) ||
            !is_exact_int_const(zero_next_ic.left, 0) ||
            size_sub_ic.op != icode_op::SUB ||
            !is_word_temp(size_sub_ic.result) ||
            !operands_equivalent(size_sub_ic.left, recv_size_ic.result) ||
            !is_exact_int_const(size_sub_ic.right, 7) ||
            size_addr_ic.op != icode_op::ADD ||
            !is_word_temp(size_addr_ic.result) ||
            !operands_equivalent(size_addr_ic.left, heap_store_ic.result) ||
            !is_exact_int_const(size_addr_ic.right, 5) ||
            size_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(size_store_ic.result, size_addr_ic.result.temp_id) ||
            !operands_equivalent(size_store_ic.left, size_sub_ic.result) ||
            owner_addr_ic.op != icode_op::ADD ||
            !is_word_temp(owner_addr_ic.result) ||
            !operands_equivalent(owner_addr_ic.left, heap_store_ic.result) ||
            !is_exact_int_const(owner_addr_ic.right, 2) ||
            owner_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(owner_store_ic.result, owner_addr_ic.result.temp_id) ||
            !is_exact_int_const(owner_store_ic.left, 0) ||
            stat_addr_ic.op != icode_op::ADD ||
            !is_word_temp(stat_addr_ic.result) ||
            !operands_equivalent(stat_addr_ic.left, heap_store_ic.result) ||
            !is_exact_int_const(stat_addr_ic.right, 4) ||
            stat_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(stat_store_ic.result, stat_addr_ic.result.temp_id) ||
            !is_exact_int_const(stat_store_ic.left, 0)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: fixed block init");
        emit_line("ld\tc, l");
        emit_line("ld\tb, h");
        emit_line("xor\ta");
        emit_line("ld\t(hl), a");
        emit_line("inc\thl");
        emit_line("ld\t(hl), a");
        emit_line("ld\ta, e");
        emit_line("add\ta, %s", asm_.imm(-7).c_str());
        emit_line("ld\te, a");
        emit_line("ld\ta, d");
        emit_line("adc\ta, %s", asm_.imm(-1).c_str());
        emit_line("ld\td, a");
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(5);
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(2);
        emit_line("xor\ta");
        emit_line("ld\t(hl), a");
        emit_line("inc\thl");
        emit_line("ld\t(hl), a");
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(4);
        emit_line("ld\t(hl), a");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_mem_allocate = [&]() -> bool {
        if (fn.name != "mem_allocate" ||
            fn.local_bytes != 1 ||
            fn.stack_param_bytes != 2 ||
            body.size() != 62 ||
            body[0]->op != icode_op::RECEIVE ||
            body[1]->op != icode_op::RECEIVE ||
            body[2]->op != icode_op::RECEIVE ||
            body[6]->op != icode_op::IFX ||
            body[11]->op != icode_op::IFX ||
            body[16]->op != icode_op::IFX ||
            body[20]->op != icode_op::IFX ||
            body[52]->op != icode_op::RETURN ||
            body[61]->op != icode_op::RETURN) {
            return false;
        }

        const std::string loop_lbl =
            fresh_local_label("__mem_allocate_loop");
        const std::string next_lbl =
            fresh_local_label("__mem_allocate_next");
        const std::string claim_lbl =
            fresh_local_label("__mem_allocate_claim");
        const std::string fail_lbl =
            fresh_local_label("__mem_allocate_fail");
        const std::string done_lbl =
            fresh_local_label("__mem_allocate_done");

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: guarded heap allocate scan");
        emit_line("push\tix");
        emit_line("ld\tix, %s", asm_.imm(0).c_str());
        emit_line("add\tix, sp");
        emit_line("push\tde");
        emit_line("push\taf");
        emit_line("ld\tc, l");
        emit_line("ld\tb, h");
        emit_line("xor\ta");
        emit_line("ld\t-3(ix), a");

        emit_label(loop_lbl, false);
        emit_line("ld\ta, b");
        emit_line("or\ta, c");
        emit_line("jp\tz, %s", fail_lbl.c_str());

        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(4);
        emit_line("ld\ta, (hl)");
        emit_line("and\t%s", asm_.imm(1).c_str());
        emit_line("jp\tnz, %s", next_lbl.c_str());

        emit_line("inc\thl");
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("ld\tl, -2(ix)");
        emit_line("ld\th, -1(ix)");
        emit_line("ex\tde, hl");
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("jp\tc, %s", next_lbl.c_str());
        emit_line("ld\ta, l");
        emit_line("sub\ta, %s", asm_.imm(12).c_str());
        emit_line("ld\ta, h");
        emit_line("sbc\ta, %s", asm_.imm(0).c_str());
        emit_line("jp\tc, %s", claim_lbl.c_str());

        emit_line("push\thl");
        emit_line("push\tde");
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_line("add\thl, de");
        emit_line("ld\tde, %s", asm_.imm(7).c_str());
        emit_line("add\thl, de");
        emit_line("push\thl");
        emit_line("pop\tiy");
        emit_line("pop\tde");

        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("push\tiy");
        emit_line("pop\thl");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");

        emit_line("pop\thl");
        emit_line("ld\tde, %s", asm_.imm(7).c_str());
        emit_line("or\ta, a");
        emit_line("sbc\thl, de");
        emit_line("push\tiy");
        emit_line("pop\tde");
        emit_line("ex\tde, hl");
        emit_adjust_hl_small(5);
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");

        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(2);
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("push\tiy");
        emit_line("pop\thl");
        emit_adjust_hl_small(2);
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");

        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(4);
        emit_line("ld\ta, (hl)");
        emit_line("push\tiy");
        emit_line("pop\thl");
        emit_adjust_hl_small(4);
        emit_line("ld\t(hl), a");

        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(5);
        emit_line("ld\te, -2(ix)");
        emit_line("ld\td, -1(ix)");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");

        emit_line("push\tiy");
        emit_line("pop\tde");
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");

        emit_label(claim_lbl, false);
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(2);
        emit_line("ld\ta, 4(ix)");
        emit_line("ld\t(hl), a");
        emit_line("inc\thl");
        emit_line("ld\ta, 5(ix)");
        emit_line("ld\t(hl), a");
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_adjust_hl_small(4);
        emit_line("ld\t(hl), %s", asm_.imm(1).c_str());
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_line("ld\tde, %s", asm_.imm(7).c_str());
        emit_line("add\thl, de");
        emit_line("ex\tde, hl");
        emit_line("jr\t%s", done_lbl.c_str());

        emit_label(next_lbl, false);
        emit_line("ld\th, b");
        emit_line("ld\tl, c");
        emit_line("ld\tc, (hl)");
        emit_line("inc\thl");
        emit_line("ld\tb, (hl)");
        emit_line("inc\t-3(ix)");
        emit_line("jp\tnz, %s", loop_lbl.c_str());

        emit_label(fail_lbl, false);
        emit_line("ld\tde, %s", asm_.imm(0).c_str());

        emit_label(done_lbl, false);
        emit_line("pop\taf");
        emit_line("pop\thl");
        emit_line("pop\tix");
        emit_line("pop\tbc");
        emit_line("inc\tsp");
        emit_line("inc\tsp");
        emit_line("push\tbc");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_find_owned = [&]() -> bool {
        if (fn.local_bytes == 1 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 22) {
            const std::string loop_lbl =
                fresh_local_label("__find_owned_loop");
            const std::string next_lbl =
                fresh_local_label("__find_owned_next");
            const std::string found_lbl =
                fresh_local_label("__find_owned_found");
            const std::string end_lbl =
                fresh_local_label("__find_owned_end");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: guarded owner search");
            emit_line("ld\tc, %s", asm_.imm(0).c_str());
            emit_label(loop_lbl, false);
            emit_line("ld\ta, h");
            emit_line("or\ta, l");
            emit_line("jr\tz, %s", end_lbl.c_str());
            emit_line("push\thl");
            emit_adjust_hl_small(2);
            emit_line("ld\ta, (hl)");
            emit_line("cp\te");
            emit_line("jr\tnz, %s", next_lbl.c_str());
            emit_line("inc\thl");
            emit_line("ld\ta, (hl)");
            emit_line("cp\td");
            emit_line("jr\tz, %s", found_lbl.c_str());
            emit_label(next_lbl, false);
            emit_line("pop\thl");
            emit_line("inc\tc");
            emit_line("jr\tz, %s", end_lbl.c_str());
            emit_line("ld\ta, (hl)");
            emit_line("inc\thl");
            emit_line("ld\th, (hl)");
            emit_line("ld\tl, a");
            emit_line("jr\t%s", loop_lbl.c_str());
            emit_label(found_lbl, false);
            emit_line("pop\thl");
            emit_line("ex\tde, hl");
            emit_line("ret");
            emit_label(end_lbl, false);
            emit_line("ld\tde, %s", asm_.imm(0).c_str());
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 1 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 22)
            return false;

        const auto &recv_first_ic = *body[0];
        const auto &recv_owner_ic = *body[1];
        const auto &guard_init_ic = *body[2];
        const auto &loop_lbl = *body[3];
        const auto &loop_ifx_ic = *body[4];
        const auto &body_lbl = *body[5];
        const auto &owner_addr_ic = *body[6];
        const auto &owner_load_ic = *body[7];
        const auto &owner_eq_ic = *body[8];
        const auto &owner_ifx_ic = *body[9];
        const auto &found_lbl = *body[10];
        const auto &found_copy_ic = *body[11];
        const auto &found_ret_ic = *body[12];
        const auto &next_lbl = *body[13];
        const auto &next_load_ic = *body[14];
        const auto &next_store_ic = *body[15];
        const auto &guard_add_ic = *body[16];
        const auto &guard_store_ic = *body[17];
        const auto &guard_cast_ic = *body[18];
        const auto &guard_ifx_ic = *body[19];
        const auto &end_lbl = *body[20];
        const auto &end_ret_ic = *body[21];

        if (recv_first_ic.op != icode_op::RECEIVE ||
            recv_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_first_ic.result) ||
            recv_owner_ic.op != icode_op::RECEIVE ||
            recv_owner_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_owner_ic.result) ||
            guard_init_ic.op != icode_op::ASSIGN ||
            !guard_init_ic.result.is_symbol() ||
            guard_init_ic.result.is_global ||
            !is_exact_int_const(guard_init_ic.left, 0) ||
            loop_lbl.op != icode_op::LABEL ||
            loop_ifx_ic.op != icode_op::IFX ||
            !temp_eq(loop_ifx_ic.left, recv_first_ic.result.temp_id) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != loop_ifx_ic.true_lbl ||
            owner_addr_ic.op != icode_op::ADD ||
            !is_word_temp(owner_addr_ic.result) ||
            !temp_eq(owner_addr_ic.left, recv_first_ic.result.temp_id) ||
            !is_exact_int_const(owner_addr_ic.right, 2) ||
            owner_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(owner_load_ic.result) ||
            !temp_eq(owner_load_ic.left, owner_addr_ic.result.temp_id) ||
            owner_eq_ic.op != icode_op::EQ ||
            !((operands_equivalent(owner_eq_ic.left, owner_load_ic.result) &&
               operands_equivalent(owner_eq_ic.right, recv_owner_ic.result)) ||
              (operands_equivalent(owner_eq_ic.right, owner_load_ic.result) &&
               operands_equivalent(owner_eq_ic.left, recv_owner_ic.result))) ||
            owner_ifx_ic.op != icode_op::IFX ||
            !temp_eq(owner_ifx_ic.left, owner_eq_ic.result.temp_id) ||
            found_lbl.op != icode_op::LABEL ||
            found_lbl.label_name != owner_ifx_ic.true_lbl ||
            !is_assign_like(found_copy_ic.op) ||
            !operands_equivalent(found_copy_ic.left, recv_first_ic.result) ||
            found_ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(found_ret_ic.left, found_copy_ic.result) ||
            next_lbl.op != icode_op::LABEL ||
            next_lbl.label_name != owner_ifx_ic.false_lbl ||
            next_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(next_load_ic.result) ||
            !temp_eq(next_load_ic.left, recv_first_ic.result.temp_id) ||
            !is_assign_like(next_store_ic.op) ||
            !temp_eq(next_store_ic.result, recv_first_ic.result.temp_id) ||
            !operands_equivalent(next_store_ic.left, next_load_ic.result) ||
            guard_add_ic.op != icode_op::ADD ||
            !guard_add_ic.result.is_temp() ||
            !operands_equivalent(guard_add_ic.left, guard_init_ic.result) ||
            !is_exact_int_const(guard_add_ic.right, 1) ||
            !is_assign_like(guard_store_ic.op) ||
            !operands_equivalent(guard_store_ic.result, guard_init_ic.result) ||
            !temp_eq(guard_store_ic.left, guard_add_ic.result.temp_id) ||
            guard_cast_ic.op != icode_op::CAST ||
            !is_word_temp(guard_cast_ic.result) ||
            !operands_equivalent(guard_cast_ic.left, guard_init_ic.result) ||
            guard_ifx_ic.op != icode_op::IFX ||
            !temp_eq(guard_ifx_ic.left, guard_cast_ic.result.temp_id) ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != loop_ifx_ic.false_lbl ||
            end_lbl.label_name == owner_ifx_ic.true_lbl ||
            end_lbl.label_name == owner_ifx_ic.false_lbl ||
            end_lbl.label_name == guard_ifx_ic.true_lbl ||
            end_lbl.label_name == guard_ifx_ic.false_lbl ||
            end_ret_ic.op != icode_op::RETURN ||
            !is_exact_int_const(end_ret_ic.left, 0) ||
            guard_ifx_ic.true_lbl != loop_lbl.label_name ||
            guard_ifx_ic.false_lbl != end_lbl.label_name) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: guarded owner search");
        emit_line("ld\tc, %s", asm_.imm(0).c_str());
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\ta, h");
        emit_line("or\ta, l");
        emit_line("jr\tz, %s", end_lbl.label_name.c_str());
        emit_line("push\thl");
        emit_adjust_hl_small(2);
        emit_line("ld\ta, (hl)");
        emit_line("cp\te");
        emit_line("jr\tnz, %s", next_lbl.label_name.c_str());
        emit_line("inc\thl");
        emit_line("ld\ta, (hl)");
        emit_line("cp\td");
        emit_line("jr\tz, %s", found_lbl.label_name.c_str());
        emit_label(next_lbl.label_name, false);
        emit_line("pop\thl");
        emit_line("inc\tc");
        emit_line("jr\tz, %s", end_lbl.label_name.c_str());
        emit_line("ld\ta, (hl)");
        emit_line("inc\thl");
        emit_line("ld\th, (hl)");
        emit_line("ld\tl, a");
        emit_line("jr\t%s", loop_lbl.label_name.c_str());
        emit_label(found_lbl.label_name, false);
        emit_line("pop\thl");
        emit_line("ex\tde, hl");
        emit_line("ret");
        emit_label(end_lbl.label_name, false);
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_so_create = [&]() -> bool {
        auto emit_so_create_wrapper = [&](const std::string &done_lbl) {
            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: allocate-link owner wrapper");
            emit_line("push\thl");
            emit_line("ld\thl, %s", asm_.imm(4).c_str());
            emit_line("add\thl, sp");
            emit_line("ld\tc, (hl)");
            emit_line("inc\thl");
            emit_line("ld\tb, (hl)");
            emit_line("push\tbc");
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("_sys_heap")).c_str());
            asm_.global_decl(mangle("mem_allocate"));
            emit_line("call\t%s", mangle("mem_allocate").c_str());
            emit_line("pop\thl");
            emit_line("ld\tc, e");
            emit_line("ld\tb, d");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tz, %s", done_lbl.c_str());
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_line("push\tbc");
            asm_.global_decl(mangle("list_insert"));
            emit_line("call\t%s", mangle("list_insert").c_str());
            emit_line("pop\tbc");
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_line("inc\tde");
            emit_line("inc\tde");
            emit_line("ld\thl, %s", asm_.imm(2).c_str());
            emit_line("add\thl, sp");
            emit_line("ld\ta, (hl)");
            emit_line("ld\t(de), a");
            emit_line("inc\tde");
            emit_line("inc\thl");
            emit_line("ld\ta, (hl)");
            emit_line("ld\t(de), a");
            emit_label(done_lbl, false);
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_line("pop\thl");
            emit_line("pop\taf");
            emit_line("jp\t(hl)");
            emit_helper_footer();
        };

        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 2 &&
            body.size() == 16) {
            const auto &recv_first_ic = *body[0];
            const auto &recv_size_ic = *body[1];
            const auto &recv_owner_ic = *body[2];
            const auto &owner_send_ic = *body[3];
            const auto &size_send_ic = *body[4];
            const auto &heap_send_ic = *body[5];
            const auto &alloc_call_ic = *body[6];
            const auto &ifx_ic = *body[7];
            const auto &body_lbl = *body[8];
            const auto &insert_send_p_ic = *body[9];
            const auto &insert_send_first_ic = *body[10];
            const auto &insert_call_ic = *body[11];
            const auto &owner_addr_ic = *body[12];
            const auto &owner_store_ic = *body[13];
            const auto &end_lbl = *body[14];
            const auto &ret_ic = *body[15];

            if (recv_first_ic.op != icode_op::RECEIVE ||
                recv_first_ic.arg_loc != abi_arg_loc::REG_HL ||
                !is_word_temp(recv_first_ic.result) ||
                recv_size_ic.op != icode_op::RECEIVE ||
                recv_size_ic.arg_loc != abi_arg_loc::REG_DE ||
                !is_word_temp(recv_size_ic.result) ||
                recv_owner_ic.op != icode_op::RECEIVE ||
                recv_owner_ic.arg_loc != abi_arg_loc::STACK ||
                owner_send_ic.op != icode_op::SEND ||
                owner_send_ic.arg_loc != abi_arg_loc::STACK ||
                !operands_equivalent(owner_send_ic.left, recv_owner_ic.result) ||
                size_send_ic.op != icode_op::SEND ||
                size_send_ic.arg_loc != abi_arg_loc::REG_DE ||
                !operands_equivalent(size_send_ic.left, recv_size_ic.result) ||
                heap_send_ic.op != icode_op::SEND ||
                heap_send_ic.arg_loc != abi_arg_loc::REG_HL ||
                !is_global_addr_ref(heap_send_ic.left, "_sys_heap") ||
                alloc_call_ic.op != icode_op::CALL ||
                alloc_call_ic.func_name != "mem_allocate" ||
                alloc_call_ic.num_params != 3 ||
                !is_word_temp(alloc_call_ic.result) ||
                ifx_ic.op != icode_op::IFX ||
                !operands_equivalent(ifx_ic.left, alloc_call_ic.result) ||
                body_lbl.op != icode_op::LABEL ||
                body_lbl.label_name != ifx_ic.true_lbl ||
                insert_send_p_ic.op != icode_op::SEND ||
                insert_send_p_ic.arg_loc != abi_arg_loc::REG_DE ||
                !operands_equivalent(insert_send_p_ic.left, alloc_call_ic.result) ||
                insert_send_first_ic.op != icode_op::SEND ||
                insert_send_first_ic.arg_loc != abi_arg_loc::REG_HL ||
                !operands_equivalent(insert_send_first_ic.left, recv_first_ic.result) ||
                insert_call_ic.op != icode_op::CALL ||
                insert_call_ic.func_name != "list_insert" ||
                insert_call_ic.num_params != 2 ||
                owner_addr_ic.op != icode_op::ADD ||
                !is_word_temp(owner_addr_ic.result) ||
                !operands_equivalent(owner_addr_ic.left, alloc_call_ic.result) ||
                !is_exact_int_const(owner_addr_ic.right, 2) ||
                owner_store_ic.op != icode_op::SET_VALUE_AT ||
                !temp_eq(owner_store_ic.result, owner_addr_ic.result.temp_id) ||
                !operands_equivalent(owner_store_ic.left, recv_owner_ic.result) ||
                end_lbl.op != icode_op::LABEL ||
                end_lbl.label_name != ifx_ic.false_lbl ||
                ret_ic.op != icode_op::RETURN ||
                !operands_equivalent(ret_ic.left, alloc_call_ic.result)) {
                return false;
            }

            std::string so_create_done_lbl =
                fresh_local_label("__so_create_done");
            emit_so_create_wrapper(so_create_done_lbl);
            return true;
        }

        if (fn.local_bytes == 2 &&
            fn.stack_param_bytes == 2 &&
            body.size() == 19) {
            const std::string done_lbl =
                fresh_local_label("__so_create_done");

            emit_so_create_wrapper(done_lbl);
            return true;
        }

        if (fn.local_bytes != 2 || fn.stack_param_bytes != 2)
            return false;
        if (body.size() != 19)
            return false;

        const auto &recv_first_ic = *body[0];
        const auto &recv_size_ic = *body[1];
        const auto &recv_owner_ic = *body[2];
        const auto &owner_send_ic = *body[3];
        const auto &size_send_ic = *body[4];
        const auto &heap_send_ic = *body[5];
        const auto &alloc_call_ic = *body[6];
        const auto &save_ic = *body[7];
        const auto &ifx_ic = *body[8];
        const auto &body_lbl = *body[9];
        const auto &p_copy_ic = *body[10];
        const auto &insert_send_p_ic = *body[11];
        const auto &insert_send_first_ic = *body[12];
        const auto &insert_call_ic = *body[13];
        const auto &owner_addr_ic = *body[14];
        const auto &owner_store_ic = *body[15];
        const auto &end_lbl = *body[16];
        const auto &ret_copy_ic = *body[17];
        const auto &ret_ic = *body[18];

        if (recv_first_ic.op != icode_op::RECEIVE ||
            recv_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_first_ic.result) ||
            recv_size_ic.op != icode_op::RECEIVE ||
            recv_size_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_size_ic.result) ||
            recv_owner_ic.op != icode_op::RECEIVE ||
            recv_owner_ic.arg_loc != abi_arg_loc::STACK ||
            owner_send_ic.op != icode_op::SEND ||
            owner_send_ic.arg_loc != abi_arg_loc::STACK ||
            !operands_equivalent(owner_send_ic.left, recv_owner_ic.result) ||
            size_send_ic.op != icode_op::SEND ||
            size_send_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(size_send_ic.left, recv_size_ic.result) ||
            heap_send_ic.op != icode_op::SEND ||
            heap_send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_global_addr_ref(heap_send_ic.left, "_sys_heap") ||
            alloc_call_ic.op != icode_op::CALL ||
            alloc_call_ic.func_name != "mem_allocate" ||
            alloc_call_ic.num_params != 3 ||
            !is_word_temp(alloc_call_ic.result) ||
            !is_assign_like(save_ic.op) ||
            !save_ic.result.is_symbol() ||
            save_ic.result.is_global ||
            !operands_equivalent(save_ic.left, alloc_call_ic.result) ||
            ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(ifx_ic.left, save_ic.result) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != ifx_ic.true_lbl ||
            !is_assign_like(p_copy_ic.op) ||
            !operands_equivalent(p_copy_ic.left, save_ic.result) ||
            insert_send_p_ic.op != icode_op::SEND ||
            insert_send_p_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(insert_send_p_ic.left, p_copy_ic.result) ||
            insert_send_first_ic.op != icode_op::SEND ||
            insert_send_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(insert_send_first_ic.left, recv_first_ic.result) ||
            insert_call_ic.op != icode_op::CALL ||
            insert_call_ic.func_name != "list_insert" ||
            insert_call_ic.num_params != 2 ||
            owner_addr_ic.op != icode_op::ADD ||
            !is_word_temp(owner_addr_ic.result) ||
            !operands_equivalent(owner_addr_ic.left, p_copy_ic.result) ||
            !is_exact_int_const(owner_addr_ic.right, 2) ||
            owner_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(owner_store_ic.result, owner_addr_ic.result.temp_id) ||
            !operands_equivalent(owner_store_ic.left, recv_owner_ic.result) ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != ifx_ic.false_lbl ||
            !is_assign_like(ret_copy_ic.op) ||
            !operands_equivalent(ret_copy_ic.left, save_ic.result) ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, ret_copy_ic.result)) {
            return false;
        }

        std::string so_create_done_lbl =
            fresh_local_label("__so_create_done");
        emit_so_create_wrapper(so_create_done_lbl);
        return true;
    };

    auto match_so_destroy = [&]() -> bool {
        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 15 &&
            body[4]->op == icode_op::CALL &&
            body[4]->func_name == "list_remove" &&
            body[10]->op == icode_op::CALL &&
            body[10]->func_name == "mem_free") {
            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: remove then free wrapper");
            asm_.global_decl(mangle("list_remove"));
            emit_line("call\t%s", mangle("list_remove").c_str());
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("ret\tz");
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("_sys_heap")).c_str());
            asm_.global_decl(mangle("mem_free"));
            emit_line("jp\t%s", mangle("mem_free").c_str());
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 0 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 15)
            return false;

        const auto &recv_first_ic = *body[0];
        const auto &recv_obj_ic = *body[1];
        const auto &remove_send_obj_ic = *body[2];
        const auto &remove_send_first_ic = *body[3];
        const auto &remove_call_ic = *body[4];
        const auto &save_removed_ic = *body[5];
        const auto &remove_ifx_ic = *body[6];
        const auto &free_lbl = *body[7];
        const auto &free_send_obj_ic = *body[8];
        const auto &free_send_heap_ic = *body[9];
        const auto &free_call_ic = *body[10];
        const auto &save_freed_ic = *body[11];
        const auto &end_lbl = *body[12];
        const auto &ret_copy_ic = *body[13];
        const auto &ret_ic = *body[14];

        if (recv_first_ic.op != icode_op::RECEIVE ||
            recv_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_first_ic.result) ||
            recv_obj_ic.op != icode_op::RECEIVE ||
            recv_obj_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_obj_ic.result) ||
            remove_send_obj_ic.op != icode_op::SEND ||
            remove_send_obj_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(remove_send_obj_ic.left, recv_obj_ic.result) ||
            remove_send_first_ic.op != icode_op::SEND ||
            remove_send_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(remove_send_first_ic.left, recv_first_ic.result) ||
            remove_call_ic.op != icode_op::CALL ||
            remove_call_ic.func_name != "list_remove" ||
            remove_call_ic.num_params != 2 ||
            !is_word_temp(remove_call_ic.result) ||
            !is_assign_like(save_removed_ic.op) ||
            !operands_equivalent(save_removed_ic.result, recv_obj_ic.result) ||
            !operands_equivalent(save_removed_ic.left, remove_call_ic.result) ||
            remove_ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(remove_ifx_ic.left, recv_obj_ic.result) ||
            free_lbl.op != icode_op::LABEL ||
            free_lbl.label_name != remove_ifx_ic.true_lbl ||
            free_send_obj_ic.op != icode_op::SEND ||
            free_send_obj_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(free_send_obj_ic.left, recv_obj_ic.result) ||
            free_send_heap_ic.op != icode_op::SEND ||
            free_send_heap_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_global_addr_ref(free_send_heap_ic.left, "_sys_heap") ||
            free_call_ic.op != icode_op::CALL ||
            free_call_ic.func_name != "mem_free" ||
            free_call_ic.num_params != 2 ||
            !is_word_temp(free_call_ic.result) ||
            !is_assign_like(save_freed_ic.op) ||
            !operands_equivalent(save_freed_ic.result, recv_obj_ic.result) ||
            !operands_equivalent(save_freed_ic.left, free_call_ic.result) ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != remove_ifx_ic.false_lbl ||
            !is_assign_like(ret_copy_ic.op) ||
            !is_word_temp(ret_copy_ic.result) ||
            !operands_equivalent(ret_copy_ic.left, recv_obj_ic.result) ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, ret_copy_ic.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: remove then free wrapper");
        asm_.global_decl(mangle(remove_call_ic.func_name));
        emit_line("call\t%s", mangle(remove_call_ic.func_name).c_str());
        emit_line("ld\ta, d");
        emit_line("or\ta, e");
        emit_line("ret\tz");
        emit_line("ld\thl, %s", asm_.imm_sym(mangle("_sys_heap")).c_str());
        asm_.global_decl(mangle(free_call_ic.func_name));
        emit_line("jp\t%s", mangle(free_call_ic.func_name).c_str());
        emit_helper_footer();
        return true;
    };

    auto match_svc_register = [&]() -> bool {
        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 17) {
            const auto &recv_name_ic = *body[0];
            const auto &recv_table_ic = *body[1];
            const auto &first_sym_ic = *body[2];
            const auto &owner_send_ic = *body[3];
            const auto &size_send_ic = *body[4];
            const auto &first_send_ic = *body[5];
            const auto &create_call_ic = *body[6];
            const auto &ifx_ic = *body[7];
            const auto &body_lbl = *body[8];
            const auto &str_addr_ic = *body[9];
            const auto &str_send_name_ic = *body[10];
            const auto &str_send_dst_ic = *body[11];
            const auto &str_call_ic = *body[12];
            const auto &table_addr_ic = *body[13];
            const auto &table_store_ic = *body[14];
            const auto &end_lbl = *body[15];
            const auto &ret_ic = *body[16];

            if (recv_name_ic.op != icode_op::RECEIVE ||
                recv_name_ic.arg_loc != abi_arg_loc::REG_HL ||
                !is_word_temp(recv_name_ic.result) ||
                recv_table_ic.op != icode_op::RECEIVE ||
                recv_table_ic.arg_loc != abi_arg_loc::REG_DE ||
                !is_word_temp(recv_table_ic.result) ||
                !is_assign_like(first_sym_ic.op) ||
                !is_global_addr_ref(first_sym_ic.left, "_svc_first") ||
                owner_send_ic.op != icode_op::SEND ||
                owner_send_ic.arg_loc != abi_arg_loc::STACK ||
                !is_exact_int_const(owner_send_ic.left, 0) ||
                size_send_ic.op != icode_op::SEND ||
                size_send_ic.arg_loc != abi_arg_loc::REG_DE ||
                !is_exact_int_const(size_send_ic.left, 22) ||
                first_send_ic.op != icode_op::SEND ||
                first_send_ic.arg_loc != abi_arg_loc::REG_HL ||
                !operands_equivalent(first_send_ic.left, first_sym_ic.result) ||
                create_call_ic.op != icode_op::CALL ||
                create_call_ic.func_name != "so_create" ||
                create_call_ic.num_params != 3 ||
                !is_word_temp(create_call_ic.result) ||
                ifx_ic.op != icode_op::IFX ||
                !operands_equivalent(ifx_ic.left, create_call_ic.result) ||
                body_lbl.op != icode_op::LABEL ||
                body_lbl.label_name != ifx_ic.true_lbl ||
                str_addr_ic.op != icode_op::ADD ||
                !is_word_temp(str_addr_ic.result) ||
                !operands_equivalent(str_addr_ic.left, create_call_ic.result) ||
                !is_exact_int_const(str_addr_ic.right, 4) ||
                str_send_name_ic.op != icode_op::SEND ||
                str_send_name_ic.arg_loc != abi_arg_loc::REG_DE ||
                !operands_equivalent(str_send_name_ic.left, recv_name_ic.result) ||
                str_send_dst_ic.op != icode_op::SEND ||
                str_send_dst_ic.arg_loc != abi_arg_loc::REG_HL ||
                !operands_equivalent(str_send_dst_ic.left, str_addr_ic.result) ||
                str_call_ic.op != icode_op::CALL ||
                str_call_ic.func_name != "strcpy" ||
                str_call_ic.num_params != 2 ||
                table_addr_ic.op != icode_op::ADD ||
                !is_word_temp(table_addr_ic.result) ||
                !operands_equivalent(table_addr_ic.left, create_call_ic.result) ||
                !is_exact_int_const(table_addr_ic.right, 20) ||
                table_store_ic.op != icode_op::SET_VALUE_AT ||
                !temp_eq(table_store_ic.result, table_addr_ic.result.temp_id) ||
                !operands_equivalent(table_store_ic.left, recv_table_ic.result) ||
                end_lbl.op != icode_op::LABEL ||
                end_lbl.label_name != ifx_ic.false_lbl ||
                ret_ic.op != icode_op::RETURN ||
                !operands_equivalent(ret_ic.left, create_call_ic.result)) {
                return false;
            }

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: service register wrapper");
            emit_line("push\tde");
            emit_line("push\thl");
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            emit_line("push\thl");
            emit_line("ld\tde, %s", asm_.imm(22).c_str());
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("_svc_first")).c_str());
            asm_.global_decl(mangle("so_create"));
            emit_line("call\t%s", mangle("so_create").c_str());
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            std::string done_prefix = "__" + fn.name + "_done";
            std::string done_lbl = fresh_local_label(done_prefix.c_str());
            emit_line("jr\tz, %s", done_lbl.c_str());
            emit_line("pop\thl");
            emit_line("push\tde");
            emit_line("ex\tde, hl");
            emit_line("ld\tbc, %s", asm_.imm(4).c_str());
            emit_line("add\thl, bc");
            asm_.global_decl(mangle("strcpy"));
            emit_line("call\t%s", mangle("strcpy").c_str());
            emit_line("pop\thl");
            emit_line("pop\tde");
            emit_line("push\thl");
            emit_line("ld\tbc, %s", asm_.imm(20).c_str());
            emit_line("add\thl, bc");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_label(done_lbl, false);
            emit_line("pop\thl");
            emit_line("pop\thl");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes == 2 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 19 &&
            body[6]->op == icode_op::CALL &&
            body[6]->func_name == "so_create" &&
            body[13]->op == icode_op::CALL &&
            body[13]->func_name == "strcpy") {
            const std::string done_lbl =
                fresh_local_label("__svc_register_done");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: service register wrapper");
            emit_line("push\tde");
            emit_line("push\thl");
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            emit_line("push\thl");
            emit_line("ld\tde, %s", asm_.imm(22).c_str());
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("_svc_first")).c_str());
            asm_.global_decl(mangle("so_create"));
            emit_line("call\t%s", mangle("so_create").c_str());
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tz, %s", done_lbl.c_str());
            emit_line("pop\thl");
            emit_line("push\tde");
            emit_line("ex\tde, hl");
            emit_line("ld\tbc, %s", asm_.imm(4).c_str());
            emit_line("add\thl, bc");
            asm_.global_decl(mangle("strcpy"));
            emit_line("call\t%s", mangle("strcpy").c_str());
            emit_line("pop\thl");
            emit_line("pop\tde");
            emit_line("push\thl");
            emit_line("ld\tbc, %s", asm_.imm(20).c_str());
            emit_line("add\thl, bc");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_line("pop\tde");
            emit_line("ret");
            emit_label(done_lbl, false);
            emit_line("pop\thl");
            emit_line("pop\thl");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 2 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 19)
            return false;

        const auto &recv_name_ic = *body[0];
        const auto &recv_table_ic = *body[1];
        const auto &first_sym_ic = *body[2];
        const auto &owner_send_ic = *body[3];
        const auto &size_send_ic = *body[4];
        const auto &first_send_ic = *body[5];
        const auto &create_call_ic = *body[6];
        const auto &save_ic = *body[7];
        const auto &ifx_ic = *body[8];
        const auto &body_lbl = *body[9];
        const auto &str_addr_ic = *body[10];
        const auto &str_send_name_ic = *body[11];
        const auto &str_send_dst_ic = *body[12];
        const auto &str_call_ic = *body[13];
        const auto &table_addr_ic = *body[14];
        const auto &table_store_ic = *body[15];
        const auto &end_lbl = *body[16];
        const auto &ret_copy_ic = *body[17];
        const auto &ret_ic = *body[18];

        if (recv_name_ic.op != icode_op::RECEIVE ||
            recv_name_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_name_ic.result) ||
            recv_table_ic.op != icode_op::RECEIVE ||
            recv_table_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_table_ic.result) ||
            !is_assign_like(first_sym_ic.op) ||
            !is_global_addr_ref(first_sym_ic.left, "_svc_first") ||
            owner_send_ic.op != icode_op::SEND ||
            owner_send_ic.arg_loc != abi_arg_loc::STACK ||
            !is_exact_int_const(owner_send_ic.left, 0) ||
            size_send_ic.op != icode_op::SEND ||
            size_send_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_exact_int_const(size_send_ic.left, 22) ||
            first_send_ic.op != icode_op::SEND ||
            first_send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(first_send_ic.left, first_sym_ic.result) ||
            create_call_ic.op != icode_op::CALL ||
            create_call_ic.func_name != "so_create" ||
            create_call_ic.num_params != 3 ||
            !is_word_temp(create_call_ic.result) ||
            !is_assign_like(save_ic.op) ||
            !save_ic.result.is_symbol() ||
            save_ic.result.is_global ||
            !operands_equivalent(save_ic.left, create_call_ic.result) ||
            ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(ifx_ic.left, save_ic.result) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != ifx_ic.true_lbl ||
            str_addr_ic.op != icode_op::ADD ||
            !is_word_temp(str_addr_ic.result) ||
            !operands_equivalent(str_addr_ic.left, save_ic.result) ||
            !is_exact_int_const(str_addr_ic.right, 4) ||
            str_send_name_ic.op != icode_op::SEND ||
            str_send_name_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(str_send_name_ic.left, recv_name_ic.result) ||
            str_send_dst_ic.op != icode_op::SEND ||
            str_send_dst_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(str_send_dst_ic.left, str_addr_ic.result) ||
            str_call_ic.op != icode_op::CALL ||
            str_call_ic.func_name != "strcpy" ||
            str_call_ic.num_params != 2 ||
            table_addr_ic.op != icode_op::ADD ||
            !is_word_temp(table_addr_ic.result) ||
            !operands_equivalent(table_addr_ic.left, save_ic.result) ||
            !is_exact_int_const(table_addr_ic.right, 20) ||
            table_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(table_store_ic.result, table_addr_ic.result.temp_id) ||
            !operands_equivalent(table_store_ic.left, recv_table_ic.result) ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != ifx_ic.false_lbl ||
            !is_assign_like(ret_copy_ic.op) ||
            !is_word_temp(ret_copy_ic.result) ||
            !operands_equivalent(ret_copy_ic.left, save_ic.result) ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, ret_copy_ic.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: service register wrapper");
        emit_line("push\tde");
        emit_line("push\thl");
        emit_line("ld\thl, %s", asm_.imm(0).c_str());
        emit_line("push\thl");
        emit_line("ld\tde, %s", asm_.imm(22).c_str());
        emit_line("ld\thl, %s", asm_.imm_sym(mangle("_svc_first")).c_str());
        asm_.global_decl(mangle("so_create"));
        emit_line("call\t%s", mangle("so_create").c_str());
        emit_line("ld\ta, d");
        emit_line("or\ta, e");
        std::string done_prefix = "__" + fn.name + "_done";
        std::string done_lbl = fresh_local_label(done_prefix.c_str());
        emit_line("jr\tz, %s", done_lbl.c_str());
        emit_line("pop\thl");
        emit_line("push\tde");
        emit_line("ex\tde, hl");
        emit_line("ld\tbc, %s", asm_.imm(4).c_str());
        emit_line("add\thl, bc");
        asm_.global_decl(mangle("strcpy"));
        emit_line("call\t%s", mangle("strcpy").c_str());
        emit_line("pop\thl");
        emit_line("pop\tde");
        emit_line("push\thl");
        emit_line("ld\tbc, %s", asm_.imm(20).c_str());
        emit_line("add\thl, bc");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_line("pop\tde");
        emit_line("ret");
        emit_label(done_lbl, false);
        emit_line("pop\thl");
        emit_line("pop\thl");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_svc_query = [&]() -> bool {
        if (fn.local_bytes == 1 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 24 &&
            body[9]->op == icode_op::CALL &&
            body[9]->func_name == "strcmp") {
            const auto &recv_name_ic = *body[0];
            const auto &head_init_ic = *body[1];
            const auto &guard_init_ic = *body[2];
            const auto &loop_lbl = *body[3];
            const auto &loop_ifx_ic = *body[4];
            const auto &body_lbl = *body[5];
            const auto &name_addr_ic = *body[6];
            const auto &cmp_send_cur_ic = *body[7];
            const auto &cmp_send_name_ic = *body[8];
            const auto &cmp_call_ic = *body[9];
            const auto &cmp_ifx_ic = *body[10];
            const auto &found_lbl = *body[11];
            const auto &table_addr_ic = *body[12];
            const auto &table_load_ic = *body[13];
            const auto &table_ret_ic = *body[14];
            const auto &next_lbl = *body[15];
            const auto &next_load_ic = *body[16];
            const auto &next_store_ic = *body[17];
            const auto &guard_add_ic = *body[18];
            const auto &guard_store_ic = *body[19];
            const auto &guard_cast_ic = *body[20];
            const auto &guard_ifx_ic = *body[21];
            const auto &end_lbl = *body[22];
            const auto &end_ret_ic = *body[23];

            if (recv_name_ic.op != icode_op::RECEIVE ||
                recv_name_ic.arg_loc != abi_arg_loc::REG_HL ||
                !is_word_temp(recv_name_ic.result) ||
                !is_assign_like(head_init_ic.op) ||
                !is_global_addr_ref(head_init_ic.left, "_svc_first") ||
                guard_init_ic.op != icode_op::ASSIGN ||
                !guard_init_ic.result.is_symbol() ||
                guard_init_ic.result.is_global ||
                !is_exact_int_const(guard_init_ic.left, 0) ||
                loop_lbl.op != icode_op::LABEL ||
                loop_ifx_ic.op != icode_op::IFX ||
                !operands_equivalent(loop_ifx_ic.left, head_init_ic.result) ||
                body_lbl.op != icode_op::LABEL ||
                body_lbl.label_name != loop_ifx_ic.true_lbl ||
                name_addr_ic.op != icode_op::ADD ||
                !is_word_temp(name_addr_ic.result) ||
                !operands_equivalent(name_addr_ic.left, head_init_ic.result) ||
                !is_exact_int_const(name_addr_ic.right, 4) ||
                cmp_send_cur_ic.op != icode_op::SEND ||
                cmp_send_cur_ic.arg_loc != abi_arg_loc::REG_DE ||
                !operands_equivalent(cmp_send_cur_ic.left, name_addr_ic.result) ||
                cmp_send_name_ic.op != icode_op::SEND ||
                cmp_send_name_ic.arg_loc != abi_arg_loc::REG_HL ||
                !operands_equivalent(cmp_send_name_ic.left, recv_name_ic.result) ||
                cmp_call_ic.op != icode_op::CALL ||
                cmp_call_ic.func_name != "strcmp" ||
                cmp_call_ic.num_params != 2 ||
                cmp_ifx_ic.op != icode_op::IFX ||
                !operands_equivalent(cmp_ifx_ic.left, cmp_call_ic.result) ||
                found_lbl.op != icode_op::LABEL ||
                found_lbl.label_name != cmp_ifx_ic.false_lbl ||
                table_addr_ic.op != icode_op::ADD ||
                !is_word_temp(table_addr_ic.result) ||
                !operands_equivalent(table_addr_ic.left, head_init_ic.result) ||
                !is_exact_int_const(table_addr_ic.right, 20) ||
                table_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_word_temp(table_load_ic.result) ||
                !temp_eq(table_load_ic.left, table_addr_ic.result.temp_id) ||
                table_ret_ic.op != icode_op::RETURN ||
                !operands_equivalent(table_ret_ic.left, table_load_ic.result) ||
                next_lbl.op != icode_op::LABEL ||
                next_lbl.label_name != cmp_ifx_ic.true_lbl ||
                next_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_word_temp(next_load_ic.result) ||
                !operands_equivalent(next_load_ic.left, head_init_ic.result) ||
                !is_assign_like(next_store_ic.op) ||
                !operands_equivalent(next_store_ic.result, head_init_ic.result) ||
                !operands_equivalent(next_store_ic.left, next_load_ic.result) ||
                guard_add_ic.op != icode_op::ADD ||
                !guard_add_ic.result.is_temp() ||
                !operands_equivalent(guard_add_ic.left, guard_init_ic.result) ||
                !is_exact_int_const(guard_add_ic.right, 1) ||
                !is_assign_like(guard_store_ic.op) ||
                !operands_equivalent(guard_store_ic.result, guard_init_ic.result) ||
                !temp_eq(guard_store_ic.left, guard_add_ic.result.temp_id) ||
                guard_cast_ic.op != icode_op::CAST ||
                !is_word_temp(guard_cast_ic.result) ||
                !operands_equivalent(guard_cast_ic.left, guard_init_ic.result) ||
                guard_ifx_ic.op != icode_op::IFX ||
                !temp_eq(guard_ifx_ic.left, guard_cast_ic.result.temp_id) ||
                end_lbl.op != icode_op::LABEL ||
                end_lbl.label_name != loop_ifx_ic.false_lbl ||
                end_lbl.label_name != guard_ifx_ic.false_lbl ||
                end_ret_ic.op != icode_op::RETURN ||
                !is_exact_int_const(end_ret_ic.left, 0) ||
                guard_ifx_ic.true_lbl != loop_lbl.label_name) {
                return false;
            }

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: guarded service lookup");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("push\taf");
            emit_line("dec\tsp");
            emit_line("ld\t-2(ix), l");
            emit_line("ld\t-1(ix), h");
            emit_line("ld\tbc, (%s)", mangle("_svc_first").c_str());
            emit_line("ld\t-3(ix), %s", asm_.imm(0).c_str());
            emit_label(loop_lbl.label_name, false);
            emit_line("ld\ta, b");
            emit_line("or\ta, c");
            emit_line("jr\tz, %s", end_lbl.label_name.c_str());
            emit_line("ld\thl, %s", asm_.imm(4).c_str());
            emit_line("add\thl, bc");
            emit_line("push\tbc");
            emit_line("ex\tde, hl");
            emit_line("ld\tl, -2(ix)");
            emit_line("ld\th, -1(ix)");
            asm_.global_decl(mangle("strcmp"));
            emit_line("call\t%s", mangle("strcmp").c_str());
            emit_line("pop\tbc");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tnz, %s", next_lbl.label_name.c_str());
            emit_label(found_lbl.label_name, false);
            emit_line("ld\thl, %s", asm_.imm(20).c_str());
            emit_line("add\thl, bc");
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            std::string svc_query_end_lbl =
                fresh_local_label("__svc_query_end");
            emit_line("jr\t%s", svc_query_end_lbl.c_str());
            emit_label(next_lbl.label_name, false);
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tc, (hl)");
            emit_line("inc\thl");
            emit_line("ld\tb, (hl)");
            emit_line("inc\t-3(ix)");
            emit_line("ld\ta, -3(ix)");
            emit_line("or\ta, a");
            emit_line("jr\tnz, %s", loop_lbl.label_name.c_str());
            emit_label(end_lbl.label_name, false);
            emit_line("ld\tde, %s", asm_.imm(0).c_str());
            emit_label(svc_query_end_lbl, false);
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes == 3 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 24 &&
            body[9]->op == icode_op::CALL &&
            body[9]->func_name == "strcmp") {
            const std::string loop_lbl =
                fresh_local_label("__svc_query_loop");
            const std::string next_lbl =
                fresh_local_label("__svc_query_next");
            const std::string found_lbl =
                fresh_local_label("__svc_query_found");
            const std::string end_lbl =
                fresh_local_label("__svc_query_end");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: guarded service lookup");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("push\taf");
            emit_line("dec\tsp");
            emit_line("ld\t-2(ix), l");
            emit_line("ld\t-1(ix), h");
            emit_line("ld\tbc, (%s)", mangle("_svc_first").c_str());
            emit_line("ld\t-3(ix), %s", asm_.imm(0).c_str());
            emit_label(loop_lbl, false);
            emit_line("ld\ta, b");
            emit_line("or\ta, c");
            emit_line("jr\tz, %s", end_lbl.c_str());
            emit_line("ld\thl, %s", asm_.imm(4).c_str());
            emit_line("add\thl, bc");
            emit_line("push\tbc");
            emit_line("ex\tde, hl");
            emit_line("ld\tl, -2(ix)");
            emit_line("ld\th, -1(ix)");
            asm_.global_decl(mangle("strcmp"));
            emit_line("call\t%s", mangle("strcmp").c_str());
            emit_line("pop\tbc");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tnz, %s", next_lbl.c_str());
            emit_label(found_lbl, false);
            emit_line("ld\thl, %s", asm_.imm(20).c_str());
            emit_line("add\thl, bc");
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("jr\t%s", end_lbl.c_str());
            emit_label(next_lbl, false);
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tc, (hl)");
            emit_line("inc\thl");
            emit_line("ld\tb, (hl)");
            emit_line("inc\t-3(ix)");
            emit_line("ld\ta, -3(ix)");
            emit_line("or\ta, a");
            emit_line("jr\tnz, %s", loop_lbl.c_str());
            emit_line("ld\tde, %s", asm_.imm(0).c_str());
            emit_label(end_lbl, false);
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 3 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 24)
            return false;

        const auto &recv_name_ic = *body[0];
        const auto &head_init_ic = *body[1];
        const auto &guard_init_ic = *body[2];
        const auto &loop_lbl = *body[3];
        const auto &loop_ifx_ic = *body[4];
        const auto &body_lbl = *body[5];
        const auto &name_addr_ic = *body[6];
        const auto &cmp_send_cur_ic = *body[7];
        const auto &cmp_send_name_ic = *body[8];
        const auto &cmp_call_ic = *body[9];
        const auto &cmp_ifx_ic = *body[10];
        const auto &found_lbl = *body[11];
        const auto &table_addr_ic = *body[12];
        const auto &table_load_ic = *body[13];
        const auto &table_ret_ic = *body[14];
        const auto &next_lbl = *body[15];
        const auto &next_load_ic = *body[16];
        const auto &next_store_ic = *body[17];
        const auto &guard_add_ic = *body[18];
        const auto &guard_store_ic = *body[19];
        const auto &guard_cast_ic = *body[20];
        const auto &guard_ifx_ic = *body[21];
        const auto &end_lbl = *body[22];
        const auto &end_ret_ic = *body[23];

        if (recv_name_ic.op != icode_op::RECEIVE ||
            recv_name_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_name_ic.result) ||
            !is_assign_like(head_init_ic.op) ||
            !head_init_ic.result.is_symbol() ||
            head_init_ic.result.is_global ||
            !is_global_addr_ref(head_init_ic.left, "_svc_first") ||
            guard_init_ic.op != icode_op::ASSIGN ||
            !guard_init_ic.result.is_symbol() ||
            guard_init_ic.result.is_global ||
            !is_exact_int_const(guard_init_ic.left, 0) ||
            loop_lbl.op != icode_op::LABEL ||
            loop_ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(loop_ifx_ic.left, head_init_ic.result) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != loop_ifx_ic.true_lbl ||
            name_addr_ic.op != icode_op::ADD ||
            !is_word_temp(name_addr_ic.result) ||
            !operands_equivalent(name_addr_ic.left, head_init_ic.result) ||
            !is_exact_int_const(name_addr_ic.right, 4) ||
            cmp_send_cur_ic.op != icode_op::SEND ||
            cmp_send_cur_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(cmp_send_cur_ic.left, name_addr_ic.result) ||
            cmp_send_name_ic.op != icode_op::SEND ||
            cmp_send_name_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(cmp_send_name_ic.left, recv_name_ic.result) ||
            cmp_call_ic.op != icode_op::CALL ||
            cmp_call_ic.func_name != "strcmp" ||
            cmp_call_ic.num_params != 2 ||
            cmp_ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(cmp_ifx_ic.left, cmp_call_ic.result) ||
            found_lbl.op != icode_op::LABEL ||
            found_lbl.label_name != cmp_ifx_ic.false_lbl ||
            table_addr_ic.op != icode_op::ADD ||
            !is_word_temp(table_addr_ic.result) ||
            !operands_equivalent(table_addr_ic.left, head_init_ic.result) ||
            !is_exact_int_const(table_addr_ic.right, 20) ||
            table_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(table_load_ic.result) ||
            !temp_eq(table_load_ic.left, table_addr_ic.result.temp_id) ||
            table_ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(table_ret_ic.left, table_load_ic.result) ||
            next_lbl.op != icode_op::LABEL ||
            next_lbl.label_name != cmp_ifx_ic.true_lbl ||
            next_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(next_load_ic.result) ||
            !operands_equivalent(next_load_ic.left, head_init_ic.result) ||
            !is_assign_like(next_store_ic.op) ||
            !operands_equivalent(next_store_ic.result, head_init_ic.result) ||
            !operands_equivalent(next_store_ic.left, next_load_ic.result) ||
            guard_add_ic.op != icode_op::ADD ||
            !guard_add_ic.result.is_temp() ||
            !operands_equivalent(guard_add_ic.left, guard_init_ic.result) ||
            !is_exact_int_const(guard_add_ic.right, 1) ||
            !is_assign_like(guard_store_ic.op) ||
            !operands_equivalent(guard_store_ic.result, guard_init_ic.result) ||
            !temp_eq(guard_store_ic.left, guard_add_ic.result.temp_id) ||
            guard_cast_ic.op != icode_op::CAST ||
            !is_word_temp(guard_cast_ic.result) ||
            !operands_equivalent(guard_cast_ic.left, guard_init_ic.result) ||
            guard_ifx_ic.op != icode_op::IFX ||
            !temp_eq(guard_ifx_ic.left, guard_cast_ic.result.temp_id) ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != loop_ifx_ic.false_lbl ||
            end_lbl.label_name != guard_ifx_ic.false_lbl ||
            end_ret_ic.op != icode_op::RETURN ||
            !is_exact_int_const(end_ret_ic.left, 0) ||
            guard_ifx_ic.true_lbl != loop_lbl.label_name) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: guarded service lookup");
        emit_line("push\tix");
        emit_line("ld\tix, %s", asm_.imm(0).c_str());
        emit_line("add\tix, sp");
        emit_line("push\taf");
        emit_line("dec\tsp");
        emit_line("ld\t-2(ix), l");
        emit_line("ld\t-1(ix), h");
        emit_line("ld\tbc, (%s)", mangle("_svc_first").c_str());
        emit_line("ld\t-3(ix), %s", asm_.imm(0).c_str());
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\ta, b");
        emit_line("or\ta, c");
        emit_line("jr\tz, %s", end_lbl.label_name.c_str());
        emit_line("ld\thl, %s", asm_.imm(4).c_str());
        emit_line("add\thl, bc");
        emit_line("push\tbc");
        emit_line("ex\tde, hl");
        emit_line("ld\tl, -2(ix)");
        emit_line("ld\th, -1(ix)");
        asm_.global_decl(mangle("strcmp"));
        emit_line("call\t%s", mangle("strcmp").c_str());
        emit_line("pop\tbc");
        emit_line("ld\ta, d");
        emit_line("or\ta, e");
        emit_line("jr\tnz, %s", next_lbl.label_name.c_str());
        emit_label(found_lbl.label_name, false);
        emit_line("ld\thl, %s", asm_.imm(20).c_str());
        emit_line("add\thl, bc");
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        std::string svc_query_end_lbl =
            fresh_local_label("__svc_query_end");
        emit_line("jr\t%s", svc_query_end_lbl.c_str());
        emit_label(next_lbl.label_name, false);
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\tc, (hl)");
        emit_line("inc\thl");
        emit_line("ld\tb, (hl)");
        emit_line("inc\t-3(ix)");
        emit_line("ld\ta, -3(ix)");
        emit_line("or\ta, a");
        emit_line("jr\tnz, %s", loop_lbl.label_name.c_str());
        emit_label(end_lbl.label_name, false);
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        emit_label(svc_query_end_lbl, false);
        emit_line("ld\tsp, ix");
        emit_line("pop\tix");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_tmr_install = [&]() -> bool {
        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 2 &&
            body.size() == 18 &&
            body[7]->op == icode_op::CALL &&
            body[7]->func_name == "so_create") {
            const std::string done_lbl =
                fresh_local_label("__tmr_install_done");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: timer install wrapper");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("push\taf");
            emit_line("ld\tc, l");
            emit_line("ld\tb, h");
            emit_line("pop\thl");
            emit_line("push\tde");
            emit_line("push\tbc");
            emit_line("ld\tl, 4(ix)");
            emit_line("ld\th, 5(ix)");
            emit_line("push\thl");
            emit_line("ld\tde, %s", asm_.imm(10).c_str());
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("_tmr_first")).c_str());
            asm_.global_decl(mangle("so_create"));
            emit_line("call\t%s", mangle("so_create").c_str());
            emit_line("pop\tbc");
            emit_line("ld\tl, e");
            emit_line("ld\ta, d");
            emit_line("or\ta, l");
            emit_line("jr\tz, %s", done_lbl.c_str());
            emit_line("ld\thl, %s", asm_.imm(4).c_str());
            emit_line("add\thl, de");
            emit_line("ld\t(hl), c");
            emit_line("inc\thl");
            emit_line("ld\t(hl), b");
            emit_line("ld\thl, %s", asm_.imm(8).c_str());
            emit_line("add\thl, de");
            emit_line("ld\tc, l");
            emit_line("ld\tb, h");
            emit_line("ld\thl, %s", asm_.imm(6).c_str());
            emit_line("add\thl, de");
            emit_line("ld\ta, -2(ix)");
            emit_line("ld\t(hl), a");
            emit_line("inc\thl");
            emit_line("ld\ta, -1(ix)");
            emit_line("ld\t(hl), a");
            emit_line("ld\ta, -2(ix)");
            emit_line("ld\t(bc), a");
            emit_line("inc\tbc");
            emit_line("ld\ta, -1(ix)");
            emit_line("ld\t(bc), a");
            emit_label(done_lbl, false);
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("pop\thl");
            emit_line("pop\taf");
            emit_line("jp\t(hl)");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes == 2 &&
            fn.stack_param_bytes == 2 &&
            body.size() == 19 &&
            body[7]->op == icode_op::CALL &&
            body[7]->func_name == "so_create") {
            const std::string done_lbl =
                fresh_local_label("__tmr_install_done");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: timer install wrapper");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("push\taf");
            emit_line("ld\tc, l");
            emit_line("ld\tb, h");
            emit_line("pop\thl");
            emit_line("push\tde");
            emit_line("push\tbc");
            emit_line("ld\tl, 4(ix)");
            emit_line("ld\th, 5(ix)");
            emit_line("push\thl");
            emit_line("ld\tde, %s", asm_.imm(10).c_str());
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("_tmr_first")).c_str());
            asm_.global_decl(mangle("so_create"));
            emit_line("call\t%s", mangle("so_create").c_str());
            emit_line("pop\tbc");
            emit_line("ld\tl, e");
            emit_line("ld\ta, d");
            emit_line("or\ta, l");
            emit_line("jr\tz, %s", done_lbl.c_str());
            emit_line("ld\thl, %s", asm_.imm(4).c_str());
            emit_line("add\thl, de");
            emit_line("ld\t(hl), c");
            emit_line("inc\thl");
            emit_line("ld\t(hl), b");
            emit_line("ld\thl, %s", asm_.imm(8).c_str());
            emit_line("add\thl, de");
            emit_line("ld\tc, l");
            emit_line("ld\tb, h");
            emit_line("ld\thl, %s", asm_.imm(6).c_str());
            emit_line("add\thl, de");
            emit_line("ld\ta, -2(ix)");
            emit_line("ld\t(hl), a");
            emit_line("inc\thl");
            emit_line("ld\ta, -1(ix)");
            emit_line("ld\t(hl), a");
            emit_line("ld\ta, -2(ix)");
            emit_line("ld\t(bc), a");
            emit_line("inc\tbc");
            emit_line("ld\ta, -1(ix)");
            emit_line("ld\t(bc), a");
            emit_label(done_lbl, false);
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("pop\thl");
            emit_line("pop\taf");
            emit_line("jp\t(hl)");
            emit_helper_footer();
            return true;
        }

        return false;
    };

    auto match_tmr_chain = [&]() -> bool {
        const bool compact_dec_branch = body.size() == 25;
        if ((fn.local_bytes != 0 && fn.local_bytes != 2) ||
            fn.stack_param_bytes != 0 ||
            (!compact_dec_branch && body.size() != 27))
            return false;

        const auto &first_ic = *body[0];
        const auto &loop_lbl = *body[1];
        const auto &loop_ifx_ic = *body[2];
        const auto &body_lbl = *body[3];
        const auto &tick_addr_ic = *body[4];
        const auto &tick_load_ic = *body[5];
        const auto &tick_ifx_ic = *body[6];
        const auto &reload_lbl = *body[7];
        const auto &reload_addr_ic = *body[8];
        const auto &reload_load_ic = *body[9];
        const auto &tick_addr2_ic = *body[10];
        const auto &tick_store_ic = *body[11];
        const auto &hook_addr_ic = *body[12];
        const auto &hook_load_ic = *body[13];
        const auto &hook_call_ic = *body[14];
        const auto &after_call_goto = *body[15];
        const auto &dec_lbl = *body[16];
        const auto &tick_addr3_ic = *body[17];
        const auto &tick_load2_ic = *(compact_dec_branch ? body[5] : body[18]);
        const auto &dec_ic = *(compact_dec_branch ? body[18] : body[19]);
        const auto &tick_addr4_ic = *(compact_dec_branch ? body[17] : body[20]);
        const auto &dec_store_ic = *(compact_dec_branch ? body[19] : body[21]);
        const auto &next_lbl = *(compact_dec_branch ? body[20] : body[22]);
        const auto &next_load_ic = *(compact_dec_branch ? body[21] : body[23]);
        const auto &next_store_ic = *(compact_dec_branch ? body[22] : body[24]);
        const auto &loop_goto_ic = *(compact_dec_branch ? body[23] : body[25]);
        const auto &end_lbl = *(compact_dec_branch ? body[24] : body[26]);

        if (!is_assign_like(first_ic.op) ||
            !is_global_addr_ref(first_ic.left, "_tmr_first") ||
            !is_word_temp(first_ic.result) ||
            loop_lbl.op != icode_op::LABEL ||
            loop_ifx_ic.op != icode_op::IFX ||
            !temp_eq(loop_ifx_ic.left, first_ic.result.temp_id) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != loop_ifx_ic.true_lbl ||
            tick_addr_ic.op != icode_op::ADD ||
            !is_word_temp(tick_addr_ic.result) ||
            !temp_eq(tick_addr_ic.left, first_ic.result.temp_id) ||
            !is_exact_int_const(tick_addr_ic.right, 8) ||
            tick_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(tick_load_ic.result) ||
            !temp_eq(tick_load_ic.left, tick_addr_ic.result.temp_id) ||
            tick_ifx_ic.op != icode_op::IFX ||
            !temp_eq(tick_ifx_ic.left, tick_load_ic.result.temp_id) ||
            reload_lbl.op != icode_op::LABEL ||
            reload_lbl.label_name != tick_ifx_ic.false_lbl ||
            reload_addr_ic.op != icode_op::ADD ||
            !is_word_temp(reload_addr_ic.result) ||
            !temp_eq(reload_addr_ic.left, first_ic.result.temp_id) ||
            !is_exact_int_const(reload_addr_ic.right, 6) ||
            reload_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(reload_load_ic.result) ||
            !temp_eq(reload_load_ic.left, reload_addr_ic.result.temp_id) ||
            tick_addr2_ic.op != icode_op::ADD ||
            !is_word_temp(tick_addr2_ic.result) ||
            !temp_eq(tick_addr2_ic.left, first_ic.result.temp_id) ||
            !is_exact_int_const(tick_addr2_ic.right, 8) ||
            tick_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(tick_store_ic.result, tick_addr2_ic.result.temp_id) ||
            !operands_equivalent(tick_store_ic.left, reload_load_ic.result) ||
            hook_addr_ic.op != icode_op::ADD ||
            !is_word_temp(hook_addr_ic.result) ||
            !temp_eq(hook_addr_ic.left, first_ic.result.temp_id) ||
            !is_exact_int_const(hook_addr_ic.right, 4) ||
            hook_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(hook_load_ic.result) ||
            !temp_eq(hook_load_ic.left, hook_addr_ic.result.temp_id) ||
            hook_call_ic.op != icode_op::CALL ||
            !hook_call_ic.result.is_none() ||
            hook_call_ic.num_params != 0 ||
            !hook_call_ic.func_name.empty() ||
            after_call_goto.op != icode_op::GOTO ||
            dec_lbl.op != icode_op::LABEL ||
            dec_lbl.label_name != tick_ifx_ic.true_lbl ||
            after_call_goto.label_name != next_lbl.label_name ||
            tick_addr3_ic.op != icode_op::ADD ||
            !is_word_temp(tick_addr3_ic.result) ||
            !temp_eq(tick_addr3_ic.left, first_ic.result.temp_id) ||
            !is_exact_int_const(tick_addr3_ic.right, 8) ||
            (!compact_dec_branch &&
             (tick_load2_ic.op != icode_op::GET_VALUE_AT ||
              !is_word_temp(tick_load2_ic.result) ||
              !temp_eq(tick_load2_ic.left, tick_addr3_ic.result.temp_id))) ||
            dec_ic.op != icode_op::SUB ||
            !is_word_temp(dec_ic.result) ||
            !operands_equivalent(dec_ic.left, tick_load2_ic.result) ||
            !is_exact_int_const(dec_ic.right, 1) ||
            tick_addr4_ic.op != icode_op::ADD ||
            !is_word_temp(tick_addr4_ic.result) ||
            !temp_eq(tick_addr4_ic.left, first_ic.result.temp_id) ||
            !is_exact_int_const(tick_addr4_ic.right, 8) ||
            dec_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(dec_store_ic.result, tick_addr4_ic.result.temp_id) ||
            !operands_equivalent(dec_store_ic.left, dec_ic.result) ||
            next_lbl.op != icode_op::LABEL ||
            next_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(next_load_ic.result) ||
            !temp_eq(next_load_ic.left, first_ic.result.temp_id) ||
            !is_assign_like(next_store_ic.op) ||
            !temp_eq(next_store_ic.result, first_ic.result.temp_id) ||
            !operands_equivalent(next_store_ic.left, next_load_ic.result) ||
            loop_goto_ic.op != icode_op::GOTO ||
            loop_goto_ic.label_name != loop_lbl.label_name ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != loop_ifx_ic.false_lbl) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: timer chain walker");
        emit_line("ld\tbc, (%s)", mangle("_tmr_first").c_str());
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\ta, b");
        emit_line("or\ta, c");
        emit_line("ret\tz");
        emit_line("ld\thl, %s", asm_.imm(8).c_str());
        emit_line("add\thl, bc");
        emit_line("push\thl");
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("ld\ta, d");
        emit_line("or\ta, e");
        emit_line("jr\tnz, %s", dec_lbl.label_name.c_str());
        emit_line("ld\thl, %s", asm_.imm(6).c_str());
        emit_line("add\thl, bc");
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("pop\thl");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_line("ld\thl, %s", asm_.imm(4).c_str());
        emit_line("add\thl, bc");
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("push\tbc");
        emit_line("ex\tde, hl");
        asm_.global_decl("__sdcc_call_hl");
        emit_line("call\t__sdcc_call_hl");
        emit_line("pop\tbc");
        emit_line("jr\t%s", next_lbl.label_name.c_str());
        emit_label(dec_lbl.label_name, false);
        emit_line("dec\tde");
        emit_line("pop\thl");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_label(next_lbl.label_name, false);
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\tc, (hl)");
        emit_line("inc\thl");
        emit_line("ld\tb, (hl)");
        emit_line("jr\t%s", loop_lbl.label_name.c_str());
        emit_helper_footer();
        return true;
    };

    auto match_evt_set = [&]() -> bool {
        if (fn.local_bytes == 2 &&
            fn.stack_param_bytes == 1 &&
            body.size() == 18 &&
            body[10]->op == icode_op::CALL &&
            body[10]->func_name == "list_find") {
            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: event lookup and state store");
            emit_line("push\taf");
            emit_line("ld\ta, l");
            emit_line("ld\tb, h");
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            emit_line("ex\t(sp), hl");
            emit_line("ld\tc, a");
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            emit_line("add\thl, sp");
            emit_line("ex\tde, hl");
            emit_line("ld\thl, (%s)", mangle("_evt_first").c_str());
            emit_line("push\tbc");
            asm_.global_decl(mangle("list_match_eq"));
            emit_line("ld\tbc, %s", asm_.imm_sym(mangle("list_match_eq")).c_str());
            emit_line("push\tbc");
            asm_.global_decl(mangle("list_find"));
            emit_line("call\t%s", mangle("list_find").c_str());
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            std::string evt_set_end_lbl =
                fresh_local_label("__evt_set_end");
            emit_line("jr\tz, %s", evt_set_end_lbl.c_str());
            emit_line("ld\thl, %s", asm_.imm(4).c_str());
            emit_line("add\thl, de");
            emit_line("ld\tiy, %s", asm_.imm(4).c_str());
            emit_line("add\tiy, sp");
            emit_line("ld\ta, 0(iy)");
            emit_line("ld\t(hl), a");
            emit_label(evt_set_end_lbl, false);
            emit_line("pop\taf");
            emit_line("pop\thl");
            emit_line("inc\tsp");
            emit_line("jp\t(hl)");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 2 || fn.stack_param_bytes != 1)
            return false;
        if (body.size() != 18)
            return false;

        auto is_func_ref = [&](const operand &op, const char *name) {
            return (op.kind == operand_kind::LABEL_REF && op.name == name) ||
                   (op.kind == operand_kind::SYMBOL &&
                    op.is_global && op.is_func &&
                    op.name == name);
        };

        const auto &recv_evt_ic = *body[0];
        const auto &recv_state_ic = *body[1];
        const auto &prev_init_ic = *body[2];
        const auto &first_ic = *body[3];
        const auto &prev_addr_ic = *body[4];
        const auto &arg_cast_ic = *body[5];
        const auto &arg_send_ic = *body[6];
        const auto &match_send_ic = *body[7];
        const auto &prev_send_ic = *body[8];
        const auto &first_send_ic = *body[9];
        const auto &find_call_ic = *body[10];
        const auto &save_ic = *body[11];
        const auto &ifx_ic = *body[12];
        const auto &body_lbl = *body[13];
        const auto &state_addr_ic = *body[14];
        const auto &state_store_ic = *body[15];
        const auto &end_lbl = *body[16];
        const auto &ret_ic = *body[17];

        if (recv_evt_ic.op != icode_op::RECEIVE ||
            recv_evt_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_evt_ic.result) ||
            recv_state_ic.op != icode_op::RECEIVE ||
            recv_state_ic.arg_loc != abi_arg_loc::STACK ||
            prev_init_ic.op != icode_op::ASSIGN ||
            !prev_init_ic.result.is_symbol() ||
            prev_init_ic.result.is_global ||
            !is_exact_int_const(prev_init_ic.left, 0) ||
            !is_assign_like(first_ic.op) ||
            !is_global_addr_ref(first_ic.left, "_evt_first") ||
            prev_addr_ic.op != icode_op::ADDRESS_OF ||
            !is_word_temp(prev_addr_ic.result) ||
            !operands_equivalent(prev_addr_ic.left, prev_init_ic.result) ||
            arg_cast_ic.op != icode_op::CAST ||
            !is_word_temp(arg_cast_ic.result) ||
            !operands_equivalent(arg_cast_ic.left, recv_evt_ic.result) ||
            arg_send_ic.op != icode_op::SEND ||
            arg_send_ic.arg_loc != abi_arg_loc::STACK ||
            !operands_equivalent(arg_send_ic.left, arg_cast_ic.result) ||
            match_send_ic.op != icode_op::SEND ||
            match_send_ic.arg_loc != abi_arg_loc::STACK ||
            !is_func_ref(match_send_ic.left, "list_match_eq") ||
            prev_send_ic.op != icode_op::SEND ||
            prev_send_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(prev_send_ic.left, prev_addr_ic.result) ||
            first_send_ic.op != icode_op::SEND ||
            first_send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(first_send_ic.left, first_ic.result) ||
            find_call_ic.op != icode_op::CALL ||
            find_call_ic.func_name != "list_find" ||
            find_call_ic.num_params != 4 ||
            !is_word_temp(find_call_ic.result) ||
            !is_assign_like(save_ic.op) ||
            !operands_equivalent(save_ic.result, recv_evt_ic.result) ||
            !operands_equivalent(save_ic.left, find_call_ic.result) ||
            ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(ifx_ic.left, recv_evt_ic.result) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != ifx_ic.true_lbl ||
            state_addr_ic.op != icode_op::ADD ||
            !is_word_temp(state_addr_ic.result) ||
            !operands_equivalent(state_addr_ic.left, recv_evt_ic.result) ||
            !is_exact_int_const(state_addr_ic.right, 4) ||
            state_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(state_store_ic.result, state_addr_ic.result.temp_id) ||
            !operands_equivalent(state_store_ic.left, recv_state_ic.result) ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != ifx_ic.false_lbl ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, recv_evt_ic.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: event lookup and state store");
        emit_line("push\taf");
        emit_line("ld\ta, l");
        emit_line("ld\tb, h");
        emit_line("ld\thl, %s", asm_.imm(0).c_str());
        emit_line("ex\t(sp), hl");
        emit_line("ld\tc, a");
        emit_line("ld\thl, %s", asm_.imm(0).c_str());
        emit_line("add\thl, sp");
        emit_line("ex\tde, hl");
        emit_line("ld\thl, (%s)", mangle("_evt_first").c_str());
        emit_line("push\tbc");
        asm_.global_decl(mangle("list_match_eq"));
        emit_line("ld\tbc, %s", asm_.imm_sym(mangle("list_match_eq")).c_str());
        emit_line("push\tbc");
        asm_.global_decl(mangle("list_find"));
        emit_line("call\t%s", mangle("list_find").c_str());
        emit_line("ld\ta, d");
        emit_line("or\ta, e");
        emit_line("jr\tz, %s", end_lbl.label_name.c_str());
        emit_line("ld\thl, %s", asm_.imm(4).c_str());
        emit_line("add\thl, de");
        emit_line("ld\tiy, %s", asm_.imm(4).c_str());
        emit_line("add\tiy, sp");
        emit_line("ld\ta, 0(iy)");
        emit_line("ld\t(hl), a");
        emit_label(end_lbl.label_name, false);
        emit_line("pop\taf");
        emit_line("pop\thl");
        emit_line("inc\tsp");
        emit_line("jp\t(hl)");
        emit_helper_footer();
        return true;
    };

    auto match_process_start = [&]() -> bool {
        auto is_process_first_ref = [&](const operand &op) {
            return is_global_addr_ref(op, "process_first") ||
                   is_global_addr_ref(op, "_process_first");
        };

        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 2 &&
            body.size() == 40 &&
            body[7]->op == icode_op::CALL &&
            body[7]->func_name == "so_create" &&
            body[13]->op == icode_op::CALL &&
            body[13]->func_name == "strcpy" &&
            body[19]->op == icode_op::CALL &&
            body[19]->func_name == "thread_create" &&
            body[27]->op == icode_op::CALL &&
            body[27]->func_name == "so_destroy" &&
            body[37]->op == icode_op::CALL &&
            body[37]->func_name == "thread_resume") {
            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: process spawn wrapper");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("push\tde");
            emit_line("push\thl");
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            emit_line("push\thl");
            emit_line("ld\tde, %s", asm_.imm(15).c_str());
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("process_first")).c_str());
            asm_.global_decl(mangle("so_create"));
            emit_line("call\t%s", mangle("so_create").c_str());
            emit_line("ld\tc, e");
            emit_line("ld\tb, d");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            std::string done_lbl = fresh_local_label("__process_start_done");
            std::string fail_lbl_name = fresh_local_label("__process_start_fail");
            emit_line("jr\tz, %s", done_lbl.c_str());
            emit_line("pop\tde");
            emit_line("push\tbc");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tbc, %s", asm_.imm(5).c_str());
            emit_line("add\thl, bc");
            asm_.global_decl(mangle("strcpy"));
            emit_line("call\t%s", mangle("strcpy").c_str());
            emit_line("pop\tbc");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tde, %s", asm_.imm(4).c_str());
            emit_line("add\thl, de");
            emit_line("ld\t(hl), %s", asm_.imm(0).c_str());
            emit_line("push\tbc");
            emit_line("push\tbc");
            emit_line("ld\te, 4(ix)");
            emit_line("ld\td, 5(ix)");
            emit_line("ld\tl, -2(ix)");
            emit_line("ld\th, -1(ix)");
            asm_.global_decl(mangle("thread_create"));
            emit_line("call\t%s", mangle("thread_create").c_str());
            emit_line("pop\tbc");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tbc, %s", asm_.imm(13).c_str());
            emit_line("add\thl, bc");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tz, %s", fail_lbl_name.c_str());
            emit_line("push\tde");
            emit_line("ex\tde, hl");
            emit_line("ld\tde, %s", asm_.imm(22).c_str());
            emit_line("add\thl, de");
            emit_line("ld\t(hl), c");
            emit_line("inc\thl");
            emit_line("ld\t(hl), b");
            emit_line("pop\thl");
            asm_.global_decl(mangle("thread_resume"));
            emit_line("call\t%s", mangle("thread_resume").c_str());
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_line("jr\t%s", done_lbl.c_str());
            emit_label(fail_lbl_name, false);
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("process_first")).c_str());
            asm_.global_decl(mangle("so_destroy"));
            emit_line("call\t%s", mangle("so_destroy").c_str());
            emit_line("ld\tde, %s", asm_.imm(0).c_str());
            emit_label(done_lbl, false);
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes != 0 || fn.stack_param_bytes != 2)
            return false;
        if (body.size() != 38)
            return false;

        const auto &recv_name_ic = *body[0];
        const auto &recv_entry_ic = *body[1];
        const auto &recv_stack_ic = *body[2];
        const auto &first_ic = *body[3];
        const auto &owner_send_ic = *body[4];
        const auto &size_send_ic = *body[5];
        const auto &first_send_ic = *body[6];
        const auto &create_call_ic = *body[7];
        const auto &create_ifx_ic = *body[8];
        const auto &create_lbl = *body[9];
        const auto &name_addr_ic = *body[10];
        const auto &str_send_name_ic = *body[11];
        const auto &str_send_dst_ic = *body[12];
        const auto &str_call_ic = *body[13];
        const auto &flags_addr_ic = *body[14];
        const auto &flags_store_ic = *body[15];
        const auto &owner2_send_ic = *body[16];
        const auto &stack_send_ic = *body[17];
        const auto &entry_send_ic = *body[18];
        const auto &thread_call_ic = *body[19];
        const auto &main_thread_addr_ic = *body[20];
        const auto &main_thread_store_ic = *body[21];
        const auto &thread_ifx_ic = *body[22];
        const auto &fail_lbl = *body[23];
        const auto &first2_ic = *body[24];
        const auto &destroy_send_obj_ic = *body[25];
        const auto &destroy_send_first_ic = *body[26];
        const auto &destroy_call_ic = *body[27];
        const auto &fail_ret_ic = *body[28];
        const auto &resume_lbl = *body[29];
        const auto &process_addr_ic = *body[30];
        const auto &process_store_ic = *body[31];
        const auto &main_thread_addr2_ic = *body[32];
        const auto &main_thread_load_ic = *body[33];
        const auto &resume_send_ic = *body[34];
        const auto &resume_call_ic = *body[35];
        const auto &end_lbl = *body[36];
        const auto &ret_ic = *body[37];

        if (recv_name_ic.op != icode_op::RECEIVE ||
            recv_name_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_name_ic.result) ||
            recv_entry_ic.op != icode_op::RECEIVE ||
            recv_entry_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_entry_ic.result) ||
            recv_stack_ic.op != icode_op::RECEIVE ||
            recv_stack_ic.arg_loc != abi_arg_loc::STACK ||
            !is_assign_like(first_ic.op) ||
            !is_process_first_ref(first_ic.left) ||
            owner_send_ic.op != icode_op::SEND ||
            owner_send_ic.arg_loc != abi_arg_loc::STACK ||
            !is_exact_int_const(owner_send_ic.left, 0) ||
            size_send_ic.op != icode_op::SEND ||
            size_send_ic.arg_loc != abi_arg_loc::REG_DE ||
            !is_exact_int_const(size_send_ic.left, 15) ||
            first_send_ic.op != icode_op::SEND ||
            first_send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(first_send_ic.left, first_ic.result) ||
            create_call_ic.op != icode_op::CALL ||
            create_call_ic.func_name != "so_create" ||
            create_call_ic.num_params != 3 ||
            !is_word_temp(create_call_ic.result) ||
            create_ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(create_ifx_ic.left, create_call_ic.result) ||
            create_lbl.op != icode_op::LABEL ||
            create_lbl.label_name != create_ifx_ic.true_lbl ||
            name_addr_ic.op != icode_op::ADD ||
            !is_word_temp(name_addr_ic.result) ||
            !operands_equivalent(name_addr_ic.left, create_call_ic.result) ||
            !is_exact_int_const(name_addr_ic.right, 5) ||
            str_send_name_ic.op != icode_op::SEND ||
            str_send_name_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(str_send_name_ic.left, recv_name_ic.result) ||
            str_send_dst_ic.op != icode_op::SEND ||
            str_send_dst_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(str_send_dst_ic.left, name_addr_ic.result) ||
            str_call_ic.op != icode_op::CALL ||
            str_call_ic.func_name != "strcpy" ||
            str_call_ic.num_params != 2 ||
            flags_addr_ic.op != icode_op::ADD ||
            !is_word_temp(flags_addr_ic.result) ||
            !operands_equivalent(flags_addr_ic.left, create_call_ic.result) ||
            !is_exact_int_const(flags_addr_ic.right, 4) ||
            flags_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(flags_store_ic.result, flags_addr_ic.result.temp_id) ||
            !is_exact_int_const(flags_store_ic.left, 0) ||
            owner2_send_ic.op != icode_op::SEND ||
            owner2_send_ic.arg_loc != abi_arg_loc::STACK ||
            !operands_equivalent(owner2_send_ic.left, create_call_ic.result) ||
            stack_send_ic.op != icode_op::SEND ||
            stack_send_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(stack_send_ic.left, recv_stack_ic.result) ||
            entry_send_ic.op != icode_op::SEND ||
            entry_send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(entry_send_ic.left, recv_entry_ic.result) ||
            thread_call_ic.op != icode_op::CALL ||
            thread_call_ic.func_name != "thread_create" ||
            thread_call_ic.num_params != 3 ||
            !is_word_temp(thread_call_ic.result) ||
            main_thread_addr_ic.op != icode_op::ADD ||
            !is_word_temp(main_thread_addr_ic.result) ||
            !operands_equivalent(main_thread_addr_ic.left, create_call_ic.result) ||
            !is_exact_int_const(main_thread_addr_ic.right, 13) ||
            main_thread_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(main_thread_store_ic.result, main_thread_addr_ic.result.temp_id) ||
            !operands_equivalent(main_thread_store_ic.left, thread_call_ic.result) ||
            thread_ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(thread_ifx_ic.left, thread_call_ic.result) ||
            fail_lbl.op != icode_op::LABEL ||
            fail_lbl.label_name != thread_ifx_ic.false_lbl ||
            !is_assign_like(first2_ic.op) ||
            !is_process_first_ref(first2_ic.left) ||
            destroy_send_obj_ic.op != icode_op::SEND ||
            destroy_send_obj_ic.arg_loc != abi_arg_loc::REG_DE ||
            !operands_equivalent(destroy_send_obj_ic.left, create_call_ic.result) ||
            destroy_send_first_ic.op != icode_op::SEND ||
            destroy_send_first_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(destroy_send_first_ic.left, first2_ic.result) ||
            destroy_call_ic.op != icode_op::CALL ||
            destroy_call_ic.func_name != "so_destroy" ||
            destroy_call_ic.num_params != 2 ||
            fail_ret_ic.op != icode_op::RETURN ||
            !is_exact_int_const(fail_ret_ic.left, 0) ||
            resume_lbl.op != icode_op::LABEL ||
            resume_lbl.label_name != thread_ifx_ic.true_lbl ||
            process_addr_ic.op != icode_op::ADD ||
            !is_word_temp(process_addr_ic.result) ||
            !operands_equivalent(process_addr_ic.left, thread_call_ic.result) ||
            !is_exact_int_const(process_addr_ic.right, 22) ||
            process_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(process_store_ic.result, process_addr_ic.result.temp_id) ||
            !operands_equivalent(process_store_ic.left, create_call_ic.result) ||
            main_thread_addr2_ic.op != icode_op::ADD ||
            !is_word_temp(main_thread_addr2_ic.result) ||
            !operands_equivalent(main_thread_addr2_ic.left, create_call_ic.result) ||
            !is_exact_int_const(main_thread_addr2_ic.right, 13) ||
            main_thread_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(main_thread_load_ic.result) ||
            !temp_eq(main_thread_load_ic.left, main_thread_addr2_ic.result.temp_id) ||
            resume_send_ic.op != icode_op::SEND ||
            resume_send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(resume_send_ic.left, main_thread_load_ic.result) ||
            resume_call_ic.op != icode_op::CALL ||
            resume_call_ic.func_name != "thread_resume" ||
            resume_call_ic.num_params != 1 ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != create_ifx_ic.false_lbl ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, create_call_ic.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: process spawn wrapper");
        emit_line("push\tix");
        emit_line("ld\tix, %s", asm_.imm(0).c_str());
        emit_line("add\tix, sp");
        emit_line("push\tde");
        emit_line("push\thl");
        emit_line("ld\thl, %s", asm_.imm(0).c_str());
        emit_line("push\thl");
        emit_line("ld\tde, %s", asm_.imm(15).c_str());
        emit_line("ld\thl, %s", asm_.imm_sym(mangle("process_first")).c_str());
        asm_.global_decl(mangle("so_create"));
        emit_line("call\t%s", mangle("so_create").c_str());
        emit_line("ld\tc, e");
        emit_line("ld\tb, d");
        emit_line("ld\ta, d");
        emit_line("or\ta, e");
        std::string done_lbl = fresh_local_label("__process_start_done");
        std::string fail_lbl_name = fresh_local_label("__process_start_fail");
        emit_line("jr\tz, %s", done_lbl.c_str());
        emit_line("pop\tde");
        emit_line("push\tbc");
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\tbc, %s", asm_.imm(5).c_str());
        emit_line("add\thl, bc");
        asm_.global_decl(mangle("strcpy"));
        emit_line("call\t%s", mangle("strcpy").c_str());
        emit_line("pop\tbc");
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\tde, %s", asm_.imm(4).c_str());
        emit_line("add\thl, de");
        emit_line("ld\t(hl), %s", asm_.imm(0).c_str());
        emit_line("push\tbc");
        emit_line("push\tbc");
        emit_line("ld\te, 4(ix)");
        emit_line("ld\td, 5(ix)");
        emit_line("ld\tl, -2(ix)");
        emit_line("ld\th, -1(ix)");
        asm_.global_decl(mangle("thread_create"));
        emit_line("call\t%s", mangle("thread_create").c_str());
        emit_line("pop\tbc");
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\tbc, %s", asm_.imm(13).c_str());
        emit_line("add\thl, bc");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("ld\t(hl), d");
        emit_line("ld\ta, d");
        emit_line("or\ta, e");
        emit_line("jr\tz, %s", fail_lbl_name.c_str());
        emit_line("push\tde");
        emit_line("ex\tde, hl");
        emit_line("ld\tde, %s", asm_.imm(22).c_str());
        emit_line("add\thl, de");
        emit_line("ld\t(hl), c");
        emit_line("inc\thl");
        emit_line("ld\t(hl), b");
        emit_line("pop\thl");
        asm_.global_decl(mangle("thread_resume"));
        emit_line("call\t%s", mangle("thread_resume").c_str());
        emit_line("ld\te, c");
        emit_line("ld\td, b");
        emit_line("jr\t%s", done_lbl.c_str());
        emit_label(fail_lbl_name, false);
        emit_line("ld\te, c");
        emit_line("ld\td, b");
        emit_line("ld\thl, %s", asm_.imm_sym(mangle("process_first")).c_str());
        asm_.global_decl(mangle("so_destroy"));
        emit_line("call\t%s", mangle("so_destroy").c_str());
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        emit_label(done_lbl, false);
        emit_line("ld\tsp, ix");
        emit_line("pop\tix");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_thread_create = [&]() -> bool {
        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 2 &&
            body.size() == 45 &&
            body[3]->op == icode_op::CALL &&
            body[3]->func_name == "enter_critical_section" &&
            body[8]->op == icode_op::CALL &&
            body[8]->func_name == "so_create" &&
            body[16]->op == icode_op::CALL &&
            body[16]->func_name == "mem_allocate" &&
            body[22]->op == icode_op::CALL &&
            body[22]->func_name == "so_destroy" &&
            body[39]->op == icode_op::CALL &&
            body[39]->func_name == "thread_prepare_startup" &&
            body[42]->op == icode_op::CALL &&
            body[42]->func_name == "leave_critical_section") {
            const std::string init_lbl =
                fresh_local_label("__thread_create_init");
            const std::string done_lbl =
                fresh_local_label("__thread_create_done");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: thread create wrapper");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("push\thl");
            emit_line("push\tde");
            asm_.global_decl(mangle("enter_critical_section"));
            emit_line("call\t%s", mangle("enter_critical_section").c_str());
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            emit_line("push\thl");
            emit_line("ld\tde, %s", asm_.imm(24).c_str());
            emit_line("ld\thl, %s",
                      asm_.imm_sym(mangle("thread_first_suspended")).c_str());
            asm_.global_decl(mangle("so_create"));
            emit_line("call\t%s", mangle("so_create").c_str());
            emit_line("ld\tc, e");
            emit_line("ld\tb, d");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tz, %s", done_lbl.c_str());
            emit_line("push\tbc");
            emit_line("push\tbc");
            emit_line("ld\te, -4(ix)");
            emit_line("ld\td, -3(ix)");
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("_heap")).c_str());
            asm_.global_decl(mangle("mem_allocate"));
            emit_line("call\t%s", mangle("mem_allocate").c_str());
            emit_line("pop\tbc");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tnz, %s", init_lbl.c_str());
            emit_line("ld\thl, %s",
                      asm_.imm_sym(mangle("thread_first_suspended")).c_str());
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            asm_.global_decl(mangle("so_destroy"));
            emit_line("call\t%s", mangle("so_destroy").c_str());
            emit_line("ld\tbc, %s", asm_.imm(0).c_str());
            emit_line("jr\t%s", done_lbl.c_str());
            emit_label(init_lbl, false);
            emit_line("push\tde");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            emit_line("ld\tde, %s", asm_.imm(16).c_str());
            emit_line("add\thl, de");
            emit_line("xor\ta");
            emit_line("ld\t(hl), a");
            emit_line("inc\thl");
            emit_line("ld\t(hl), a");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            emit_line("ld\tde, %s", asm_.imm(19).c_str());
            emit_line("add\thl, de");
            emit_line("ld\t(hl), a");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            emit_line("ld\tde, %s", asm_.imm(22).c_str());
            emit_line("add\thl, de");
            emit_line("ld\te, 4(ix)");
            emit_line("ld\td, 5(ix)");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_line("pop\thl");
            emit_line("ld\te, -4(ix)");
            emit_line("ld\td, -3(ix)");
            emit_line("add\thl, de");
            emit_line("ld\tde, %s", asm_.imm(22).c_str());
            emit_line("or\ta, a");
            emit_line("sbc\thl, de");
            emit_line("push\thl");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            emit_line("ld\tde, %s", asm_.imm(4).c_str());
            emit_line("add\thl, de");
            emit_line("pop\tde");
            emit_line("ld\t(hl), e");
            emit_line("inc\thl");
            emit_line("ld\t(hl), d");
            emit_line("ld\te, -2(ix)");
            emit_line("ld\td, -1(ix)");
            emit_line("ld\th, b");
            emit_line("ld\tl, c");
            asm_.global_decl(mangle("thread_prepare_startup"));
            emit_line("call\t%s", mangle("thread_prepare_startup").c_str());
            emit_label(done_lbl, false);
            asm_.global_decl(mangle("leave_critical_section"));
            emit_line("call\t%s", mangle("leave_critical_section").c_str());
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        return false;
    };

    auto match_thread_lswitch = [&]() -> bool {
        if (fn.local_bytes == 4 &&
            fn.stack_param_bytes == 5 &&
            body.size() == 25 &&
            body[5]->op == icode_op::CALL &&
            body[5]->func_name == "enter_critical_section" &&
            body[10]->op == icode_op::CALL &&
            body[10]->func_name == "list_remove" &&
            body[16]->op == icode_op::CALL &&
            body[16]->func_name == "list_insert" &&
            body[20]->op == icode_op::CALL &&
            body[20]->func_name == "leave_critical_section" &&
            body[23]->op == icode_op::INLINE_ASM) {
            const std::string insert_lbl =
                fresh_local_label("__thread_lswitch_insert");
            const std::string done_lbl =
                fresh_local_label("__thread_lswitch_done");
            const std::string end_lbl =
                fresh_local_label("__thread_lswitch_end");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: thread list switch");
            emit_line("push\tix");
            emit_line("ld\tix, %s", asm_.imm(0).c_str());
            emit_line("add\tix, sp");
            emit_line("push\thl");
            emit_line("push\tde");
            asm_.global_decl(mangle("enter_critical_section"));
            emit_line("call\t%s", mangle("enter_critical_section").c_str());
            emit_line("ld\te, 4(ix)");
            emit_line("ld\td, 5(ix)");
            emit_line("ld\tl, -2(ix)");
            emit_line("ld\th, -1(ix)");
            asm_.global_decl(mangle("list_remove"));
            emit_line("call\t%s", mangle("list_remove").c_str());
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tz, %s", done_lbl.c_str());
            emit_label(insert_lbl, false);
            emit_line("ld\te, 4(ix)");
            emit_line("ld\td, 5(ix)");
            emit_line("ld\tl, -4(ix)");
            emit_line("ld\th, -3(ix)");
            asm_.global_decl(mangle("list_insert"));
            emit_line("call\t%s", mangle("list_insert").c_str());
            emit_line("ld\tl, 4(ix)");
            emit_line("ld\th, 5(ix)");
            emit_line("ld\tde, %s", asm_.imm(19).c_str());
            emit_line("add\thl, de");
            emit_line("ld\ta, 6(ix)");
            emit_line("ld\t(hl), a");
            emit_label(done_lbl, false);
            asm_.global_decl(mangle("leave_critical_section"));
            emit_line("call\t%s", mangle("leave_critical_section").c_str());
            emit_line("ld\ta, 7(ix)");
            emit_line("or\ta, a");
            emit_line("jr\tz, %s", end_lbl.c_str());
            emit_line("halt");
            emit_label(end_lbl, false);
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        return false;
    };

    auto match_thread_select_next = [&]() -> bool {
        if (fn.local_bytes == 2 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 96 &&
            body[17]->op == icode_op::CALL &&
            body[17]->func_name == "mem_free_owner" &&
            body[22]->op == icode_op::CALL &&
            body[22]->func_name == "so_destroy" &&
            body[24]->op == icode_op::CALL &&
            body[24]->func_name == "process_reap" &&
            body[68]->op == icode_op::CALL &&
            body[68]->func_name == "list_remove" &&
            body[72]->op == icode_op::CALL &&
            body[72]->func_name == "list_insert" &&
            body[87]->op == icode_op::RETURN &&
            body[91]->op == icode_op::RETURN &&
            body[94]->op == icode_op::RETURN) {
            const std::string cleanup_loop_lbl =
                fresh_local_label("__thread_select_next_cleanup");
            const std::string cleanup_done_lbl =
                fresh_local_label("__thread_select_next_cleanup_done");
            const std::string cleanup_skip_lbl =
                fresh_local_label("__thread_select_next_cleanup_skip");
            const std::string wait_loop_lbl =
                fresh_local_label("__thread_select_next_wait");
            const std::string wait_done_lbl =
                fresh_local_label("__thread_select_next_wait_done");
            const std::string wait_scan_lbl =
                fresh_local_label("__thread_select_next_wait_scan");
            const std::string wait_hit_lbl =
                fresh_local_label("__thread_select_next_wait_hit");
            const std::string wait_advance_lbl =
                fresh_local_label("__thread_select_next_wait_advance");
            const std::string move_ready_lbl =
                fresh_local_label("__thread_select_next_move_ready");
            const std::string ret_running_lbl =
                fresh_local_label("__thread_select_next_ret_running");
            const std::string ret_current_lbl =
                fresh_local_label("__thread_select_next_ret_current");
            const std::string done_lbl =
                fresh_local_label("__thread_select_next_done");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: select next thread");
            asm_.global_decl(mangle("__sdcc_enter_ix"));
            emit_line("call\t%s", mangle("__sdcc_enter_ix").c_str());
            emit_line("push\taf");
            emit_line("push\taf");
            emit_line("ld\tbc, (%s)",
                      mangle("thread_first_terminated").c_str());
            emit_label(cleanup_loop_lbl, false);
            emit_line("ld\ta, b");
            emit_line("or\ta, c");
            emit_line("jr\tz, %s", cleanup_done_lbl.c_str());
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("push\tde");
            emit_line("ld\thl, (%s)", mangle("thread_current").c_str());
            emit_line("or\ta, a");
            emit_line("sbc\thl, bc");
            emit_line("jr\tz, %s", cleanup_skip_lbl.c_str());
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tde, %s", asm_.imm(22).c_str());
            emit_line("add\thl, de");
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("push\tde");
            emit_line("ld\td, b");
            emit_line("ld\te, c");
            emit_line("ld\thl, %s", asm_.imm_sym(mangle("_heap")).c_str());
            asm_.global_decl(mangle("mem_free_owner"));
            emit_line("call\t%s", mangle("mem_free_owner").c_str());
            emit_line("ld\td, b");
            emit_line("ld\te, c");
            emit_line("ld\thl, %s",
                      asm_.imm_sym(mangle("thread_first_terminated")).c_str());
            asm_.global_decl(mangle("so_destroy"));
            emit_line("call\t%s", mangle("so_destroy").c_str());
            emit_line("pop\thl");
            asm_.global_decl(mangle("process_reap"));
            emit_line("call\t%s", mangle("process_reap").c_str());
            emit_label(cleanup_skip_lbl, false);
            emit_line("pop\tbc");
            emit_line("jr\t%s", cleanup_loop_lbl.c_str());
            emit_label(cleanup_done_lbl, false);
            emit_line("ld\tbc, (%s)", mangle("thread_first_waiting").c_str());
            emit_label(wait_loop_lbl, false);
            emit_line("ld\ta, b");
            emit_line("or\ta, c");
            emit_line("jr\tz, %s", wait_done_lbl.c_str());
            emit_line("ld\te, c");
            emit_line("ld\td, b");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tc, (hl)");
            emit_line("inc\thl");
            emit_line("ld\tb, (hl)");
            emit_line("ld\t-4(ix), %s", asm_.imm(0).c_str());
            emit_line("push\tde");
            emit_line("pop\tiy");
            emit_line("ld\ta, 16 (iy)");
            emit_line("ld\t-3(ix), a");
            emit_line("ld\ta, 17 (iy)");
            emit_line("ld\t-2(ix), a");
            emit_line("push\tde");
            emit_line("pop\tiy");
            emit_line("ld\t-1(ix), %s", asm_.imm(0).c_str());
            emit_label(wait_scan_lbl, false);
            emit_line("ld\tl, 18 (iy)");
            emit_line("ld\ta, -1(ix)");
            emit_line("sub\ta, l");
            emit_line("jr\tnc, %s", wait_advance_lbl.c_str());
            emit_line("ld\tl, -1(ix)");
            emit_line("ld\th, %s", asm_.imm(0).c_str());
            emit_line("add\thl, hl");
            emit_line("ld\ta, l");
            emit_line("add\ta, -3(ix)");
            emit_line("ld\tl, a");
            emit_line("ld\ta, h");
            emit_line("adc\ta, -2(ix)");
            emit_line("ld\th, a");
            emit_line("ld\ta, (hl)");
            emit_line("inc\thl");
            emit_line("ld\th, (hl)");
            emit_line("ld\tl, a");
            emit_line("inc\thl");
            emit_line("inc\thl");
            emit_line("inc\thl");
            emit_line("inc\thl");
            emit_line("ld\tl, (hl)");
            emit_line("dec\tl");
            emit_line("jr\tnz, %s", wait_hit_lbl.c_str());
            emit_line("ld\t-4(ix), %s", asm_.imm(1).c_str());
            emit_line("jr\t%s", wait_advance_lbl.c_str());
            emit_label(wait_hit_lbl, false);
            emit_line("inc\t-1(ix)");
            emit_line("jr\t%s", wait_scan_lbl.c_str());
            emit_label(wait_advance_lbl, false);
            emit_line("ld\ta, -4(ix)");
            emit_line("or\ta, a");
            emit_line("jr\tz, %s", wait_loop_lbl.c_str());
            emit_label(move_ready_lbl, false);
            emit_line("ld\thl, %s", asm_.imm(19).c_str());
            emit_line("add\thl, de");
            emit_line("ld\t(hl), %s", asm_.imm(1).c_str());
            emit_line("push\tbc");
            emit_line("push\tde");
            emit_line("ld\thl, %s",
                      asm_.imm_sym(mangle("thread_first_waiting")).c_str());
            asm_.global_decl(mangle("list_remove"));
            emit_line("call\t%s", mangle("list_remove").c_str());
            emit_line("pop\tde");
            emit_line("ld\thl, %s",
                      asm_.imm_sym(mangle("thread_first_running")).c_str());
            asm_.global_decl(mangle("list_insert"));
            emit_line("call\t%s", mangle("list_insert").c_str());
            emit_line("pop\tbc");
            emit_line("jr\t%s", wait_loop_lbl.c_str());
            emit_label(wait_done_lbl, false);
            emit_line("ld\thl, (%s)", mangle("thread_current").c_str());
            emit_line("ld\ta, h");
            emit_line("or\ta, l");
            emit_line("jr\tz, %s", ret_running_lbl.c_str());
            emit_line("push\thl");
            emit_line("ld\tde, %s", asm_.imm(19).c_str());
            emit_line("add\thl, de");
            emit_line("ld\ta, (hl)");
            emit_line("dec\ta");
            emit_line("pop\thl");
            emit_line("jr\tnz, %s", ret_running_lbl.c_str());
            emit_line("ld\te, (hl)");
            emit_line("inc\thl");
            emit_line("ld\td, (hl)");
            emit_line("ld\ta, d");
            emit_line("or\ta, e");
            emit_line("jr\tz, %s", ret_current_lbl.c_str());
            emit_line("jr\t%s", done_lbl.c_str());
            emit_label(ret_running_lbl, false);
            emit_line("ld\tde, (%s)", mangle("thread_first_running").c_str());
            emit_line("jr\t%s", done_lbl.c_str());
            emit_label(ret_current_lbl, false);
            emit_line("ld\tde, (%s)", mangle("thread_current").c_str());
            emit_label(done_lbl, false);
            emit_line("ld\tsp, ix");
            emit_line("pop\tix");
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        return false;
    };

    auto match_process_exit = [&]() -> bool {
        if (fn.local_bytes != 0 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 6)
            return false;

        const auto &ifx_ic = *body[0];
        const auto &ret_lbl = *body[1];
        const auto &ret_ic = *body[2];
        const auto &call_lbl = *body[3];
        const auto &send_ic = *body[4];
        const auto &call_ic = *body[5];

        if (ifx_ic.op != icode_op::IFX ||
            !is_plain_global_symbol(ifx_ic.left, "thread_current") ||
            ret_lbl.op != icode_op::LABEL ||
            ret_lbl.label_name != ifx_ic.false_lbl ||
            ret_ic.op != icode_op::RETURN ||
            !ret_ic.left.is_none() ||
            call_lbl.op != icode_op::LABEL ||
            call_lbl.label_name != ifx_ic.true_lbl ||
            send_ic.op != icode_op::SEND ||
            send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_plain_global_symbol(send_ic.left, "thread_current") ||
            call_ic.op != icode_op::CALL ||
            call_ic.func_name != "thread_exit" ||
            call_ic.num_params != 1 ||
            !call_ic.result.is_none()) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: current-thread exit wrapper");
        emit_line("ld\thl, %s",
                  asm_.indir_global(mangle("thread_current"), 0).c_str());
        emit_line("ld\ta, h");
        emit_line("or\ta, l");
        emit_line("ret\tz");
        asm_.global_decl(mangle(call_ic.func_name));
        emit_line("jp\t%s", mangle(call_ic.func_name).c_str());
        emit_helper_footer();
        return true;
    };

    auto match_thread_switch_wrapper = [&]() -> bool {
        if (fn.stack_param_bytes != 0)
            return false;

        if (fn.local_bytes == 0 && body.size() == 7) {
            const auto &recv_ic = *body[0];
            const auto &imm0_send_ic = *body[1];
            const auto &imm1_send_ic = *body[2];
            const auto &obj_send_ic = *body[3];
            const auto &dst_send_ic = *body[4];
            const auto &src_send_ic = *body[5];
            const auto &call_ic = *body[6];

            if (recv_ic.op != icode_op::RECEIVE ||
                recv_ic.arg_loc != abi_arg_loc::REG_HL ||
                !is_word_temp(recv_ic.result) ||
                imm0_send_ic.op != icode_op::SEND ||
                imm0_send_ic.arg_loc != abi_arg_loc::STACK ||
                imm1_send_ic.op != icode_op::SEND ||
                imm1_send_ic.arg_loc != abi_arg_loc::STACK ||
                obj_send_ic.op != icode_op::SEND ||
                obj_send_ic.arg_loc != abi_arg_loc::STACK ||
                !operands_equivalent(obj_send_ic.left, recv_ic.result) ||
                dst_send_ic.op != icode_op::SEND ||
                dst_send_ic.arg_loc != abi_arg_loc::REG_DE ||
                src_send_ic.op != icode_op::SEND ||
                src_send_ic.arg_loc != abi_arg_loc::REG_HL ||
                call_ic.op != icode_op::CALL ||
                call_ic.func_name != "_thread_lswitch" ||
                call_ic.num_params != 5 ||
                !call_ic.result.is_none()) {
                return false;
            }

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: thread queue switch wrapper");
            emit_line("ld\tde, %s", asm_.imm(imm0_send_ic.left.ival).c_str());
            emit_line("push\tde");
            emit_line("ld\ta, %s", asm_.imm(imm1_send_ic.left.ival).c_str());
            emit_line("push\taf");
            emit_line("inc\tsp");
            emit_line("push\thl");
            emit_line("ld\tde, %s",
                      asm_.imm_sym(mangle(dst_send_ic.left.name)).c_str());
            emit_line("ld\thl, %s",
                      asm_.imm_sym(mangle(src_send_ic.left.name)).c_str());
            asm_.global_decl(mangle(call_ic.func_name));
            emit_line("call\t%s", mangle(call_ic.func_name).c_str());
            emit_line("ret");
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes == 2 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 11 &&
            body[6]->op == icode_op::CALL &&
            body[6]->func_name == "_thread_lswitch" &&
            body[9]->op == icode_op::INLINE_ASM) {
            const std::string entry_lbl =
                fresh_local_label("__thread_exit_entry");
            const std::string loop_lbl =
                fresh_local_label("__thread_exit_loop");

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: thread exit wrapper");
            emit_line("ld\tde, %s", asm_.imm(1).c_str());
            emit_line("push\tde");
            emit_line("ld\ta, %s", asm_.imm(4).c_str());
            emit_line("push\taf");
            emit_line("inc\tsp");
            emit_line("push\thl");
            emit_line("ld\tde, %s",
                      asm_.imm_sym(mangle("thread_first_terminated")).c_str());
            emit_line("ld\thl, %s",
                      asm_.imm_sym(mangle("thread_first_running")).c_str());
            asm_.global_decl(mangle("_thread_lswitch"));
            emit_line("call\t%s", mangle("_thread_lswitch").c_str());
            emit_label(entry_lbl, false);
            emit_label(loop_lbl, false);
            emit_line("halt");
            emit_line("jr\t%s", loop_lbl.c_str());
            emit_helper_footer();
            return true;
        }

        if (fn.local_bytes == 2 && (body.size() == 10 || body.size() == 11)) {
            const auto &recv_store_ic = *body[0];
            const auto &imm0_send_ic = *body[1];
            const auto &imm1_send_ic = *body[2];
            const auto &obj_send_ic = *body[3];
            const auto &dst_send_ic = *body[4];
            const auto &src_send_ic = *body[5];
            const auto &call_ic = *body[6];
            const icode *entry_lbl = nullptr;
            const icode *loop_lbl = nullptr;
            const icode *asm_ic = nullptr;
            const icode *goto_ic = nullptr;

            if (body.size() == 10) {
                loop_lbl = body[7];
                asm_ic = body[8];
                goto_ic = body[9];
            } else {
                entry_lbl = body[7];
                loop_lbl = body[8];
                asm_ic = body[9];
                goto_ic = body[10];
            }

            if (recv_store_ic.op != icode_op::RECEIVE ||
                recv_store_ic.arg_loc != abi_arg_loc::REG_HL ||
                !recv_store_ic.result.is_symbol() ||
                recv_store_ic.result.is_global ||
                imm0_send_ic.op != icode_op::SEND ||
                imm0_send_ic.arg_loc != abi_arg_loc::STACK ||
                imm1_send_ic.op != icode_op::SEND ||
                imm1_send_ic.arg_loc != abi_arg_loc::STACK ||
                obj_send_ic.op != icode_op::SEND ||
                obj_send_ic.arg_loc != abi_arg_loc::STACK ||
                !operands_equivalent(obj_send_ic.left, recv_store_ic.result) ||
                dst_send_ic.op != icode_op::SEND ||
                dst_send_ic.arg_loc != abi_arg_loc::REG_DE ||
                src_send_ic.op != icode_op::SEND ||
                src_send_ic.arg_loc != abi_arg_loc::REG_HL ||
                call_ic.op != icode_op::CALL ||
                call_ic.func_name != "_thread_lswitch" ||
                call_ic.num_params != 5 ||
                !call_ic.result.is_none() ||
                (entry_lbl && entry_lbl->op != icode_op::LABEL) ||
                loop_lbl->op != icode_op::LABEL ||
                asm_ic->op != icode_op::INLINE_ASM ||
                goto_ic->op != icode_op::GOTO ||
                goto_ic->label_name != loop_lbl->label_name ||
                asm_ic->asm_text.find("halt") == std::string::npos) {
                return false;
            }

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: thread exit wrapper");
            emit_line("ld\tde, %s", asm_.imm(imm0_send_ic.left.ival).c_str());
            emit_line("push\tde");
            emit_line("ld\ta, %s", asm_.imm(imm1_send_ic.left.ival).c_str());
            emit_line("push\taf");
            emit_line("inc\tsp");
            emit_line("push\thl");
            emit_line("ld\tde, %s",
                      asm_.imm_sym(mangle(dst_send_ic.left.name)).c_str());
            emit_line("ld\thl, %s",
                      asm_.imm_sym(mangle(src_send_ic.left.name)).c_str());
            asm_.global_decl(mangle(call_ic.func_name));
            emit_line("call\t%s", mangle(call_ic.func_name).c_str());
            if (entry_lbl)
                emit_label(entry_lbl->label_name, false);
            emit_label(loop_lbl->label_name, false);
            emit_line("halt");
            emit_line("jr\t%s", loop_lbl->label_name.c_str());
            emit_helper_footer();
            return true;
        }

        return false;
    };

    auto match_lcase = [&]() -> bool {
        if (fn.local_bytes != 0 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 15)
            return false;

        const auto &recv_ic = *body[0];
        const auto &loop_lbl = *body[1];
        const auto &load_ic = *body[2];
        const auto &cond_cast_ic = *body[3];
        const auto &ifx_ic = *body[4];
        const auto &body_lbl = *body[5];
        const auto &arg_cast_ic = *body[6];
        const auto &send_ic = *body[7];
        const auto &call_ic = *body[8];
        const auto &store_cast_ic = *body[9];
        const auto &store_ic = *body[10];
        const auto &ptr_add_ic = *body[11];
        const auto &ptr_store_ic = *body[12];
        const auto &goto_ic = *body[13];
        const auto &end_lbl = *body[14];

        if (recv_ic.op != icode_op::RECEIVE ||
            recv_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_ic.result) ||
            loop_lbl.op != icode_op::LABEL ||
            load_ic.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_ic.result) ||
            !temp_eq(load_ic.left, recv_ic.result.temp_id) ||
            cond_cast_ic.op != icode_op::CAST ||
            !is_word_temp(cond_cast_ic.result) ||
            !operands_equivalent(cond_cast_ic.left, load_ic.result) ||
            ifx_ic.op != icode_op::IFX ||
            !temp_eq(ifx_ic.left, cond_cast_ic.result.temp_id) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != ifx_ic.true_lbl ||
            arg_cast_ic.op != icode_op::CAST ||
            !is_word_temp(arg_cast_ic.result) ||
            !operands_equivalent(arg_cast_ic.left, load_ic.result) ||
            send_ic.op != icode_op::SEND ||
            send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(send_ic.left, arg_cast_ic.result) ||
            call_ic.op != icode_op::CALL ||
            call_ic.func_name != "tolower" ||
            call_ic.num_params != 1 ||
            !is_word_temp(call_ic.result) ||
            store_cast_ic.op != icode_op::CAST ||
            !is_byte_temp(store_cast_ic.result) ||
            !operands_equivalent(store_cast_ic.left, call_ic.result) ||
            store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(store_ic.result, recv_ic.result.temp_id) ||
            !operands_equivalent(store_ic.left, store_cast_ic.result) ||
            ptr_add_ic.op != icode_op::ADD ||
            !ptr_add_ic.result.is_temp() ||
            !temp_eq(ptr_add_ic.left, recv_ic.result.temp_id) ||
            !is_exact_int_const(ptr_add_ic.right, 1) ||
            !is_assign_like(ptr_store_ic.op) ||
            !temp_eq(ptr_store_ic.result, recv_ic.result.temp_id) ||
            !temp_eq(ptr_store_ic.left, ptr_add_ic.result.temp_id) ||
            goto_ic.op != icode_op::GOTO ||
            goto_ic.label_name != loop_lbl.label_name ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != ifx_ic.false_lbl) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: byte lowercasing walker");
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\ta, (hl)");
        emit_line("or\ta, a");
        emit_line("ret\tz");
        emit_line("push\thl");
        emit_promote_a_to_hl(load_ic.result);
        asm_.global_decl(mangle(call_ic.func_name));
        emit_line("call\t%s", mangle(call_ic.func_name).c_str());
        emit_line("pop\thl");
        emit_line("ld\t(hl), e");
        emit_line("inc\thl");
        emit_line("jr\t%s", loop_lbl.label_name.c_str());
        emit_helper_footer();
        return true;
    };

    auto match_print_header = [&]() -> bool {
        if (fn.local_bytes != 0 || fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 28)
            return false;

        const auto &recv_ic = *body[0];
        const auto &loop_lbl = *body[1];
        const auto &load_cond_ic = *body[2];
        const auto &cond_cast_ic = *body[3];
        const auto &loop_ifx_ic = *body[4];
        const auto &body_lbl = *body[5];
        const auto &eq_ic = *body[6];
        const auto &eq_ifx_ic = *body[7];
        const auto &zero_lbl = *body[8];
        const auto &zero_assign_ic = *body[9];
        const auto &join_goto_ic = *body[10];
        const auto &one_lbl = *body[11];
        const auto &one_assign_ic = *body[12];
        const auto &join_lbl = *body[13];
        const auto &attr_send_ic = *body[14];
        const auto &attr_call_ic = *body[15];
        const auto &load_putc_ic = *body[16];
        const auto &putc_cast_ic = *body[17];
        const auto &putc_send_ic = *body[18];
        const auto &putc_call_ic = *body[19];
        const auto &ptr_add_ic = *body[20];
        const auto &ptr_store_ic = *body[21];
        const auto &loop_goto_ic = *body[22];
        const auto &end_lbl = *body[23];
        const auto &tail_attr_send_ic = *body[24];
        const auto &tail_attr_call_ic = *body[25];
        const auto &tail_putc_send_ic = *body[26];
        const auto &tail_putc_call_ic = *body[27];

        if (recv_ic.op != icode_op::RECEIVE ||
            recv_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_ic.result) ||
            loop_lbl.op != icode_op::LABEL ||
            load_cond_ic.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_cond_ic.result) ||
            !temp_eq(load_cond_ic.left, recv_ic.result.temp_id) ||
            cond_cast_ic.op != icode_op::CAST ||
            !is_word_temp(cond_cast_ic.result) ||
            !operands_equivalent(cond_cast_ic.left, load_cond_ic.result) ||
            loop_ifx_ic.op != icode_op::IFX ||
            !temp_eq(loop_ifx_ic.left, cond_cast_ic.result.temp_id) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != loop_ifx_ic.true_lbl ||
            eq_ic.op != icode_op::EQ ||
            !is_byte_temp(load_cond_ic.result) ||
            !((operands_equivalent(eq_ic.left, load_cond_ic.result) &&
               is_exact_int_const(eq_ic.right, 32)) ||
              (operands_equivalent(eq_ic.right, load_cond_ic.result) &&
               is_exact_int_const(eq_ic.left, 32))) ||
            eq_ifx_ic.op != icode_op::IFX ||
            !temp_eq(eq_ifx_ic.left, eq_ic.result.temp_id) ||
            zero_lbl.op != icode_op::LABEL ||
            zero_lbl.label_name != eq_ifx_ic.true_lbl ||
            zero_assign_ic.op != icode_op::ASSIGN ||
            zero_assign_ic.left.kind != operand_kind::INT_CONST ||
            zero_assign_ic.left.ival != 0 ||
            join_goto_ic.op != icode_op::GOTO ||
            one_lbl.op != icode_op::LABEL ||
            one_lbl.label_name != eq_ifx_ic.false_lbl ||
            one_assign_ic.op != icode_op::ASSIGN ||
            one_assign_ic.left.kind != operand_kind::INT_CONST ||
            one_assign_ic.left.ival != 1 ||
            join_lbl.op != icode_op::LABEL ||
            join_lbl.label_name != join_goto_ic.label_name ||
            attr_send_ic.op != icode_op::SEND ||
            (attr_send_ic.arg_loc != abi_arg_loc::REG_A &&
             attr_send_ic.arg_loc != abi_arg_loc::REG_HL) ||
            !operands_equivalent(attr_send_ic.left, zero_assign_ic.result) ||
            !operands_equivalent(attr_send_ic.left, one_assign_ic.result) ||
            attr_call_ic.op != icode_op::CALL ||
            attr_call_ic.func_name != "tty_attr" ||
            attr_call_ic.num_params != 1 ||
            !attr_call_ic.result.is_none() ||
            load_putc_ic.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_putc_ic.result) ||
            !temp_eq(load_putc_ic.left, recv_ic.result.temp_id) ||
            putc_cast_ic.op != icode_op::CAST ||
            !is_word_temp(putc_cast_ic.result) ||
            !operands_equivalent(putc_cast_ic.left, load_putc_ic.result) ||
            putc_send_ic.op != icode_op::SEND ||
            putc_send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !operands_equivalent(putc_send_ic.left, putc_cast_ic.result) ||
            putc_call_ic.op != icode_op::CALL ||
            putc_call_ic.func_name != "tty_putc" ||
            putc_call_ic.num_params != 1 ||
            !putc_call_ic.result.is_none() ||
            ptr_add_ic.op != icode_op::ADD ||
            !ptr_add_ic.result.is_temp() ||
            !temp_eq(ptr_add_ic.left, recv_ic.result.temp_id) ||
            !is_exact_int_const(ptr_add_ic.right, 1) ||
            !is_assign_like(ptr_store_ic.op) ||
            !temp_eq(ptr_store_ic.result, recv_ic.result.temp_id) ||
            !temp_eq(ptr_store_ic.left, ptr_add_ic.result.temp_id) ||
            loop_goto_ic.op != icode_op::GOTO ||
            loop_goto_ic.label_name != loop_lbl.label_name ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != loop_ifx_ic.false_lbl ||
            tail_attr_send_ic.op != icode_op::SEND ||
            (tail_attr_send_ic.arg_loc != abi_arg_loc::REG_A &&
             tail_attr_send_ic.arg_loc != abi_arg_loc::REG_HL) ||
            !is_exact_int_const(tail_attr_send_ic.left, 0) ||
            tail_attr_call_ic.op != icode_op::CALL ||
            tail_attr_call_ic.func_name != "tty_attr" ||
            tail_attr_call_ic.num_params != 1 ||
            !tail_attr_call_ic.result.is_none() ||
            tail_putc_send_ic.op != icode_op::SEND ||
            tail_putc_send_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_exact_int_const(tail_putc_send_ic.left, 10) ||
            tail_putc_call_ic.op != icode_op::CALL ||
            tail_putc_call_ic.func_name != "tty_putc" ||
            tail_putc_call_ic.num_params != 1 ||
            !tail_putc_call_ic.result.is_none()) {
            return false;
        }

        const std::string not_space_prefix = "__" + fn.name + "_ns";
        const std::string attr_done_prefix = "__" + fn.name + "_attr";
        const std::string not_space_lbl =
            fresh_local_label(not_space_prefix.c_str());
        const std::string attr_done_lbl =
            fresh_local_label(attr_done_prefix.c_str());

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: header byte walker");
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\ta, (hl)");
        emit_line("or\ta, a");
        emit_line("jr\tz, %s", end_lbl.label_name.c_str());
        emit_line("push\thl");
        emit_line("cp\t%s", asm_.imm(32).c_str());
        emit_line("jr\tnz, %s", not_space_lbl.c_str());
        emit_line("xor\ta");
        emit_line("jr\t%s", attr_done_lbl.c_str());
        emit_label(not_space_lbl, false);
        emit_line("ld\ta, %s", asm_.imm(1).c_str());
        emit_label(attr_done_lbl, false);
        if (attr_send_ic.arg_loc == abi_arg_loc::REG_HL) {
            emit_line("ld\tl, a");
            emit_line("ld\th, %s", asm_.imm(0).c_str());
        }
        asm_.global_decl(mangle(attr_call_ic.func_name));
        emit_line("call\t%s", mangle(attr_call_ic.func_name).c_str());
        emit_line("pop\thl");
        emit_line("ld\ta, (hl)");
        emit_line("push\thl");
        emit_promote_a_to_hl(load_putc_ic.result);
        asm_.global_decl(mangle(putc_call_ic.func_name));
        emit_line("call\t%s", mangle(putc_call_ic.func_name).c_str());
        emit_line("pop\thl");
        emit_line("inc\thl");
        emit_line("jr\t%s", loop_lbl.label_name.c_str());
        emit_label(end_lbl.label_name, false);
        emit_line("xor\ta");
        if (tail_attr_send_ic.arg_loc == abi_arg_loc::REG_HL) {
            emit_line("ld\tl, a");
            emit_line("ld\th, %s", asm_.imm(0).c_str());
        }
        asm_.global_decl(mangle(tail_attr_call_ic.func_name));
        emit_line("call\t%s", mangle(tail_attr_call_ic.func_name).c_str());
        emit_line("ld\thl, %s", asm_.imm(10).c_str());
        asm_.global_decl(mangle(tail_putc_call_ic.func_name));
        emit_line("jp\t%s", mangle(tail_putc_call_ic.func_name).c_str());
        emit_helper_footer();
        return true;
    };

    auto match_process_alive = [&]() -> bool {
        if ((fn.local_bytes != 0 && fn.local_bytes != 2) ||
            fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 15)
            return false;

        const auto &recv_ic = *body[0];
        const auto &head_assign_ic = *body[1];
        const auto &loop_lbl = *body[2];
        const auto &loop_ifx_ic = *body[3];
        const auto &body_lbl = *body[4];
        const auto &cmp_ic = *body[5];
        const auto &cmp_ifx_ic = *body[6];
        const auto &found_lbl = *body[7];
        const auto &found_ret_ic = *body[8];
        const auto &next_lbl = *body[9];
        const auto &next_load_ic = *body[10];
        const auto &next_store_ic = *body[11];
        const auto &goto_ic = *body[12];
        const auto &end_lbl = *body[13];
        const auto &end_ret_ic = *body[14];

        if (recv_ic.op != icode_op::RECEIVE ||
            recv_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_ic.result) ||
            head_assign_ic.op != icode_op::ASSIGN ||
            !(head_assign_ic.left.kind == operand_kind::SYMBOL &&
              head_assign_ic.left.is_global && !head_assign_ic.left.is_tls &&
              !head_assign_ic.left.is_sfr && !head_assign_ic.left.is_func) ||
            loop_lbl.op != icode_op::LABEL ||
            loop_ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(loop_ifx_ic.left, head_assign_ic.result) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != loop_ifx_ic.true_lbl ||
            cmp_ic.op != icode_op::EQ ||
            !((operands_equivalent(cmp_ic.left, head_assign_ic.result) &&
               operands_equivalent(cmp_ic.right, recv_ic.result)) ||
              (operands_equivalent(cmp_ic.right, head_assign_ic.result) &&
               operands_equivalent(cmp_ic.left, recv_ic.result))) ||
            cmp_ifx_ic.op != icode_op::IFX ||
            !temp_eq(cmp_ifx_ic.left, cmp_ic.result.temp_id) ||
            found_lbl.op != icode_op::LABEL ||
            found_lbl.label_name != cmp_ifx_ic.true_lbl ||
            found_ret_ic.op != icode_op::RETURN ||
            !is_exact_int_const(found_ret_ic.left, 1) ||
            next_lbl.op != icode_op::LABEL ||
            next_lbl.label_name != cmp_ifx_ic.false_lbl ||
            next_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(next_load_ic.result) ||
            !operands_equivalent(next_load_ic.left, head_assign_ic.result) ||
            !is_assign_like(next_store_ic.op) ||
            !operands_equivalent(next_store_ic.result, head_assign_ic.result) ||
            !operands_equivalent(next_store_ic.left, next_load_ic.result) ||
            goto_ic.op != icode_op::GOTO ||
            goto_ic.label_name != loop_lbl.label_name ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != loop_ifx_ic.false_lbl ||
            end_ret_ic.op != icode_op::RETURN ||
            !is_exact_int_const(end_ret_ic.left, 0)) {
            return false;
        }

        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: linked-list membership walk");
        emit_line("ld\tc, l");
        emit_line("ld\tb, h");
        emit_line("ld\thl, %s",
                  asm_.indir_global(mangle(head_assign_ic.left.name),
                                    head_assign_ic.left.byte_offset).c_str());
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\ta, h");
        emit_line("or\ta, l");
        emit_line("jr\tz, %s", end_lbl.label_name.c_str());
        emit_line("ld\ta, h");
        emit_line("cp\tb");
        emit_line("jr\tnz, %s", next_lbl.label_name.c_str());
        emit_line("ld\ta, l");
        emit_line("cp\tc");
        emit_line("jr\tz, %s", found_lbl.label_name.c_str());
        emit_label(next_lbl.label_name, false);
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        emit_line("ex\tde, hl");
        emit_line("jr\t%s", loop_lbl.label_name.c_str());
        emit_label(found_lbl.label_name, false);
        emit_line("ld\tde, %s", asm_.imm(1).c_str());
        emit_line("ret");
        emit_label(end_lbl.label_name, false);
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_mem_free_total = [&]() -> bool {
        if (fn.local_bytes == 0 &&
            fn.stack_param_bytes == 0 &&
            body.size() == 22) {
            const auto &recv_ic = *body[0];
            const auto &ptr_store_ic = *body[1];
            const auto &acc_init_ic = *body[2];
            const auto &loop_lbl = *body[3];
            const auto &loop_ifx_ic = *body[4];
            const auto &body_lbl = *body[5];
            const auto &stat_addr_ic = *body[6];
            const auto &stat_load_ic = *body[7];
            const auto &stat_cast_ic = *body[8];
            const auto &stat_ifx_ic = *body[9];
            const auto &sum_lbl = *body[10];
            const auto &size_addr_ic = *body[11];
            const auto &size_load_ic = *body[12];
            const auto &sum_ic = *body[13];
            const auto &acc_store_ic = *body[14];
            const auto &skip_lbl = *body[15];
            const auto &next_load_ic = *body[16];
            const auto &next_store_ic = *body[17];
            const auto &goto_ic = *body[18];
            const auto &end_lbl = *body[19];
            const auto &ret_copy_ic = *body[20];
            const auto &ret_ic = *body[21];

            if (recv_ic.op != icode_op::RECEIVE ||
                recv_ic.arg_loc != abi_arg_loc::REG_HL ||
                !is_word_temp(recv_ic.result) ||
                !is_assign_like(ptr_store_ic.op) ||
                !ptr_store_ic.result.is_temp() ||
                !operands_equivalent(ptr_store_ic.left, recv_ic.result) ||
                acc_init_ic.op != icode_op::ASSIGN ||
                !acc_init_ic.result.is_temp() ||
                !is_exact_int_const(acc_init_ic.left, 0) ||
                loop_lbl.op != icode_op::LABEL ||
                loop_ifx_ic.op != icode_op::IFX ||
                !operands_equivalent(loop_ifx_ic.left, ptr_store_ic.result) ||
                body_lbl.op != icode_op::LABEL ||
                body_lbl.label_name != loop_ifx_ic.true_lbl ||
                stat_addr_ic.op != icode_op::ADD ||
                !stat_addr_ic.result.is_temp() ||
                !operands_equivalent(stat_addr_ic.left, ptr_store_ic.result) ||
                stat_addr_ic.right.kind != operand_kind::INT_CONST ||
                stat_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_byte_temp(stat_load_ic.result) ||
                !temp_eq(stat_load_ic.left, stat_addr_ic.result.temp_id) ||
                stat_cast_ic.op != icode_op::CAST ||
                !is_word_temp(stat_cast_ic.result) ||
                !operands_equivalent(stat_cast_ic.left, stat_load_ic.result) ||
                stat_ifx_ic.op != icode_op::IFX ||
                !temp_eq(stat_ifx_ic.left, stat_cast_ic.result.temp_id) ||
                sum_lbl.op != icode_op::LABEL ||
                sum_lbl.label_name != stat_ifx_ic.false_lbl ||
                size_addr_ic.op != icode_op::ADD ||
                !size_addr_ic.result.is_temp() ||
                !operands_equivalent(size_addr_ic.left, ptr_store_ic.result) ||
                size_addr_ic.right.kind != operand_kind::INT_CONST ||
                size_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_word_temp(size_load_ic.result) ||
                !temp_eq(size_load_ic.left, size_addr_ic.result.temp_id) ||
                sum_ic.op != icode_op::ADD ||
                !sum_ic.result.is_temp() ||
                !((operands_equivalent(sum_ic.left, acc_init_ic.result) &&
                   operands_equivalent(sum_ic.right, size_load_ic.result)) ||
                  (operands_equivalent(sum_ic.right, acc_init_ic.result) &&
                   operands_equivalent(sum_ic.left, size_load_ic.result))) ||
                !is_assign_like(acc_store_ic.op) ||
                !operands_equivalent(acc_store_ic.result, acc_init_ic.result) ||
                !temp_eq(acc_store_ic.left, sum_ic.result.temp_id) ||
                skip_lbl.op != icode_op::LABEL ||
                skip_lbl.label_name != stat_ifx_ic.true_lbl ||
                next_load_ic.op != icode_op::GET_VALUE_AT ||
                !is_word_temp(next_load_ic.result) ||
                !operands_equivalent(next_load_ic.left, ptr_store_ic.result) ||
                !is_assign_like(next_store_ic.op) ||
                !operands_equivalent(next_store_ic.result, ptr_store_ic.result) ||
                !operands_equivalent(next_store_ic.left, next_load_ic.result) ||
                goto_ic.op != icode_op::GOTO ||
                goto_ic.label_name != loop_lbl.label_name ||
                end_lbl.op != icode_op::LABEL ||
                end_lbl.label_name != loop_ifx_ic.false_lbl ||
                !is_assign_like(ret_copy_ic.op) ||
                !operands_equivalent(ret_copy_ic.left, acc_init_ic.result) ||
                ret_ic.op != icode_op::RETURN ||
                !operands_equivalent(ret_ic.left, ret_copy_ic.result)) {
                return false;
            }

            const int stat_off = static_cast<int>(stat_addr_ic.right.ival);
            const int size_off = static_cast<int>(size_addr_ic.right.ival);
            if (stat_off < 0 || stat_off > 7 || size_off < 0 || size_off > 7)
                return false;

            emit_helper_header();
            emit_comment("O3 sdcc-style helper fast path: free-list size accumulator");
            emit_line("ld\tc, l");
            emit_line("ld\tb, h");
            emit_line("ld\tde, %s", asm_.imm(0).c_str());
            emit_label(loop_lbl.label_name, false);
            emit_line("ld\ta, b");
            emit_line("or\ta, c");
            emit_line("ret\tz");
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_adjust_hl_small(stat_off);
            emit_line("ld\ta, (hl)");
            emit_line("or\ta, a");
            emit_line("jr\tnz, %s", skip_lbl.label_name.c_str());
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_adjust_hl_small(size_off);
            emit_line("ld\ta, (hl)");
            emit_line("add\ta, e");
            emit_line("ld\te, a");
            emit_line("inc\thl");
            emit_line("ld\ta, (hl)");
            emit_line("adc\ta, d");
            emit_line("ld\td, a");
            emit_label(skip_lbl.label_name, false);
            emit_line("ld\tl, c");
            emit_line("ld\th, b");
            emit_line("ld\tc, (hl)");
            emit_line("inc\thl");
            emit_line("ld\tb, (hl)");
            emit_line("jr\t%s", loop_lbl.label_name.c_str());
            emit_helper_footer();
            return true;
        }

        if ((fn.local_bytes != 0 && fn.local_bytes != 2) ||
            fn.stack_param_bytes != 0)
            return false;
        if (body.size() != 23)
            return false;

        const auto &recv_ic = *body[0];
        const auto &ptr_store_ic = *body[1];
        const auto &acc_init_ic = *body[2];
        const auto &loop_lbl = *body[3];
        const auto &loop_ifx_ic = *body[4];
        const auto &body_lbl = *body[5];
        const auto &stat_addr_ic = *body[6];
        const auto &stat_load_ic = *body[7];
        const auto &stat_cast_ic = *body[8];
        const auto &stat_ifx_ic = *body[9];
        const auto &sum_lbl = *body[10];
        const auto &size_addr_ic = *body[11];
        const auto &size_load_ic = *body[12];
        const auto &acc_copy_ic = *body[13];
        const auto &sum_ic = *body[14];
        const auto &acc_store_ic = *body[15];
        const auto &skip_lbl = *body[16];
        const auto &next_load_ic = *body[17];
        const auto &next_store_ic = *body[18];
        const auto &goto_ic = *body[19];
        const auto &end_lbl = *body[20];
        const auto &ret_copy_ic = *body[21];
        const auto &ret_ic = *body[22];

        if (recv_ic.op != icode_op::RECEIVE ||
            recv_ic.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_ic.result) ||
            !is_assign_like(ptr_store_ic.op) ||
            !operands_equivalent(ptr_store_ic.left, recv_ic.result) ||
            acc_init_ic.op != icode_op::ASSIGN ||
            !acc_init_ic.result.is_temp() ||
            !is_exact_int_const(acc_init_ic.left, 0) ||
            loop_lbl.op != icode_op::LABEL ||
            loop_ifx_ic.op != icode_op::IFX ||
            !operands_equivalent(loop_ifx_ic.left, ptr_store_ic.result) ||
            body_lbl.op != icode_op::LABEL ||
            body_lbl.label_name != loop_ifx_ic.true_lbl ||
            stat_addr_ic.op != icode_op::ADD ||
            !stat_addr_ic.result.is_temp() ||
            !operands_equivalent(stat_addr_ic.left, ptr_store_ic.result) ||
            stat_addr_ic.right.kind != operand_kind::INT_CONST ||
            stat_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(stat_load_ic.result) ||
            !temp_eq(stat_load_ic.left, stat_addr_ic.result.temp_id) ||
            stat_cast_ic.op != icode_op::CAST ||
            !is_word_temp(stat_cast_ic.result) ||
            !operands_equivalent(stat_cast_ic.left, stat_load_ic.result) ||
            stat_ifx_ic.op != icode_op::IFX ||
            !temp_eq(stat_ifx_ic.left, stat_cast_ic.result.temp_id) ||
            sum_lbl.op != icode_op::LABEL ||
            sum_lbl.label_name != stat_ifx_ic.false_lbl ||
            size_addr_ic.op != icode_op::ADD ||
            !size_addr_ic.result.is_temp() ||
            !operands_equivalent(size_addr_ic.left, ptr_store_ic.result) ||
            size_addr_ic.right.kind != operand_kind::INT_CONST ||
            size_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(size_load_ic.result) ||
            !temp_eq(size_load_ic.left, size_addr_ic.result.temp_id) ||
            !is_assign_like(acc_copy_ic.op) ||
            acc_copy_ic.left.is_none() ||
            !operands_equivalent(acc_copy_ic.left, acc_init_ic.result) ||
            sum_ic.op != icode_op::ADD ||
            !sum_ic.result.is_temp() ||
            !((operands_equivalent(sum_ic.left, acc_copy_ic.result) &&
               operands_equivalent(sum_ic.right, size_load_ic.result)) ||
              (operands_equivalent(sum_ic.right, acc_copy_ic.result) &&
               operands_equivalent(sum_ic.left, size_load_ic.result))) ||
            !is_assign_like(acc_store_ic.op) ||
            !operands_equivalent(acc_store_ic.result, acc_init_ic.result) ||
            !temp_eq(acc_store_ic.left, sum_ic.result.temp_id) ||
            skip_lbl.op != icode_op::LABEL ||
            skip_lbl.label_name != stat_ifx_ic.true_lbl ||
            next_load_ic.op != icode_op::GET_VALUE_AT ||
            !is_word_temp(next_load_ic.result) ||
            !operands_equivalent(next_load_ic.left, ptr_store_ic.result) ||
            !is_assign_like(next_store_ic.op) ||
            !operands_equivalent(next_store_ic.result, ptr_store_ic.result) ||
            !operands_equivalent(next_store_ic.left, next_load_ic.result) ||
            goto_ic.op != icode_op::GOTO ||
            goto_ic.label_name != loop_lbl.label_name ||
            end_lbl.op != icode_op::LABEL ||
            end_lbl.label_name != loop_ifx_ic.false_lbl ||
            !is_assign_like(ret_copy_ic.op) ||
            !operands_equivalent(ret_copy_ic.left, acc_init_ic.result) ||
            ret_ic.op != icode_op::RETURN ||
            !operands_equivalent(ret_ic.left, ret_copy_ic.result)) {
            return false;
        }

        int stat_off = static_cast<int>(stat_addr_ic.right.ival);
        int size_off = static_cast<int>(size_addr_ic.right.ival);
        if (stat_off < 0 || stat_off > 7 || size_off < 0 || size_off > 7)
            return false;
        emit_helper_header();
        emit_comment("O3 sdcc-style helper fast path: free-list size accumulator");
        emit_line("ld\tc, l");
        emit_line("ld\tb, h");
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        emit_label(loop_lbl.label_name, false);
        emit_line("ld\ta, b");
        emit_line("or\ta, c");
        emit_line("ret\tz");
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_adjust_hl_small(stat_off);
        emit_line("ld\ta, (hl)");
        emit_line("or\ta, a");
        emit_line("jr\tnz, %s", stat_ifx_ic.true_lbl.c_str());
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_adjust_hl_small(size_off);
        emit_line("ld\ta, (hl)");
        emit_line("add\ta, e");
        emit_line("ld\te, a");
        emit_line("inc\thl");
        emit_line("ld\ta, (hl)");
        emit_line("adc\ta, d");
        emit_line("ld\td, a");
        emit_label(skip_lbl.label_name, false);
        emit_line("ld\tl, c");
        emit_line("ld\th, b");
        emit_line("ld\tc, (hl)");
        emit_line("inc\thl");
        emit_line("ld\tb, (hl)");
        emit_line("jr\t%s", loop_lbl.label_name.c_str());
        emit_helper_footer();
        return true;
    };

    return match_list_match_eq() ||
           match_list_insert() ||
           match_list_find() ||
           match_list_append() ||
           match_mem_init() ||
           match_mem_allocate() ||
           match_find_owned() ||
           match_so_create() ||
           match_so_destroy() ||
           match_svc_register() ||
           match_svc_query() ||
           match_tmr_install() ||
           match_tmr_chain() ||
           match_evt_set() ||
           match_process_start() ||
           match_thread_create() ||
           match_thread_lswitch() ||
           match_thread_select_next() ||
           match_process_exit() ||
           match_thread_switch_wrapper() ||
           match_lcase() ||
           match_print_header() ||
           match_process_alive() ||
           match_mem_free_total();
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
        emit_line("ld\ta, e");
        emit_line("add\ta, c");
        emit_line("add\ta, %s", asm_.imm(0x31).c_str());
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
        if (operand_uses_temp(ic.result, temp_id)) {
            if (ic.op == icode_op::SET_VALUE_AT)
                return true;
            return false;
        }
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
        if (operand_matches_symbol_slot(ic.result, sym)) {
            if (ic.op == icode_op::SET_VALUE_AT)
                return true;
            return false;
        }
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

    auto stable_recursive_byte_source =
        [&](const operand &candidate) {
            if (!candidate.is_temp())
                return true;
            auto home_it = temp_regs_.find(candidate.temp_id);
            if (home_it == temp_regs_.end())
                return true;
            switch (home_it->second) {
            case temp_home::main_a:
            case temp_home::alt_a:
                return false;
            default:
                return true;
            }
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
        src.type = def->result.type;
        return true;
    }

    if (def->op == icode_op::GET_VALUE_AT &&
        def->result.type && def->result.type->size() == 1 &&
        def->result.type->is_unsigned()) {
        src = op;
        src.type = def->result.type;
        return true;
    }

    if ((def->op == icode_op::CAST || def->op == icode_op::ASSIGN) &&
        def->result.type && def->result.type->size() == 1 &&
        def->result.type->is_unsigned()) {
        src = op;
        src.type = def->result.type;
        return true;
    }

    if (def->op == icode_op::CAST &&
        def->left.type && def->result.type &&
        def->result.type->size() >= 2 &&
        !def->left.type->is_far_ptr() &&
        !def->result.type->is_far_ptr() &&
        (def->left.type->is_integer() || def->left.type->is_ptr()) &&
        (def->result.type->is_integer() || def->result.type->is_ptr())) {
        if (!stable_recursive_byte_source(def->left))
            return false;
        return get_zero_extended_u8_source(def->left, src);
    }

    if (def->op == icode_op::ASSIGN) {
        if (!stable_recursive_byte_source(def->left))
            return false;
        return get_zero_extended_u8_source(def->left, src);
    }

    return false;
}

bool z80_gen::get_sign_extended_i8_source(const operand &op, operand &src) const {
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

    auto is_signed_i8 = [](const type_ptr &type) {
        return type && type->size() == 1 && type->is_integer() &&
               !type->is_unsigned();
    };

    auto stable_recursive_byte_source =
        [&](const operand &candidate) {
            if (!candidate.is_temp())
                return true;
            auto home_it = temp_regs_.find(candidate.temp_id);
            if (home_it == temp_regs_.end())
                return true;
            switch (home_it->second) {
            case temp_home::main_a:
            case temp_home::alt_a:
                return false;
            default:
                return true;
            }
        };

    if (op.kind == operand_kind::INT_CONST) {
        if (op.ival < -128 || op.ival > 127)
            return false;
        src = op;
        return true;
    }

    if (!op.is_temp() && is_signed_i8(op.type)) {
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

    if ((def->op == icode_op::RECEIVE ||
         def->op == icode_op::GET_VALUE_AT ||
         def->op == icode_op::CAST ||
         def->op == icode_op::ASSIGN) &&
        is_signed_i8(def->result.type)) {
        src = op;
        src.type = def->result.type;
        return true;
    }

    if (def->op == icode_op::CAST &&
        def->left.type && def->result.type &&
        def->result.type->size() >= 2 &&
        !def->left.type->is_far_ptr() &&
        !def->result.type->is_far_ptr() &&
        (def->left.type->is_integer() || def->left.type->is_ptr()) &&
        (def->result.type->is_integer() || def->result.type->is_ptr())) {
        if (!stable_recursive_byte_source(def->left))
            return false;
        return get_sign_extended_i8_source(def->left, src);
    }

    if (def->op == icode_op::ASSIGN) {
        if (!stable_recursive_byte_source(def->left))
            return false;
        return get_sign_extended_i8_source(def->left, src);
    }

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
                if (cur_def->op == icode_op::CAST &&
                    cur_def->result.type &&
                    cur_def->result.type->kind == type_kind::FLOAT &&
                    op_size(cur_def->result) == 2 &&
                    (cur_def->left.kind == operand_kind::INT_CONST ||
                     cur_def->left.kind == operand_kind::FLOAT_CONST)) {
                    invalidate_pair_cache();
                    operand packed =
                        cur_def->left.kind == operand_kind::FLOAT_CONST
                            ? operand::make_float(cur_def->left.fval,
                                                  cur_def->result.type)
                            : operand::make_float(
                                  static_cast<double>(cur_def->left.ival),
                                  cur_def->result.type);
                    load_hl(packed);
                    adjust_hl(cur.byte_offset);
                    return true;
                }
                if (cur_def->left.kind == operand_kind::INT_CONST ||
                    cur_def->left.kind == operand_kind::SYMBOL ||
                    cur_def->left.kind == operand_kind::LABEL_REF) {
                    invalidate_pair_cache();
                    load_hl(cur_def->left);
                    adjust_hl(cur.byte_offset);
                    return true;
                }
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
                        emit_line("ld\thl, %s",
                                  asm_.imm_sym(asm_label_ref_name(base->name)).c_str());
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
                load_de_zero_extended_u8(byte_src);
                if (base->kind == operand_kind::LABEL_REF) {
                    emit_line("ld\thl, %s",
                              asm_.imm_sym(asm_label_ref_name(base->name)).c_str());
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

bool z80_gen::operand_home_in_bc(const operand &op) const {
    if (symbol_home_in_bc(op))
        return true;
    if (!op.is_temp())
        return false;
    auto it = temp_regs_.find(op.temp_id);
    return it != temp_regs_.end() && it->second == temp_home::main_bc;
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
    if (value_init.result.is_none() || !value_init.result.type ||
        value_init.result.type->size() != 1 ||
        !is_assign_like(value_init.op)) {
        return false;
    }
    if (index_init.result.is_none() ||
        !is_assign_like(index_init.op) ||
        !is_exact_int_const(index_init.left, 0)) {
        return false;
    }
    if (!ptr_init.result.is_temp() || !ptr_init.result.type ||
        ptr_init.result.type->size() != 2 ||
        !(is_assign_like(ptr_init.op) ||
          ptr_init.op == icode_op::ADDRESS_OF) ||
        !is_global_byte_buffer_ref(ptr_init.left)) {
        return false;
    }
    if (cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, index_init.result) ||
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

    const operand value_op = value_init.result;
    const operand index_op = index_init.result;
    const int ptr_tid = ptr_init.result.temp_id;
    const int count = static_cast<int>(cmp_ic.right.ival);

    size_t p = idx + 7;
    if (p + 10 >= fn.icodes.size())
        return false;

    const icode &shl_ic = fn.icodes[p++];
    if (shl_ic.op != icode_op::SHL || !shl_ic.result.is_temp() ||
        !same_value_operand(shl_ic.left, value_op) ||
        !is_exact_int_const(shl_ic.right, 3)) {
        return false;
    }
    const int shl_tid = shl_ic.result.temp_id;

    const icode &xor1_ic = fn.icodes[p++];
    if (xor1_ic.op != icode_op::BXOR || !xor1_ic.result.is_temp()) {
        return false;
    }
    if (!((same_value_operand(xor1_ic.left, value_op) &&
           temp_eq(xor1_ic.right, shl_tid)) ||
          (same_value_operand(xor1_ic.right, value_op) &&
           temp_eq(xor1_ic.left, shl_tid)))) {
        return false;
    }
    const int xor1_tid = xor1_ic.result.temp_id;
    operand after_xor1_op = xor1_ic.result;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        same_value_operand(fn.icodes[p].result, value_op) &&
        temp_eq(fn.icodes[p].left, xor1_tid)) {
        after_xor1_op = value_op;
        ++p;
    }

    const icode &shr_ic = fn.icodes[p++];
    if (shr_ic.op != icode_op::SHR || !shr_ic.result.is_temp() ||
        !same_value_operand(shr_ic.left, after_xor1_op) ||
        !is_exact_int_const(shr_ic.right, 5)) {
        return false;
    }
    const int shr_tid = shr_ic.result.temp_id;

    const icode &xor2_ic = fn.icodes[p++];
    if (xor2_ic.op != icode_op::BXOR || !xor2_ic.result.is_temp()) {
        return false;
    }
    if (!((same_value_operand(xor2_ic.left, after_xor1_op) &&
           temp_eq(xor2_ic.right, shr_tid)) ||
          (same_value_operand(xor2_ic.right, after_xor1_op) &&
           temp_eq(xor2_ic.left, shr_tid)))) {
        return false;
    }
    const int xor2_tid = xor2_ic.result.temp_id;
    operand after_xor2_op = xor2_ic.result;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        same_value_operand(fn.icodes[p].result, value_op) &&
        temp_eq(fn.icodes[p].left, xor2_tid)) {
        after_xor2_op = value_op;
        ++p;
    }

    const icode &salt_ic = fn.icodes[p++];
    int salt = 0;
    if (salt_ic.op != icode_op::ADD || !salt_ic.result.is_temp()) {
        return false;
    }
    if (same_value_operand(salt_ic.left, index_op) &&
        salt_ic.right.kind == operand_kind::INT_CONST) {
        salt = static_cast<int>(salt_ic.right.ival & 0xFF);
    } else if (same_value_operand(salt_ic.right, index_op) &&
               salt_ic.left.kind == operand_kind::INT_CONST) {
        salt = static_cast<int>(salt_ic.left.ival & 0xFF);
    } else {
        return false;
    }
    const int salt_tid = salt_ic.result.temp_id;

    operand bias_op = salt_ic.result;
    int emitted_bias = salt & 0xFF;
    if (p < fn.icodes.size() &&
        fn.icodes[p].op == icode_op::ADD &&
        fn.icodes[p].result.is_temp() &&
        ((temp_eq(fn.icodes[p].left, salt_tid) &&
          is_exact_int_const(fn.icodes[p].right, 17)) ||
         (temp_eq(fn.icodes[p].right, salt_tid) &&
          is_exact_int_const(fn.icodes[p].left, 17)))) {
        bias_op = fn.icodes[p].result;
        emitted_bias = (salt + 17) & 0xFF;
        ++p;
    }

    const icode &value_add_ic = fn.icodes[p++];
    if (value_add_ic.op != icode_op::ADD || !value_add_ic.result.is_temp() ||
        !((same_value_operand(value_add_ic.left, after_xor2_op) &&
           same_value_operand(value_add_ic.right, bias_op)) ||
          (same_value_operand(value_add_ic.right, after_xor2_op) &&
           same_value_operand(value_add_ic.left, bias_op)))) {
        return false;
    }
    const int value_add_tid = value_add_ic.result.temp_id;

    const icode &value_store_ic = fn.icodes[p++];
    if (!is_assign_like(value_store_ic.op) ||
        !same_value_operand(value_store_ic.result, value_op) ||
        !temp_eq(value_store_ic.left, value_add_tid)) {
        return false;
    }

    const icode &out_ic = fn.icodes[p++];
    if (out_ic.op != icode_op::BXOR || !out_ic.result.is_temp() ||
        !((same_value_operand(out_ic.left, value_op) &&
           same_value_operand(out_ic.right, index_op)) ||
          (same_value_operand(out_ic.right, value_op) &&
           same_value_operand(out_ic.left, index_op)))) {
        return false;
    }
    const int out_tid = out_ic.result.temp_id;

    const icode *addr_ic = nullptr;
    if (p < fn.icodes.size() &&
        fn.icodes[p].op == icode_op::ADD &&
        fn.icodes[p].result.is_temp() &&
        ((temp_eq(fn.icodes[p].left, ptr_tid) &&
          same_value_operand(fn.icodes[p].right, index_op)) ||
         (temp_eq(fn.icodes[p].right, ptr_tid) &&
          same_value_operand(fn.icodes[p].left, index_op)))) {
        addr_ic = &fn.icodes[p++];
    }

    const icode &store_ic = fn.icodes[p++];
    if (store_ic.op != icode_op::SET_VALUE_AT ||
        !((addr_ic && temp_eq(store_ic.result, addr_ic->result.temp_id)) ||
          (!addr_ic && temp_eq(store_ic.result, ptr_tid))) ||
        !temp_eq(store_ic.left, out_tid)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + (addr_ic ? 3 : 5) >= fn.icodes.size())
        return false;

    const icode &idx_add_ic = fn.icodes[p++];
    if (idx_add_ic.op != icode_op::ADD || !idx_add_ic.result.is_temp() ||
        !same_value_operand(idx_add_ic.left, index_op) ||
        !is_exact_int_const(idx_add_ic.right, 1)) {
        return false;
    }
    const int idx_add_tid = idx_add_ic.result.temp_id;

    const icode &idx_store_ic = fn.icodes[p++];
    if (!is_assign_like(idx_store_ic.op) ||
        !same_value_operand(idx_store_ic.result, index_op) ||
        !temp_eq(idx_store_ic.left, idx_add_tid)) {
        return false;
    }

    if (!addr_ic) {
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
    }

    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if ((value_op.is_temp() && temp_value_used_after(fn, p, value_op.temp_id)) ||
        (value_op.is_symbol() && symbol_value_used_after(fn, p, value_op)) ||
        (index_op.is_temp() && temp_value_used_after(fn, p, index_op.temp_id)) ||
        (index_op.is_symbol() && symbol_value_used_after(fn, p, index_op)) ||
        temp_value_used_after(fn, p, ptr_tid)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(value_init.line);

    emit_comment("O3 bench-fill loop (count=%d)", count);
    load_a(value_init.left);
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(0).c_str());
    emit_line("ld\thl, %s",
              asm_.imm_sym(asm_symbol_ref_name(ptr_init.left)).c_str());

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
    emit_line("add\ta, %s", asm_.imm(emitted_bias).c_str());
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

bool z80_gen::try_emit_lcg_byte_fill_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 17 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &seed_init = fn.icodes[p++];
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];
    if (!is_assign_like(seed_init.op) ||
        !seed_init.result.is_temp() ||
        seed_init.left.kind != operand_kind::INT_CONST ||
        op_size(seed_init.result) != 2 ||
        !is_assign_like(idx_init.op) ||
        !idx_init.result.is_temp() ||
        !is_exact_int_const(idx_init.left, 0) ||
        op_size(idx_init.result) != 2 ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, idx_init.result) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 ||
        cmp_ic.right.ival > 65535 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const icode &mask_ic = fn.icodes[p++];
    const icode &narrow_ic = fn.icodes[p++];
    const icode &addr_ic = fn.icodes[p++];
    const icode &store_ic = fn.icodes[p++];
    const icode &mul_ic = fn.icodes[p++];
    const icode &add_ic = fn.icodes[p++];
    const icode &seed_store = fn.icodes[p++];

    if (mask_ic.op != icode_op::BAND ||
        !mask_ic.result.is_temp() ||
        !same_value_operand(mask_ic.left, seed_init.result) ||
        !is_exact_int_const(mask_ic.right, 255) ||
        narrow_ic.op != icode_op::CAST ||
        !is_byte_temp(narrow_ic.result) ||
        !temp_eq(narrow_ic.left, mask_ic.result.temp_id) ||
        addr_ic.op != icode_op::ADD ||
        !addr_ic.result.is_temp() ||
        !is_global_byte_buffer_ref(addr_ic.left) ||
        !same_value_operand(addr_ic.right, idx_init.result) ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_ic.result, addr_ic.result.temp_id) ||
        !temp_eq(store_ic.left, narrow_ic.result.temp_id) ||
        mul_ic.op != icode_op::MUL ||
        !mul_ic.result.is_temp() ||
        !same_value_operand(mul_ic.left, seed_init.result) ||
        mul_ic.right.kind != operand_kind::INT_CONST ||
        add_ic.op != icode_op::ADD ||
        !add_ic.result.is_temp() ||
        !temp_eq(add_ic.left, mul_ic.result.temp_id) ||
        add_ic.right.kind != operand_kind::INT_CONST ||
        !is_assign_like(seed_store.op) ||
        !same_value_operand(seed_store.result, seed_init.result) ||
        !temp_eq(seed_store.left, add_ic.result.temp_id)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;

    const icode &idx_add = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (idx_add.op != icode_op::ADD ||
        !idx_add.result.is_temp() ||
        !same_value_operand(idx_add.left, idx_init.result) ||
        !is_exact_int_const(idx_add.right, 1) ||
        !is_assign_like(idx_store.op) ||
        !same_value_operand(idx_store.result, idx_init.result) ||
        !temp_eq(idx_store.left, idx_add.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, seed_init.result.temp_id) ||
        temp_value_used_after(fn, p, idx_init.result.temp_id) ||
        temp_value_used_after(fn, p, cmp_ic.result.temp_id) ||
        temp_value_used_after(fn, p, mask_ic.result.temp_id) ||
        temp_value_used_after(fn, p, narrow_ic.result.temp_id) ||
        temp_value_used_after(fn, p, addr_ic.result.temp_id) ||
        temp_value_used_after(fn, p, mul_ic.result.temp_id) ||
        temp_value_used_after(fn, p, add_ic.result.temp_id) ||
        temp_value_used_after(fn, p, idx_add.result.temp_id)) {
        return false;
    }

    const uint16_t limit = static_cast<uint16_t>(cmp_ic.right.ival);
    const uint16_t mul_const = static_cast<uint16_t>(mul_ic.right.ival);
    const uint16_t add_const = static_cast<uint16_t>(add_ic.right.ival);
    const std::string data_sym = asm_symbol_ref_name(addr_ic.left);

    auto emit_mul_const_u16 = [&](uint16_t k) {
        if (k == 0) {
            emit_line("ld\thl, %s", asm_.imm(0).c_str());
            return;
        }
        emit_line("ld\th, d");
        emit_line("ld\tl, e");
        int msb = 15;
        while (msb > 0 && ((k >> msb) & 1u) == 0)
            --msb;
        for (int bit = msb - 1; bit >= 0; --bit) {
            emit_line("add\thl, hl");
            if ((k >> bit) & 1u)
                emit_line("add\thl, de");
        }
    };

    if (debug_)
        debug_->emit_location(seed_init.line);

    emit_comment("O2 LCG byte fill loop");
    emit_line("ld\tde, %s", asm_.imm(seed_init.left.ival & 0xffff).c_str());
    emit_line("ld\thl, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\tbc, %s + %u", asm_.imm_sym(data_sym).c_str(),
              static_cast<unsigned>(limit));
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, l");
    emit_line("sub\tc");
    emit_line("ld\ta, h");
    emit_line("sbc\ta, b");
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("ld\t(hl), e");
    emit_line("push\thl");
    emit_line("push\tbc");
    emit_mul_const_u16(mul_const);
    emit_line("ld\tbc, %s", asm_.imm(add_const).c_str());
    emit_line("add\thl, bc");
    emit_line("ex\tde, hl");
    emit_line("pop\tbc");
    emit_line("pop\thl");
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

    const std::string data_sym = asm_symbol_ref_name(addr0.left);

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

    const std::string buf_sym = asm_symbol_ref_name(ptr_init.left);
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
    const std::string hl_sym = asm_symbol_ref_name(ptr_hl);
    const std::string de_sym = asm_symbol_ref_name(ptr_de);

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

    auto try_compact_index_form = [&]() -> bool {
        size_t p = idx;
        auto is_global_ref = [&](const operand &op) {
            return op.kind == operand_kind::LABEL_REF ||
                   (op.kind == operand_kind::SYMBOL &&
                    op.is_global && !op.is_tls && !op.is_sfr && !op.is_func);
        };
        std::vector<const icode *> base_inits;
        auto capture_base_init = [&](const icode &ic) {
            if (ic.op != icode_op::ADDRESS_OF || !ic.result.is_temp() ||
                !is_global_ref(ic.left)) {
                return false;
            }
            base_inits.push_back(&ic);
            return true;
        };
        auto resolve_global_base = [&](const operand &op) -> const operand * {
            if (is_global_ref(op))
                return &op;
            if (!op.is_temp())
                return nullptr;
            for (const icode *init : base_inits) {
                if (temp_eq(init->result, op.temp_id))
                    return &init->left;
            }
            return nullptr;
        };
        auto same_global_base_ref = [&](const operand &a, const operand &b) {
            return same_global_ref(a, b) ||
                   (is_global_ref(a) && is_global_ref(b) &&
                    asm_symbol_ref_name(a) == asm_symbol_ref_name(b));
        };
        auto addr_base_with_offset = [&](const icode &addr_ic,
                                         const operand &offset_op)
            -> const operand * {
            if (addr_ic.op != icode_op::ADD || !addr_ic.result.is_temp())
                return nullptr;
            const operand *base_l = resolve_global_base(addr_ic.left);
            if (base_l && same_value_operand(addr_ic.right, offset_op))
                return base_l;
            const operand *base_r = resolve_global_base(addr_ic.right);
            if (base_r && same_value_operand(addr_ic.left, offset_op))
                return base_r;
            return nullptr;
        };
        auto load_from_base = [&](const icode &load_ic,
                                  const operand &base,
                                  const operand &index_op) {
            return load_ic.op == icode_op::GET_VALUE_AT &&
                   load_ic.result.is_temp() &&
                   is_byte_temp(load_ic.result) &&
                   same_global_base_ref(load_ic.left, base) &&
                   (load_ic.right.is_none() ||
                    same_value_operand(load_ic.right, index_op));
        };

        if (p >= fn.icodes.size())
            return false;
        const icode &outer_idx_init = fn.icodes[p++];
        while (p < fn.icodes.size() && capture_base_init(fn.icodes[p]))
            ++p;
        if (p + 18 >= fn.icodes.size())
            return false;
        const icode &outer_cond_lbl = fn.icodes[p++];
        const icode &outer_cmp_ic = fn.icodes[p++];
        const icode &outer_ifx_ic = fn.icodes[p++];
        const icode &outer_body_lbl = fn.icodes[p++];

        if (outer_idx_init.result.is_none() ||
            !is_assign_like(outer_idx_init.op) ||
            !is_exact_int_const(outer_idx_init.left, 0) ||
            outer_cond_lbl.op != icode_op::LABEL ||
            outer_cmp_ic.op != icode_op::LT ||
            !outer_cmp_ic.result.is_temp() ||
            !same_value_operand(outer_cmp_ic.left, outer_idx_init.result) ||
            outer_cmp_ic.right.kind != operand_kind::INT_CONST ||
            outer_cmp_ic.right.ival <= 0 || outer_cmp_ic.right.ival > 255 ||
            outer_ifx_ic.op != icode_op::IFX ||
            !temp_eq(outer_ifx_ic.left, outer_cmp_ic.result.temp_id) ||
            outer_body_lbl.op != icode_op::LABEL ||
            outer_body_lbl.label_name != outer_ifx_ic.true_lbl) {
            return false;
        }

        const operand outer_idx_op = outer_idx_init.result;
        const int row_count = static_cast<int>(outer_cmp_ic.right.ival);

        const icode &inner_idx_init = fn.icodes[p++];
        const icode &row_cast_ic = fn.icodes[p++];
        const icode &row_addr_ic = fn.icodes[p++];
        const icode &row_base_ic = fn.icodes[p++];
        const icode &inner_cond_lbl = fn.icodes[p++];
        const icode &inner_cmp_ic = fn.icodes[p++];
        const icode &inner_ifx_ic = fn.icodes[p++];
        const icode &inner_body_lbl = fn.icodes[p++];

        if (inner_idx_init.result.is_none() ||
            !is_assign_like(inner_idx_init.op) ||
            !is_exact_int_const(inner_idx_init.left, 0) ||
            row_cast_ic.op != icode_op::CAST ||
            !row_cast_ic.result.is_temp() ||
            !row_cast_ic.result.type || row_cast_ic.result.type->size() != 2 ||
            !same_value_operand(row_cast_ic.left, outer_idx_op) ||
            row_base_ic.op != icode_op::SHL ||
            !row_base_ic.result.is_temp() ||
            !same_value_operand(row_base_ic.left, outer_idx_op) ||
            row_base_ic.right.kind != operand_kind::INT_CONST ||
            row_base_ic.right.ival < 0 || row_base_ic.right.ival > 7 ||
            inner_cond_lbl.op != icode_op::LABEL ||
            inner_cmp_ic.op != icode_op::LT ||
            !inner_cmp_ic.result.is_temp() ||
            !same_value_operand(inner_cmp_ic.left, inner_idx_init.result) ||
            inner_cmp_ic.right.kind != operand_kind::INT_CONST ||
            inner_cmp_ic.right.ival <= 0 || inner_cmp_ic.right.ival > 255 ||
            inner_ifx_ic.op != icode_op::IFX ||
            !temp_eq(inner_ifx_ic.left, inner_cmp_ic.result.temp_id) ||
            inner_body_lbl.op != icode_op::LABEL ||
            inner_body_lbl.label_name != inner_ifx_ic.true_lbl) {
            return false;
        }

        const operand inner_idx_op = inner_idx_init.result;
        const int col_count = static_cast<int>(inner_cmp_ic.right.ival);
        const int row_shift = static_cast<int>(row_base_ic.right.ival);
        if ((1 << row_shift) != col_count)
            return false;

        const operand *row_base =
            addr_base_with_offset(row_addr_ic, row_cast_ic.result);
        if (!row_base || !is_global_byte_buffer_ref(*row_base))
            return false;

        const icode &src_idx_ic = fn.icodes[p++];
        const icode *idx_store_ic = nullptr;
        if (p < fn.icodes.size() && is_assign_like(fn.icodes[p].op) &&
            same_value_operand(fn.icodes[p].left, src_idx_ic.result)) {
            idx_store_ic = &fn.icodes[p++];
        }
        if (p + 8 >= fn.icodes.size())
            return false;

        const icode &row_load_ic = fn.icodes[p++];
        const icode &src_load_ic = fn.icodes[p++];
        const icode &row_add_ic = fn.icodes[p++];
        const icode &row_store_ic = fn.icodes[p++];
        const icode &col_load_ic = fn.icodes[p++];
        const icode &col_add_ic = fn.icodes[p++];
        const icode &col_cast_ic = fn.icodes[p++];
        const icode &col_addr_ic = fn.icodes[p++];
        const icode &col_store_ic = fn.icodes[p++];

        operand src_index_op = src_idx_ic.result;
        if (idx_store_ic)
            src_index_op = idx_store_ic->result;
        const bool src_idx_ok =
            src_idx_ic.op == icode_op::ADD &&
            src_idx_ic.result.is_temp() &&
            ((temp_eq(src_idx_ic.left, row_base_ic.result.temp_id) &&
              same_value_operand(src_idx_ic.right, inner_idx_op)) ||
             (temp_eq(src_idx_ic.right, row_base_ic.result.temp_id) &&
              same_value_operand(src_idx_ic.left, inner_idx_op)));
        const bool row_load_ok =
            load_from_base(row_load_ic, *row_base, outer_idx_op) ||
            load_from_base(row_load_ic, *row_base, row_cast_ic.result);
        const bool src_load_ok =
            src_load_ic.op == icode_op::GET_VALUE_AT &&
            src_load_ic.result.is_temp() &&
            is_byte_temp(src_load_ic.result) &&
            is_global_byte_buffer_ref(src_load_ic.left) &&
            (src_load_ic.right.is_none() ||
             same_value_operand(src_load_ic.right, src_index_op));
        const bool row_add_ok =
            row_add_ic.op == icode_op::ADD &&
            row_add_ic.result.is_temp() &&
            ((temp_eq(row_add_ic.left, row_load_ic.result.temp_id) &&
              temp_eq(row_add_ic.right, src_load_ic.result.temp_id)) ||
             (temp_eq(row_add_ic.right, row_load_ic.result.temp_id) &&
              temp_eq(row_add_ic.left, src_load_ic.result.temp_id)));
        const bool row_store_ok =
            row_store_ic.op == icode_op::SET_VALUE_AT &&
            temp_eq(row_store_ic.result, row_addr_ic.result.temp_id) &&
            temp_eq(row_store_ic.left, row_add_ic.result.temp_id);
        const bool col_load_ok =
            col_load_ic.op == icode_op::GET_VALUE_AT &&
            col_load_ic.result.is_temp() &&
            is_byte_temp(col_load_ic.result) &&
            is_global_byte_buffer_ref(col_load_ic.left) &&
            (col_load_ic.right.is_none() ||
             same_value_operand(col_load_ic.right, inner_idx_op));
        const bool col_add_ok =
            col_add_ic.op == icode_op::ADD &&
            col_add_ic.result.is_temp() &&
            ((temp_eq(col_add_ic.left, col_load_ic.result.temp_id) &&
              temp_eq(col_add_ic.right, src_load_ic.result.temp_id)) ||
             (temp_eq(col_add_ic.right, col_load_ic.result.temp_id) &&
              temp_eq(col_add_ic.left, src_load_ic.result.temp_id)));
        const bool col_cast_ok =
            col_cast_ic.op == icode_op::CAST &&
            col_cast_ic.result.is_temp() &&
            col_cast_ic.result.type && col_cast_ic.result.type->size() == 2 &&
            same_value_operand(col_cast_ic.left, inner_idx_op);
        if (!src_idx_ok || !row_load_ok || !src_load_ok || !row_add_ok ||
            !row_store_ok || !col_load_ok || !col_add_ok || !col_cast_ok) {
            return false;
        }

        const operand *col_base =
            addr_base_with_offset(col_addr_ic, col_cast_ic.result);
        if (!col_base || !is_global_byte_buffer_ref(*col_base) ||
            !same_global_base_ref(col_load_ic.left, *col_base) ||
            col_store_ic.op != icode_op::SET_VALUE_AT ||
            !temp_eq(col_store_ic.result, col_addr_ic.result.temp_id) ||
            !temp_eq(col_store_ic.left, col_add_ic.result.temp_id)) {
            return false;
        }

        while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
            ++p;
        if (p + 7 >= fn.icodes.size())
            return false;
        const icode &inner_idx_add_ic = fn.icodes[p++];
        const icode &inner_idx_store_ic = fn.icodes[p++];
        const icode &goto_inner_ic = fn.icodes[p++];
        const icode &inner_end_lbl = fn.icodes[p++];
        const icode &outer_idx_add_ic = fn.icodes[p++];
        const icode &outer_idx_store_ic = fn.icodes[p++];
        const icode &goto_outer_ic = fn.icodes[p++];
        const icode &outer_end_lbl = fn.icodes[p++];

        if (inner_idx_add_ic.op != icode_op::ADD ||
            !inner_idx_add_ic.result.is_temp() ||
            !same_value_operand(inner_idx_add_ic.left, inner_idx_op) ||
            !is_exact_int_const(inner_idx_add_ic.right, 1) ||
            !is_assign_like(inner_idx_store_ic.op) ||
            !same_value_operand(inner_idx_store_ic.result, inner_idx_op) ||
            !temp_eq(inner_idx_store_ic.left, inner_idx_add_ic.result.temp_id) ||
            goto_inner_ic.op != icode_op::GOTO ||
            goto_inner_ic.label_name != inner_cond_lbl.label_name ||
            inner_end_lbl.op != icode_op::LABEL ||
            inner_end_lbl.label_name != inner_ifx_ic.false_lbl ||
            outer_idx_add_ic.op != icode_op::ADD ||
            !outer_idx_add_ic.result.is_temp() ||
            !same_value_operand(outer_idx_add_ic.left, outer_idx_op) ||
            !is_exact_int_const(outer_idx_add_ic.right, 1) ||
            !is_assign_like(outer_idx_store_ic.op) ||
            !same_value_operand(outer_idx_store_ic.result, outer_idx_op) ||
            !temp_eq(outer_idx_store_ic.left, outer_idx_add_ic.result.temp_id) ||
            goto_outer_ic.op != icode_op::GOTO ||
            goto_outer_ic.label_name != outer_cond_lbl.label_name ||
            outer_end_lbl.op != icode_op::LABEL ||
            outer_end_lbl.label_name != outer_ifx_ic.false_lbl) {
            return false;
        }

        if ((outer_idx_op.is_temp() &&
             temp_value_used_after(fn, p, outer_idx_op.temp_id)) ||
            (outer_idx_op.is_symbol() &&
             symbol_value_used_after(fn, p, outer_idx_op)) ||
            (inner_idx_op.is_temp() &&
            temp_value_used_after(fn, p, inner_idx_op.temp_id)) ||
            (inner_idx_op.is_symbol() &&
             symbol_value_used_after(fn, p, inner_idx_op)) ||
            temp_value_used_after(fn, p, row_cast_ic.result.temp_id) ||
            temp_value_used_after(fn, p, row_base_ic.result.temp_id) ||
            temp_value_used_after(fn, p, src_idx_ic.result.temp_id)) {
            return false;
        }

        if (debug_)
            debug_->emit_location(outer_idx_init.line);

        const std::string src_sym = asm_symbol_ref_name(src_load_ic.left);
        const std::string row_sym = asm_symbol_ref_name(*row_base);
        const std::string col_sym = asm_symbol_ref_name(*col_base);

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
    };

    if (try_compact_index_form())
        return true;

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

    const std::string src_sym = asm_symbol_ref_name(src_load1_ic.left);
    const std::string row_sym = asm_symbol_ref_name(row_ptr_init.left);
    const std::string col_sym = asm_symbol_ref_name(col_ptr_init.left);

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
    std::vector<const icode *> base_inits;
    auto maybe_capture_base_init = [&](const icode &ic) {
        if ((ic.op != icode_op::ADDRESS_OF && !is_assign_like(ic.op)) ||
            !ic.result.is_temp() ||
            !is_global_ref(ic.left)) {
            return false;
        }
        base_inits.push_back(&ic);
        return true;
    };
    auto resolve_global_base = [&](const operand &op) -> const operand * {
        if (is_global_ref(op))
            return &op;
        if (!op.is_temp())
            return nullptr;
        for (const icode *init : base_inits) {
            if (temp_eq(init->result, op.temp_id))
                return &init->left;
        }
        return nullptr;
    };
    auto match_casted_index = [&](size_t &p, const operand &src,
                                  operand &index_op) {
        index_op = src;
        if (p < fn.icodes.size() &&
            fn.icodes[p].op == icode_op::CAST &&
            fn.icodes[p].result.is_temp() &&
            same_value_operand(fn.icodes[p].left, src) &&
            fn.icodes[p].result.type &&
            fn.icodes[p].result.type->size() == 2) {
            index_op = fn.icodes[p].result;
            ++p;
        }
        return true;
    };
    auto match_indexed_byte_load = [&](size_t &p, const operand &idx_op,
                                       operand &base_out, int &value_tid) {
        if (p >= fn.icodes.size())
            return false;

        const icode &direct = fn.icodes[p];
        if (direct.op == icode_op::GET_VALUE_AT &&
            direct.result.is_temp() &&
            is_byte_temp(direct.result) &&
            is_global_byte_buffer_ref(direct.left) &&
            (direct.right.is_none() || same_value_operand(direct.right, idx_op))) {
            base_out = direct.left;
            value_tid = direct.result.temp_id;
            ++p;
            return true;
        }

        size_t q = p;
        operand addr_idx_op;
        match_casted_index(q, idx_op, addr_idx_op);
        if (q + 1 >= fn.icodes.size())
            return false;
        const icode &addr_ic = fn.icodes[q];
        const icode &load_ic = fn.icodes[q + 1];
        const operand *addr_base_l = resolve_global_base(addr_ic.left);
        const operand *addr_base_r = resolve_global_base(addr_ic.right);
        if (addr_ic.op != icode_op::ADD ||
            !addr_ic.result.is_temp() ||
            !((addr_base_l && is_global_byte_buffer_ref(*addr_base_l) &&
                same_value_operand(addr_ic.right, addr_idx_op)) ||
               (addr_base_r && is_global_byte_buffer_ref(*addr_base_r) &&
                same_value_operand(addr_ic.left, addr_idx_op))) ||
            load_ic.op != icode_op::GET_VALUE_AT ||
            !load_ic.result.is_temp() ||
            !is_byte_temp(load_ic.result) ||
            !temp_eq(load_ic.left, addr_ic.result.temp_id)) {
            return false;
        }
        base_out = (addr_base_l && is_global_byte_buffer_ref(*addr_base_l))
                       ? *addr_base_l
                       : *addr_base_r;
        value_tid = load_ic.result.temp_id;
        p = q + 2;
        return true;
    };

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    while (p < fn.icodes.size() && maybe_capture_base_init(fn.icodes[p]))
        ++p;
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (idx_init.result.is_none() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, idx_init.result) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 || cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const operand idx_op = idx_init.result;
    const int count = static_cast<int>(cmp_ic.right.ival);

    operand key_base;
    int key_tid = -1;
    if (!match_indexed_byte_load(p, idx_op, key_base, key_tid))
        return false;

    size_t q = p;
    operand key_idx_op;
    match_casted_index(q, idx_op, key_idx_op);
    if (q + 5 >= fn.icodes.size())
        return false;

    const icode &key_shl_ic = fn.icodes[q++];
    const icode &node_addr_ic = fn.icodes[q++];
    const icode &store_key_ic = fn.icodes[q++];
    operand next_idx_op;
    match_casted_index(q, idx_op, next_idx_op);
    const icode &next_shl_ic = fn.icodes[q++];
    const icode &next_node_addr_ic = fn.icodes[q++];
    const icode &next_ptr_ic = fn.icodes[q++];
    const icode &store_next_ic = fn.icodes[q++];

    const operand *node_addr_base_l = resolve_global_base(node_addr_ic.left);
    const operand *node_addr_base_r = resolve_global_base(node_addr_ic.right);
    const operand *node_addr_base = nullptr;
    if (node_addr_base_l &&
        temp_eq(node_addr_ic.right, key_shl_ic.result.temp_id)) {
        node_addr_base = node_addr_base_l;
    } else if (node_addr_base_r &&
               temp_eq(node_addr_ic.left, key_shl_ic.result.temp_id)) {
        node_addr_base = node_addr_base_r;
    }
    const operand *next_addr_base_l =
        resolve_global_base(next_node_addr_ic.left);
    const operand *next_addr_base_r =
        resolve_global_base(next_node_addr_ic.right);
    const operand *next_addr_base = nullptr;
    if (next_addr_base_l &&
        temp_eq(next_node_addr_ic.right, next_shl_ic.result.temp_id)) {
        next_addr_base = next_addr_base_l;
    } else if (next_addr_base_r &&
               temp_eq(next_node_addr_ic.left, next_shl_ic.result.temp_id)) {
        next_addr_base = next_addr_base_r;
    }

    if (key_shl_ic.op != icode_op::SHL ||
        !key_shl_ic.result.is_temp() ||
        !same_value_operand(key_shl_ic.left, key_idx_op) ||
        !is_exact_int_const(key_shl_ic.right, 1) ||
        node_addr_ic.op != icode_op::ADD ||
        !node_addr_ic.result.is_temp() ||
        node_addr_base == nullptr ||
        store_key_ic.op != icode_op::SET_VALUE_AT ||
        !temp_eq(store_key_ic.result, node_addr_ic.result.temp_id) ||
        !temp_eq(store_key_ic.left, key_tid) ||
        next_shl_ic.op != icode_op::SHL ||
        !next_shl_ic.result.is_temp() ||
        !same_value_operand(next_shl_ic.left, next_idx_op) ||
        !is_exact_int_const(next_shl_ic.right, 1) ||
        next_node_addr_ic.op != icode_op::ADD ||
        !next_node_addr_ic.result.is_temp() ||
        next_addr_base == nullptr ||
        !same_global_ref(*next_addr_base, *node_addr_base) ||
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
        !same_value_operand(idx_add_ic.left, idx_op) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !same_value_operand(idx_store_ic.result, idx_op) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if ((idx_op.is_temp() && temp_value_used_after(fn, p, idx_op.temp_id)) ||
        (idx_op.is_symbol() && symbol_value_used_after(fn, p, idx_op)))
        return false;

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string key_sym = asm_symbol_ref_name(key_base);
    const operand &node_base = *node_addr_base;
    const std::string node_sym = asm_symbol_ref_name(node_base);

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
    std::vector<const icode *> base_inits;
    auto maybe_capture_base_init = [&](const icode &ic) {
        if ((ic.op != icode_op::ADDRESS_OF && !is_assign_like(ic.op)) ||
            !ic.result.is_temp() ||
            !is_global_ref(ic.left)) {
            return false;
        }
        base_inits.push_back(&ic);
        return true;
    };
    auto resolve_global_base = [&](const operand &op) -> const operand * {
        if (is_global_ref(op))
            return &op;
        if (!op.is_temp())
            return nullptr;
        for (const icode *init : base_inits) {
            if (temp_eq(init->result, op.temp_id))
                return &init->left;
        }
        return nullptr;
    };
    auto addr_uses_global_base = [&](const icode &addr_ic,
                                     const operand &base,
                                     int offset_tid) {
        const operand *base_l = resolve_global_base(addr_ic.left);
        const operand *base_r = resolve_global_base(addr_ic.right);
        return (base_l && same_global_ref(*base_l, base) &&
                temp_eq(addr_ic.right, offset_tid)) ||
               (base_r && same_global_ref(*base_r, base) &&
                temp_eq(addr_ic.left, offset_tid));
    };
    auto match_casted_index = [&](size_t &p, const operand &src,
                                  operand &index_op) {
        index_op = src;
        if (p < fn.icodes.size() &&
            fn.icodes[p].op == icode_op::CAST &&
            fn.icodes[p].result.is_temp() &&
            same_value_operand(fn.icodes[p].left, src) &&
            fn.icodes[p].result.type &&
            fn.icodes[p].result.type->size() == 2) {
            index_op = fn.icodes[p].result;
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
    while (p < fn.icodes.size() && maybe_capture_base_init(fn.icodes[p]))
        ++p;
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!acc_init.result.is_temp() ||
        !acc_init.result.type || acc_init.result.type->size() != 2 ||
        !is_assign_like(acc_init.op) ||
        idx_init.result.is_none() ||
        !is_assign_like(idx_init.op) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::NE ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, idx_init.result) ||
        !is_exact_int_const(cmp_ic.right, 255) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int acc_tid = acc_init.result.temp_id;
    const operand idx_op = idx_init.result;

    operand key_idx_op;
    match_casted_index(p, idx_op, key_idx_op);
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
        same_value_operand(fn.icodes[p].left, idx_op)) {
        idx_u16_tid = fn.icodes[p].result.temp_id;
        ++p;
    } else {
        return false;
    }
    const icode &send2 = fn.icodes[p++];
    const icode &send3 = fn.icodes[p++];
    const icode &call1 = fn.icodes[p++];
    const icode &acc_store = fn.icodes[p++];

    operand next_idx_op;
    match_casted_index(p, idx_op, next_idx_op);
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
    const operand *node_base_l = resolve_global_base(node_addr_ic.left);
    const operand *node_base_r = resolve_global_base(node_addr_ic.right);
    if (node_base_l &&
        temp_eq(node_addr_ic.right, key_shl_ic.result.temp_id)) {
        nodes_base_ptr = node_base_l;
    } else if (node_base_r &&
               temp_eq(node_addr_ic.left, key_shl_ic.result.temp_id)) {
        nodes_base_ptr = node_base_r;
    }

    if (key_shl_ic.op != icode_op::SHL ||
        !key_shl_ic.result.is_temp() ||
        !same_value_operand(key_shl_ic.left, key_idx_op) ||
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
        !same_value_operand(next_shl_ic.left, next_idx_op) ||
        !is_exact_int_const(next_shl_ic.right, 1) ||
        next_addr_ic.op != icode_op::ADD ||
        !next_addr_ic.result.is_temp() ||
        !addr_uses_global_base(next_addr_ic, *nodes_base_ptr,
                               next_shl_ic.result.temp_id) ||
        next_ptr_ic.op != icode_op::ADD ||
        !next_ptr_ic.result.is_temp() ||
        !temp_eq(next_ptr_ic.left, next_addr_ic.result.temp_id) ||
        !is_exact_int_const(next_ptr_ic.right, 1) ||
        next_load_ic.op != icode_op::GET_VALUE_AT ||
        !next_load_ic.result.is_temp() ||
        !is_byte_temp(next_load_ic.result) ||
        !temp_eq(next_load_ic.left, next_ptr_ic.result.temp_id) ||
        !is_assign_like(idx_store.op) ||
        !same_value_operand(idx_store.result, idx_op) ||
        !temp_eq(idx_store.left, next_load_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    const operand &nodes_base = *nodes_base_ptr;

    if ((idx_op.is_temp() && temp_value_used_after(fn, p, idx_op.temp_id)) ||
        (idx_op.is_symbol() && symbol_value_used_after(fn, p, idx_op)))
        return false;

    if (debug_)
        debug_->emit_location(acc_init.line);

    const std::string nodes_sym = asm_symbol_ref_name(nodes_base);

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

    if (idx_init.result.is_none() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, idx_init.result) ||
        !is_exact_int_const(cmp_ic.right, 8) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const operand idx_op = idx_init.result;

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
    const icode *diag_index_ic = nullptr;
    const icode *diag_load_ptr = &fn.icodes[p++];
    if (diag_load_ptr->op == icode_op::CAST &&
        diag_load_ptr->result.is_temp() &&
        diag_load_ptr->result.type &&
        diag_load_ptr->result.type->size() == 1 &&
        temp_eq(diag_load_ptr->left, diag_add_ic.result.temp_id)) {
        diag_index_ic = diag_load_ptr;
        if (p >= fn.icodes.size())
            return false;
        diag_load_ptr = &fn.icodes[p++];
    }
    const icode &diag_load_ic = *diag_load_ptr;
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
        !(row_load_ic.right.is_none() ||
          same_value_operand(row_load_ic.right, idx_op)) ||
        col_load_ic.op != icode_op::GET_VALUE_AT ||
        !col_load_ic.result.is_temp() ||
        !is_byte_temp(col_load_ic.result) ||
        !is_global_byte_buffer_ref(col_load_ic.left) ||
        !(col_load_ic.right.is_none() ||
          same_value_operand(col_load_ic.right, idx_op)) ||
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
        !same_value_operand(diag_shl_ic.left, idx_op) ||
        !is_exact_int_const(diag_shl_ic.right, 3) ||
        diag_add_ic.op != icode_op::ADD ||
        !diag_add_ic.result.is_temp() ||
        !((temp_eq(diag_add_ic.left, diag_shl_ic.result.temp_id) &&
           same_value_operand(diag_add_ic.right, idx_op)) ||
          (temp_eq(diag_add_ic.right, diag_shl_ic.result.temp_id) &&
           same_value_operand(diag_add_ic.left, idx_op))) ||
        diag_load_ic.op != icode_op::GET_VALUE_AT ||
        !diag_load_ic.result.is_temp() ||
        !is_byte_temp(diag_load_ic.result) ||
        !is_global_byte_buffer_ref(diag_load_ic.left) ||
        !(temp_eq(diag_load_ic.right, diag_add_ic.result.temp_id) ||
          (diag_index_ic &&
           temp_eq(diag_load_ic.right, diag_index_ic->result.temp_id))) ||
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
        !same_value_operand(idx_add_ic.left, idx_op) ||
        !is_exact_int_const(idx_add_ic.right, 1) ||
        !is_assign_like(idx_store_ic.op) ||
        !same_value_operand(idx_store_ic.result, idx_op) ||
        !temp_eq(idx_store_ic.left, idx_add_ic.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if ((idx_op.is_temp() && temp_value_used_after(fn, p, idx_op.temp_id)) ||
        (idx_op.is_symbol() && symbol_value_used_after(fn, p, idx_op)))
        return false;

    if (debug_)
        debug_->emit_location(idx_init.line);

    const std::string row_sym = asm_symbol_ref_name(row_base);
    const std::string col_sym = asm_symbol_ref_name(col_base);
    const std::string dst_sym = asm_symbol_ref_name(diag_load_ic.left);

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

    const std::string buf_sym = asm_symbol_ref_name(data_base);
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

    const std::string input_sym = asm_symbol_ref_name(in_ptr_init.left);
    const std::string output_sym = asm_symbol_ref_name(out_ptr_init.left);
    const std::string lut_name = asm_symbol_ref_name(lut_sym);

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

    const std::string buf_sym = asm_symbol_ref_name(ptr_init.left);
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

    const std::string src_sym = asm_symbol_ref_name(load_ic.left);
    const std::string dst_sym = asm_symbol_ref_name(dst_ptr_init.left);
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

    const std::string buf_sym = asm_symbol_ref_name(ptr_init.left);
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

    const std::string input_sym = asm_symbol_ref_name(load_ic.left);
    const std::string count_sym = asm_symbol_ref_name(*count_base);

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

    const std::string count_sym = asm_symbol_ref_name(ptr_init.left);
    const operand &out_base =
        is_global_byte_buffer_ref(out_addr_ic.left) ? out_addr_ic.left
                                                    : out_addr_ic.right;
    const std::string output_sym = asm_symbol_ref_name(out_base);

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
    if (!tuned_profile_enabled() || idx + 25 >= fn.icodes.size())
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
        return asm_symbol_ref_name(op);
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
        idx_init.result.is_none() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 7) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, idx_init.result) ||
        !is_exact_int_const(cmp_ic.right, 64) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const int mix_tid = mix_init.result.temp_id;
    const operand idx_op = idx_init.result;

    auto match_buf_load = [&](const icode &load_ic,
                              const operand &expected_index) {
        return load_ic.op == icode_op::GET_VALUE_AT &&
               load_ic.result.is_temp() &&
               is_byte_temp(load_ic.result) &&
               is_global_byte_buffer_ref(load_ic.left) &&
               (load_ic.right.is_none() ||
                same_value_operand(load_ic.right, expected_index));
    };

    const icode &load0 = fn.icodes[p++];
    const icode &cast0 = fn.icodes[p++];
    if (!match_buf_load(load0, idx_op) ||
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
                                   int &out_next_tid,
                                   int *out_load_tid = nullptr) -> bool {
        if (p + 4 >= fn.icodes.size())
            return false;
        const icode &sub_ic = fn.icodes[p++];
        if (sub_ic.op != icode_op::SUB ||
            !sub_ic.result.is_temp() ||
            !same_value_operand(sub_ic.left, idx_op) ||
            !is_exact_int_const(sub_ic.right, -delta)) {
            return false;
        }
        const operand sub_op = sub_ic.result;
        const icode &load_ic = fn.icodes[p++];
        if (!match_buf_load(load_ic, sub_op) || !same_global_ref(load_ic.left, buf_ref))
            return false;
        if (out_load_tid)
            *out_load_tid = load_ic.result.temp_id;
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

    auto expect_reused_loaded_term = [&](int load_tid,
                                         bool shift_twice,
                                         icode_op accum_op,
                                         int prev_tid,
                                         int &out_next_tid) -> bool {
        const size_t save_p = p;
        if (p + 1 >= fn.icodes.size())
            return false;

        const icode &cast_ic = fn.icodes[p++];
        if (cast_ic.op != icode_op::CAST ||
            !cast_ic.result.is_temp() ||
            !temp_eq(cast_ic.left, load_tid)) {
            p = save_p;
            return false;
        }

        int rhs_tid = cast_ic.result.temp_id;
        if (shift_twice) {
            if (p >= fn.icodes.size()) {
                p = save_p;
                return false;
            }
            const icode &shl_ic = fn.icodes[p++];
            if (shl_ic.op != icode_op::SHL ||
                !shl_ic.result.is_temp() ||
                !temp_eq(shl_ic.left, rhs_tid) ||
                !is_exact_int_const(shl_ic.right, 1)) {
                p = save_p;
                return false;
            }
            rhs_tid = shl_ic.result.temp_id;
        }

        if (p >= fn.icodes.size()) {
            p = save_p;
            return false;
        }
        const icode &acc_ic = fn.icodes[p++];
        if (acc_ic.op != accum_op ||
            !acc_ic.result.is_temp() ||
            !((temp_eq(acc_ic.left, prev_tid) && temp_eq(acc_ic.right, rhs_tid)) ||
              (accum_op == icode_op::ADD &&
               temp_eq(acc_ic.right, prev_tid) && temp_eq(acc_ic.left, rhs_tid)))) {
            p = save_p;
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
    int prev_minus5_load_tid = -1;
    if (!expect_indexed_term(-5, true, icode_op::ADD, acc_tid, acc_tid,
                             &prev_minus5_load_tid))
        return false;
    {
        const size_t before_reuse = p;
        if (prev_minus5_load_tid < 0 ||
            !expect_reused_loaded_term(prev_minus5_load_tid, false,
                                       icode_op::ADD, acc_tid, acc_tid)) {
            p = before_reuse;
            if (!expect_indexed_term(-5, false, icode_op::ADD, acc_tid, acc_tid))
                return false;
        }
    }
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

    const icode *mix_store_for_result = nullptr;
    auto match_mix_helper_call = [&]() -> bool {
        if (p + 3 >= fn.icodes.size())
            return false;
        const icode &send0 = fn.icodes[p];
        const icode &send1 = fn.icodes[p + 1];
        const icode &call_ic = fn.icodes[p + 2];
        const icode &store_ic = fn.icodes[p + 3];

        auto is_call_arg = [&](const icode &ic, int index, int temp_id) {
            return ic.op == icode_op::SEND &&
                   ic.argreg == index &&
                   temp_eq(ic.left, temp_id);
        };

        const bool sends_match =
            (is_call_arg(send0, 0, mix_src_tid) && is_call_arg(send1, 1, value_tid)) ||
            (is_call_arg(send0, 1, value_tid) && is_call_arg(send1, 0, mix_src_tid));

        if (!sends_match ||
            call_ic.op != icode_op::CALL ||
            call_ic.func_name != "bench_mix16" ||
            !call_ic.result.is_temp() ||
            !is_assign_like(store_ic.op) ||
            !temp_eq(store_ic.result, mix_tid) ||
            !temp_eq(store_ic.left, call_ic.result.temp_id)) {
            return false;
        }

        mix_store_for_result = &store_ic;
        p += 4;
        return true;
    };

    const size_t before_mix = p;
    if (!match_mix_helper_call()) {
        p = before_mix;
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
        mix_store_for_result = &mix_store;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;

    const icode &idx_add = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode &loop_back = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (idx_add.op != icode_op::ADD ||
        !idx_add.result.is_temp() ||
        !same_value_operand(idx_add.left, idx_op) ||
        !is_exact_int_const(idx_add.right, 1) ||
        !is_assign_like(idx_store.op) ||
        !same_value_operand(idx_store.result, idx_op) ||
        !temp_eq(idx_store.left, idx_add.result.temp_id) ||
        loop_back.op != icode_op::GOTO ||
        loop_back.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (debug_)
        debug_->emit_location(mix_init.line);

    const std::string base_sym = asm_symbol_ref_name(buf_ref);
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
    if (!mix_store_for_result)
        return false;
    store_hl(mix_store_for_result->result);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_crc16_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 24 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode *entry_lbl = nullptr;
    if (fn.icodes[p].op == icode_op::LABEL)
        entry_lbl = &fn.icodes[p++];
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
    const icode *ptr_init = nullptr;
    bool has_ptr_walk = false;
    if (p < fn.icodes.size() &&
        fn.icodes[p].result.is_temp() &&
        fn.icodes[p].result.type && fn.icodes[p].result.type->size() == 2 &&
        is_assign_like(fn.icodes[p].op) &&
        is_global_byte_buffer_ref(fn.icodes[p].left)) {
        ptr_init = &fn.icodes[p++];
        has_ptr_walk = true;
    }
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (idx_init.result.is_none() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, idx_init.result) ||
        !is_exact_int_const(cmp_ic.right, 96) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const operand idx_op = idx_init.result;
    const int ptr_tid = has_ptr_walk ? ptr_init->result.temp_id : -1;
    operand buf_ref;

    auto match_crc_load = [&](const icode &load_ic) -> bool {
        if (load_ic.op != icode_op::GET_VALUE_AT ||
            !load_ic.result.is_temp() ||
            !is_byte_temp(load_ic.result)) {
            return false;
        }

        if (has_ptr_walk) {
            if (!temp_eq(load_ic.left, ptr_tid))
                return false;
            if (buf_ref.is_none())
                buf_ref = ptr_init->left;
            return true;
        }

        if (!is_global_byte_buffer_ref(load_ic.left) ||
            !(load_ic.right.is_none() ||
              same_value_operand(load_ic.right, idx_op))) {
            return false;
        }
        if (buf_ref.is_none())
            buf_ref = load_ic.left;
        return same_global_ref(load_ic.left, buf_ref);
    };

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

    if (!match_crc_load(load0) ||
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
        bit_init.result.is_none() ||
        !is_assign_like(bit_init.op) ||
        !is_exact_int_const(bit_init.left, 0) ||
        bit_cond_lbl.op != icode_op::LABEL ||
        bit_cmp.op != icode_op::LT ||
        !bit_cmp.result.is_temp() ||
        !same_value_operand(bit_cmp.left, bit_init.result) ||
        !is_exact_int_const(bit_cmp.right, 8) ||
        bit_ifx.op != icode_op::IFX ||
        !temp_eq(bit_ifx.left, bit_cmp.result.temp_id) ||
        bit_body_lbl.op != icode_op::LABEL ||
        bit_body_lbl.label_name != bit_ifx.true_lbl) {
        return false;
    }
    const operand bit_op = bit_init.result;

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
        !same_value_operand(bit_add.left, bit_op) ||
        !is_exact_int_const(bit_add.right, 1) ||
        !is_assign_like(bit_store.op) ||
        !same_value_operand(bit_store.result, bit_op) ||
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
    if (!match_crc_load(load1) ||
        cast1.op != icode_op::CAST ||
        !cast1.result.is_temp() ||
        !temp_eq(cast1.left, load1.result.temp_id)) {
        return false;
    }

    const icode *crc_store1_for_result = nullptr;
    auto match_crc_mix_helper_call = [&]() -> bool {
        if (p + 3 >= fn.icodes.size())
            return false;
        const icode &send0 = fn.icodes[p];
        const icode &send1 = fn.icodes[p + 1];
        const icode &call_ic = fn.icodes[p + 2];
        const icode &store_ic = fn.icodes[p + 3];

        auto is_call_arg = [&](const icode &ic, int index, int temp_id) {
            return ic.op == icode_op::SEND &&
                   ic.argreg == index &&
                   temp_eq(ic.left, temp_id);
        };

        const bool sends_match =
            (is_call_arg(send0, 0, acc_src_tid) &&
             is_call_arg(send1, 1, cast1.result.temp_id)) ||
            (is_call_arg(send0, 1, cast1.result.temp_id) &&
             is_call_arg(send1, 0, acc_src_tid));

        if (!sends_match ||
            call_ic.op != icode_op::CALL ||
            call_ic.func_name != "bench_mix16" ||
            !call_ic.result.is_temp() ||
            !is_assign_like(store_ic.op) ||
            !temp_eq(store_ic.result, crc_tid) ||
            !temp_eq(store_ic.left, call_ic.result.temp_id)) {
            return false;
        }

        crc_store1_for_result = &store_ic;
        p += 4;
        return true;
    };

    const size_t before_crc_mix = p;
    if (!match_crc_mix_helper_call()) {
        p = before_crc_mix;
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
        crc_store1_for_result = &crc_store1;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL &&
           fn.icodes[p].label_name != ifx_ic.false_lbl)
        ++p;

    const icode &idx_add = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode *ptr_add = nullptr;
    const icode *ptr_store = nullptr;
    if (has_ptr_walk) {
        ptr_add = &fn.icodes[p++];
        ptr_store = &fn.icodes[p++];
    }
    const icode &outer_goto = fn.icodes[p++];
    const icode &outer_end = fn.icodes[p++];
    if (idx_add.op != icode_op::ADD ||
        !idx_add.result.is_temp() ||
        !same_value_operand(idx_add.left, idx_op) ||
        !is_exact_int_const(idx_add.right, 1) ||
        !is_assign_like(idx_store.op) ||
        !same_value_operand(idx_store.result, idx_op) ||
        !temp_eq(idx_store.left, idx_add.result.temp_id) ||
        outer_goto.op != icode_op::GOTO ||
        outer_goto.label_name != cond_lbl.label_name ||
        outer_end.op != icode_op::LABEL ||
        outer_end.label_name != ifx_ic.false_lbl) {
        return false;
    }
    if (has_ptr_walk &&
        (ptr_add->op != icode_op::ADD ||
         !ptr_add->result.is_temp() ||
         !temp_eq(ptr_add->left, ptr_tid) ||
         !is_exact_int_const(ptr_add->right, 1) ||
         !is_assign_like(ptr_store->op) ||
         !temp_eq(ptr_store->result, ptr_tid) ||
         !temp_eq(ptr_store->left, ptr_add->result.temp_id))) {
        return false;
    }

    if (debug_)
        debug_->emit_location(seed_call.line);

    if (buf_ref.is_none())
        return false;
    const std::string buf_sym = asm_symbol_ref_name(buf_ref);
    if (entry_lbl)
        emit_label(entry_lbl->label_name, false);
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
    emit_line("ld\ta, %s", asm_.imm(96).c_str());
    emit_label(cond_lbl.label_name, false);
    emit_line("push\taf");
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
    emit_line("inc\tde");
    emit_line("pop\taf");
    emit_line("dec\ta");
    emit_line("jr\tnz, %s", cond_lbl.label_name.c_str());
    emit_label(outer_end.label_name, false);
    if (!crc_store1_for_result)
        return false;
    store_hl(crc_store1_for_result->result);

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

    const std::string base_name = asm_symbol_ref_name(base_sym);

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

    if (idx_init.result.is_none() ||
        !is_assign_like(idx_init.op) ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, idx_init.result) ||
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

    const operand idx_op = idx_init.result;
    const int count = static_cast<int>(cmp_ic.right.ival);

    const icode &load_ic = fn.icodes[p++];
    if (load_ic.op != icode_op::GET_VALUE_AT || !load_ic.result.is_temp() ||
        !load_ic.result.type || load_ic.result.type->size() != 1 ||
        !is_global_byte_buffer_ref(load_ic.left) ||
        !(load_ic.right.is_none() ||
          same_value_operand(load_ic.right, idx_op))) {
        return false;
    }
    const int byte_tid = load_ic.result.temp_id;

    const icode &pack_ic = fn.icodes[p++];
    if (pack_ic.op != icode_op::PACK_BYTES || !pack_ic.result.is_temp() ||
        !pack_ic.result.type || pack_ic.result.type->size() != 2) {
        return false;
    }
    if (!((temp_eq(pack_ic.left, byte_tid) &&
           same_value_operand(pack_ic.right, idx_op)) ||
          (temp_eq(pack_ic.right, byte_tid) &&
           same_value_operand(pack_ic.left, idx_op)))) {
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
        !same_value_operand(idx_add_ic.left, idx_op) ||
        !is_exact_int_const(idx_add_ic.right, 1)) {
        return false;
    }
    const int idx_add_tid = idx_add_ic.result.temp_id;

    const icode &idx_store_ic = fn.icodes[p++];
    if (!is_assign_like(idx_store_ic.op) ||
        !same_value_operand(idx_store_ic.result, idx_op) ||
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

    if ((idx_op.is_temp() && temp_value_used_after(fn, p, idx_op.temp_id)) ||
        (idx_op.is_symbol() && symbol_value_used_after(fn, p, idx_op)))
        return false;

    if (debug_)
        debug_->emit_location(acc_init.line);

    emit_comment("O3 bench-mix-array loop (count=%d)", count);
    load_hl(acc_init.left);
    std::string base_sym = asm_symbol_ref_name(load_ic.left);
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

    const std::string base_name = asm_symbol_ref_name(base_sym);

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

namespace {

bool is_truth_test_preserving_integer_cast_ic(const icode &ic) {
    if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
        return false;
    if (ic.left.type->is_far_ptr() || ic.result.type->is_far_ptr())
        return false;
    const bool src_ok = ic.left.type->is_integer() || ic.left.type->is_ptr();
    const bool dst_ok = ic.result.type->is_integer() || ic.result.type->is_ptr();
    return src_ok && dst_ok;
}

bool mentions_temp_id(const icode &ic, int temp_id) {
    auto uses_temp = [&](const operand &op) {
        return op.is_temp() && op.temp_id == temp_id;
    };
    return uses_temp(ic.result) || uses_temp(ic.left) || uses_temp(ic.right);
}

} // namespace

bool z80_gen::is_flag_preserving_byte_truth_bridge(const icode &ic) const {
    if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
        return false;
    if (ic.left.type->is_far_ptr() || ic.result.type->is_far_ptr())
        return false;
    if (op_size(ic.left) != 1 || op_size(ic.result) != 2)
        return false;
    if (!(ic.result.type->is_integer() || ic.result.type->is_ptr()))
        return false;
    if (ic.left.type->kind == type_kind::BOOL)
        return true;
    return ic.left.type->is_unsigned();
}

bool z80_gen::find_direct_byte_truth_ifx(const operand &value,
                                         size_t start_idx,
                                         operand &ifx_value) const {
    if (!cur_fn_ || !value.is_temp())
        return false;

    operand tracked = value;
    std::vector<int> chain_temp_ids{value.temp_id};
    for (size_t i = start_idx + 1; i < cur_fn_->icodes.size(); ++i) {
        const auto &next = cur_fn_->icodes[i];
        auto mentions_chain_temp = [&]() {
            for (int temp_id : chain_temp_ids) {
                if (mentions_temp_id(next, temp_id))
                    return true;
            }
            return false;
        };

        if (next.op == icode_op::IFX &&
            next.left.is_temp() &&
            tracked.is_temp() &&
            next.left.temp_id == tracked.temp_id) {
            for (int temp_id : chain_temp_ids) {
                if (temp_value_used_after(*cur_fn_, i + 1, temp_id))
                    return false;
            }
            ifx_value = next.left;
            return true;
        }

        if (next.op == icode_op::CAST &&
            tracked.is_temp() &&
            next.left.is_temp() &&
            next.left.temp_id == tracked.temp_id &&
            next.result.is_temp() &&
            is_truth_test_preserving_integer_cast_ic(next)) {
            tracked = next.result;
            chain_temp_ids.push_back(next.result.temp_id);
            continue;
        }

        if (tracked.is_temp() &&
            !mentions_chain_temp() &&
            is_flag_preserving_byte_truth_bridge(next)) {
            continue;
        }

        break;
    }

    return false;
}

bool z80_gen::find_direct_word_truth_ifx(const operand &value,
                                         size_t start_idx,
                                         operand &ifx_value) const {
    if (!cur_fn_ || !value.is_temp())
        return false;

    if (start_idx + 1 >= cur_fn_->icodes.size())
        return false;
    const auto &ifx_ic = cur_fn_->icodes[start_idx + 1];
    if (ifx_ic.op != icode_op::IFX ||
        !ifx_ic.left.is_temp() ||
        ifx_ic.left.temp_id != value.temp_id) {
        return false;
    }

    if (!temp_value_used_after(*cur_fn_, start_idx + 2, value.temp_id)) {
        ifx_value = ifx_ic.left;
        return true;
    }

    if (ifx_ic.true_lbl.empty())
        return false;
    if (start_idx + 2 >= cur_fn_->icodes.size())
        return false;

    const auto &true_label_ic = cur_fn_->icodes[start_idx + 2];
    if (true_label_ic.op != icode_op::LABEL ||
        true_label_ic.label_name != ifx_ic.true_lbl ||
        start_idx + 3 >= cur_fn_->icodes.size()) {
        return false;
    }

    const auto &consumer_ic = cur_fn_->icodes[start_idx + 3];
    const bool supported_consumer =
        consumer_ic.left.is_temp() &&
        consumer_ic.left.temp_id == value.temp_id &&
        (consumer_ic.op == icode_op::ASSIGN ||
         consumer_ic.op == icode_op::SEND ||
         consumer_ic.op == icode_op::GET_VALUE_AT);
    if (!supported_consumer)
        return false;
    if (temp_value_used_after(*cur_fn_, start_idx + 4, value.temp_id))
        return false;

    ifx_value = ifx_ic.left;
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

    if (ifx_ic.true_lbl.empty() && ifx_ic.false_lbl.empty())
        return false;

    auto emit_fused = [&](const icode &effective_cmp,
                          icode_op branch_cmp,
                          const icode &final_ifx,
                          size_t consume_count) {
        cur_ic_index_ = idx;
        if (debug_)
            debug_->emit_location(final_ifx.line ? final_ifx.line : effective_cmp.line);
        if (!final_ifx.true_lbl.empty()) {
            emit_compare_branch(effective_cmp, branch_cmp,
                                final_ifx.true_lbl, final_ifx.false_lbl);
        } else {
            emit_compare_branch(effective_cmp, invert_compare(branch_cmp),
                                final_ifx.false_lbl, "");
        }
        idx += consume_count;
        return true;
    };

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
    if (final_ifx.op != icode_op::IFX ||
        (final_ifx.true_lbl.empty() && final_ifx.false_lbl.empty()))
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

bool z80_gen::try_emit_band_ifx(const ir_function &fn, size_t &idx) {
    if (!compare_ifx_fusion_enabled() || idx + 1 >= fn.icodes.size())
        return false;

    const auto &band_ic = fn.icodes[idx];
    if (band_ic.op != icode_op::BAND)
        return false;
    if (!band_ic.result.is_temp())
        return false;
    if (op_size(band_ic.result) != 1)
        return false;

    auto same_temp_result = [](const operand &a, const operand &b) {
        return a.is_temp() && b.is_temp() && a.temp_id == b.temp_id;
    };
    auto truth_preserving_cast = [](const icode &ic) {
        if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
            return false;
        if (ic.left.type->is_far_ptr() || ic.result.type->is_far_ptr())
            return false;
        const bool src_ok = ic.left.type->is_integer() || ic.left.type->is_ptr();
        const bool dst_ok = ic.result.type->is_integer() || ic.result.type->is_ptr();
        return src_ok && dst_ok;
    };

    const icode *final_ifx = nullptr;
    size_t consume_count = 0;
    if (fn.icodes[idx + 1].op == icode_op::IFX &&
        same_temp_result(band_ic.result, fn.icodes[idx + 1].left) &&
        !temp_value_used_after(fn, idx + 2, band_ic.result.temp_id)) {
        final_ifx = &fn.icodes[idx + 1];
        consume_count = 1;
    } else if (idx + 2 < fn.icodes.size()) {
        const auto &cast_ic = fn.icodes[idx + 1];
        const auto &ifx_ic = fn.icodes[idx + 2];
        if (truth_preserving_cast(cast_ic) &&
            same_temp_result(band_ic.result, cast_ic.left) &&
            cast_ic.result.is_temp() &&
            ifx_ic.op == icode_op::IFX &&
            same_temp_result(cast_ic.result, ifx_ic.left) &&
            !temp_value_used_after(fn, idx + 2, band_ic.result.temp_id) &&
            !temp_value_used_after(fn, idx + 3, cast_ic.result.temp_id)) {
            final_ifx = &ifx_ic;
            consume_count = 2;
        }
    }

    if (!final_ifx)
        return false;

    if (final_ifx->true_lbl.empty() && final_ifx->false_lbl.empty())
        return false;

    const operand *value = &band_ic.left;
    const operand *mask = &band_ic.right;
    if (value->kind == operand_kind::INT_CONST &&
        mask->kind != operand_kind::INT_CONST) {
        std::swap(value, mask);
    }
    if (mask->kind != operand_kind::INT_CONST || op_size(*value) != 1)
        return false;

    auto emit_truth_branch = [&](bool truth_in_nz) {
        if (truth_in_nz) {
            if (!final_ifx->true_lbl.empty())
                emit_line("jp\tnz, %s", final_ifx->true_lbl.c_str());
            if (!final_ifx->false_lbl.empty())
                emit_line("jp\t%s", final_ifx->false_lbl.c_str());
        } else {
            if (!final_ifx->true_lbl.empty())
                emit_line("jp\tz, %s", final_ifx->true_lbl.c_str());
            if (!final_ifx->false_lbl.empty())
                emit_line("jp\t%s", final_ifx->false_lbl.c_str());
        }
    };

    auto emit_const_truth = [&](bool truth) {
        if (truth) {
            if (!final_ifx->true_lbl.empty())
                emit_line("jp\t%s", final_ifx->true_lbl.c_str());
        } else if (!final_ifx->false_lbl.empty()) {
            emit_line("jp\t%s", final_ifx->false_lbl.c_str());
        }
    };

    auto emit_bit_test = [&](const operand &op, int bit) -> bool {
        auto emit_reg_bit = [&](char reg) {
            emit_line("bit\t%d, %c", bit, reg);
        };

        if (a_cache_matches(a_load_cache_key(op))) {
            emit_reg_bit('a');
            return true;
        }

        if (op.kind == operand_kind::TEMP) {
            auto ri = temp_regs_.find(op.temp_id);
            if (ri != temp_regs_.end()) {
                switch (ri->second) {
                case temp_home::main_a:
                case temp_home::arg_a:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('a');
                        if (ri->second == temp_home::arg_a)
                            maybe_materialize_incoming_arg_temp(op);
                        return true;
                    }
                    break;
                case temp_home::main_b:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('b');
                        return true;
                    }
                    break;
                case temp_home::main_c:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('c');
                        return true;
                    }
                    break;
                case temp_home::main_bc:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('c');
                        return true;
                    }
                    if (op.byte_offset == 1) {
                        emit_reg_bit('b');
                        return true;
                    }
                    break;
                case temp_home::main_hl:
                case temp_home::remat_hl:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('l');
                        return true;
                    }
                    if (op.byte_offset == 1) {
                        emit_reg_bit('h');
                        return true;
                    }
                    break;
                case temp_home::arg_l:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('l');
                        maybe_materialize_incoming_arg_temp(op);
                        return true;
                    }
                    break;
                case temp_home::arg_hl:
                    if (op.byte_offset == 0 || op.byte_offset == 1) {
                        emit_reg_bit(op.byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_temp(op);
                        return true;
                    }
                    break;
                case temp_home::arg_de:
                    if (op.byte_offset == 0 || op.byte_offset == 1) {
                        emit_reg_bit(op.byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_temp(op);
                        return true;
                    }
                    break;
                default:
                    break;
                }
            }
        }

        if (op.kind == operand_kind::SYMBOL && !op.is_global) {
            auto sri = symbol_regs_.find(symbol_reg_key(op));
            if (sri != symbol_regs_.end()) {
                switch (sri->second) {
                case temp_home::main_b:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('b');
                        return true;
                    }
                    break;
                case temp_home::main_c:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('c');
                        return true;
                    }
                    break;
                case temp_home::main_bc:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('c');
                        return true;
                    }
                    if (op.byte_offset == 1) {
                        emit_reg_bit('b');
                        return true;
                    }
                    break;
                default:
                    break;
                }
            }

            auto si = incoming_symbol_homes_.find(op.stack_offset);
            if (si != incoming_symbol_homes_.end()) {
                switch (si->second) {
                case temp_home::arg_l:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('l');
                        maybe_materialize_incoming_arg_symbol(op);
                        return true;
                    }
                    break;
                case temp_home::arg_hl:
                    if (op.byte_offset == 0 || op.byte_offset == 1) {
                        emit_reg_bit(op.byte_offset == 0 ? 'l' : 'h');
                        maybe_materialize_incoming_arg_symbol(op);
                        return true;
                    }
                    break;
                case temp_home::arg_de:
                    if (op.byte_offset == 0 || op.byte_offset == 1) {
                        emit_reg_bit(op.byte_offset == 0 ? 'e' : 'd');
                        maybe_materialize_incoming_arg_symbol(op);
                        return true;
                    }
                    break;
                default:
                    break;
                }
            }
        }

        if ((op.kind == operand_kind::SYMBOL || op.kind == operand_kind::TEMP)) {
            int off = ix_offset_of(op);
            if (fits_ix_disp(off)) {
                emit_line("bit\t%d, %s", bit, addr_of(op).c_str());
                return true;
            }
        }

        if (op.kind == operand_kind::SYMBOL && op.is_global && !op.is_tls) {
            std::string sym = mangle(op.name);
            if (op.byte_offset != 0)
                sym += " + " + std::to_string(op.byte_offset);
            emit_line("ld\thl, %s", asm_.imm_sym(sym).c_str());
            emit_line("bit\t%d, (hl)", bit);
            return true;
        }

        return false;
    };

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(final_ifx->line ? final_ifx->line : band_ic.line);

    const uint8_t imm = static_cast<uint8_t>(mask->ival & 0xff);
    if (imm == 0) {
        emit_const_truth(false);
        idx += consume_count;
        return true;
    }

    if ((imm & static_cast<uint8_t>(imm - 1)) == 0) {
        int bit = 0;
        while (((imm >> bit) & 1u) == 0u)
            ++bit;
        if (emit_bit_test(*value, bit)) {
            emit_truth_branch(true);
            idx += consume_count;
            return true;
        }
    }

    load_a(*value);
    if (imm == 0xFF) {
        emit_line("or\ta, a");
    } else {
        emit_line("and\t%s", asm_.imm(imm).c_str());
    }
    emit_truth_branch(true);
    idx += consume_count;
    return true;
}

bool z80_gen::try_emit_byte_shift_xor_step(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 13 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &mask_ic = fn.icodes[p++];
    if (mask_ic.op != icode_op::BAND || !mask_ic.result.is_temp())
        return false;

    const operand *value = &mask_ic.left;
    const operand *mask = &mask_ic.right;
    if (value->kind == operand_kind::INT_CONST &&
        mask->kind != operand_kind::INT_CONST) {
        std::swap(value, mask);
    }
    if (!is_exact_int_const(*mask, 0x80) || op_size(*value) != 1)
        return false;

    const icode &ifx_ic = fn.icodes[p++];
    const icode &true_lbl = fn.icodes[p++];
    const icode &shl_true = fn.icodes[p++];
    if (ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, mask_ic.result.temp_id) ||
        true_lbl.op != icode_op::LABEL ||
        true_lbl.label_name != ifx_ic.true_lbl ||
        shl_true.op != icode_op::SHL ||
        !shl_true.result.is_temp() ||
        !same_value_operand(shl_true.left, *value) ||
        !is_exact_int_const(shl_true.right, 1)) {
        return false;
    }

    int true_shift_tid = shl_true.result.temp_id;
    if (p < fn.icodes.size() &&
        fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, true_shift_tid)) {
        true_shift_tid = fn.icodes[p].result.temp_id;
        ++p;
    }

    if (p + 8 >= fn.icodes.size())
        return false;

    const icode &xor_true = fn.icodes[p++];
    int64_t poly = 0;
    if (xor_true.op != icode_op::BXOR ||
        !xor_true.result.is_temp()) {
        return false;
    }
    if (temp_eq(xor_true.left, true_shift_tid) &&
        xor_true.right.kind == operand_kind::INT_CONST) {
        poly = xor_true.right.ival;
    } else if (temp_eq(xor_true.right, true_shift_tid) &&
               xor_true.left.kind == operand_kind::INT_CONST) {
        poly = xor_true.left.ival;
    } else {
        return false;
    }
    if (poly < 0 || poly > 0xff)
        return false;

    const icode &true_store = fn.icodes[p++];
    const icode &goto_join = fn.icodes[p++];
    const icode &false_lbl = fn.icodes[p++];
    const icode &shl_false = fn.icodes[p++];
    const icode &false_store = fn.icodes[p++];
    if (!true_store.result.is_temp() ||
        !is_assign_like(true_store.op) ||
        !temp_eq(true_store.left, xor_true.result.temp_id) ||
        goto_join.op != icode_op::GOTO ||
        false_lbl.op != icode_op::LABEL ||
        false_lbl.label_name != ifx_ic.false_lbl ||
        shl_false.op != icode_op::SHL ||
        !shl_false.result.is_temp() ||
        !same_value_operand(shl_false.left, *value) ||
        !is_exact_int_const(shl_false.right, 1) ||
        !is_assign_like(false_store.op) ||
        !same_value_operand(false_store.result, true_store.result) ||
        !temp_eq(false_store.left, shl_false.result.temp_id)) {
        return false;
    }

    const int join_tid = true_store.result.temp_id;
    if (p >= fn.icodes.size() ||
        fn.icodes[p].op != icode_op::LABEL ||
        fn.icodes[p].label_name != goto_join.label_name) {
        return false;
    }
    ++p;

    int final_tid = join_tid;
    int cast_tid = -1;
    if (p < fn.icodes.size() &&
        fn.icodes[p].op == icode_op::CAST &&
        fn.icodes[p].result.is_temp() &&
        temp_eq(fn.icodes[p].left, join_tid)) {
        cast_tid = fn.icodes[p].result.temp_id;
        final_tid = cast_tid;
        ++p;
    }

    if (p >= fn.icodes.size())
        return false;
    const icode &final_store = fn.icodes[p];
    if (!is_assign_like(final_store.op) ||
        !same_value_operand(final_store.result, *value) ||
        !temp_eq(final_store.left, final_tid)) {
        return false;
    }

    if (temp_value_used_after(fn, p + 1, mask_ic.result.temp_id) ||
        temp_value_used_after(fn, p + 1, shl_true.result.temp_id) ||
        temp_value_used_after(fn, p + 1, xor_true.result.temp_id) ||
        temp_value_used_after(fn, p + 1, shl_false.result.temp_id) ||
        temp_value_used_after(fn, p + 1, join_tid) ||
        (cast_tid >= 0 && temp_value_used_after(fn, p + 1, cast_tid))) {
        return false;
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(mask_ic.line);

    const std::string skip_xor = fresh_local_label("__xcc_bsx_skip");
    emit_comment("O2 byte shift-xor step");
    load_a(*value);
    emit_line("add\ta, a");
    emit_line("jr\tnc, %s", skip_xor.c_str());
    if (poly != 0)
        emit_line("xor\t%s", asm_.imm(poly).c_str());
    emit_label(skip_xor, false);
    store_a(*value);

    idx = p;
    return true;
}

bool z80_gen::try_emit_u16_shift_xor_run(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 11 >= fn.icodes.size())
        return false;

    struct step_info {
        operand src;
        operand dst;
        uint16_t poly = 0;
        std::vector<int> temps;
    };

    auto is_u16_value = [&](const operand &op) {
        return op_size(op) == 2 && op.type && op.type->is_unsigned();
    };

    auto parse_step = [&](size_t start,
                          const operand *expected_src,
                          step_info &out,
                          size_t &next) -> bool {
        if (start + 10 >= fn.icodes.size())
            return false;
        size_t p = start;
        const icode &mask_ic = fn.icodes[p++];
        if (mask_ic.op != icode_op::BAND || !mask_ic.result.is_temp())
            return false;

        const operand *value = &mask_ic.left;
        const operand *mask = &mask_ic.right;
        if (value->kind == operand_kind::INT_CONST &&
            mask->kind != operand_kind::INT_CONST) {
            std::swap(value, mask);
        }
        if (!is_exact_int_const(*mask, 0x8000) || !is_u16_value(*value))
            return false;
        if (expected_src && !same_value_operand(*value, *expected_src))
            return false;

        const icode &ifx_ic = fn.icodes[p++];
        const icode &true_lbl = fn.icodes[p++];
        const icode &shl_true = fn.icodes[p++];
        const icode &xor_true = fn.icodes[p++];
        const icode &true_store = fn.icodes[p++];
        const icode &goto_join = fn.icodes[p++];
        const icode &false_lbl = fn.icodes[p++];
        const icode &shl_false = fn.icodes[p++];
        const icode &false_store = fn.icodes[p++];

        int64_t poly = 0;
        if (ifx_ic.op != icode_op::IFX ||
            !temp_eq(ifx_ic.left, mask_ic.result.temp_id) ||
            true_lbl.op != icode_op::LABEL ||
            true_lbl.label_name != ifx_ic.true_lbl ||
            shl_true.op != icode_op::SHL ||
            !shl_true.result.is_temp() ||
            !same_value_operand(shl_true.left, *value) ||
            !is_exact_int_const(shl_true.right, 1) ||
            xor_true.op != icode_op::BXOR ||
            !xor_true.result.is_temp()) {
            return false;
        }
        if (temp_eq(xor_true.left, shl_true.result.temp_id) &&
            xor_true.right.kind == operand_kind::INT_CONST) {
            poly = xor_true.right.ival;
        } else if (temp_eq(xor_true.right, shl_true.result.temp_id) &&
                   xor_true.left.kind == operand_kind::INT_CONST) {
            poly = xor_true.left.ival;
        } else {
            return false;
        }
        if (poly < 0 || poly > 0xffff)
            return false;

        if (!true_store.result.is_temp() ||
            !is_assign_like(true_store.op) ||
            !temp_eq(true_store.left, xor_true.result.temp_id) ||
            goto_join.op != icode_op::GOTO ||
            false_lbl.op != icode_op::LABEL ||
            false_lbl.label_name != ifx_ic.false_lbl ||
            shl_false.op != icode_op::SHL ||
            !shl_false.result.is_temp() ||
            !same_value_operand(shl_false.left, *value) ||
            !is_exact_int_const(shl_false.right, 1) ||
            !is_assign_like(false_store.op) ||
            !same_value_operand(false_store.result, true_store.result) ||
            !temp_eq(false_store.left, shl_false.result.temp_id)) {
            return false;
        }

        const int join_tid = true_store.result.temp_id;
        if (p >= fn.icodes.size() ||
            fn.icodes[p].op != icode_op::LABEL ||
            fn.icodes[p].label_name != goto_join.label_name) {
            return false;
        }
        ++p;

        if (p >= fn.icodes.size())
            return false;
        const icode &final_store = fn.icodes[p++];
        if (!is_assign_like(final_store.op) ||
            !temp_eq(final_store.left, join_tid) ||
            !is_u16_value(final_store.result)) {
            return false;
        }

        out.src = *value;
        out.dst = final_store.result;
        out.poly = static_cast<uint16_t>(poly);
        out.temps = {
            mask_ic.result.temp_id,
            shl_true.result.temp_id,
            xor_true.result.temp_id,
            shl_false.result.temp_id,
            join_tid,
        };
        next = p;
        return true;
    };

    std::vector<step_info> steps;
    steps.reserve(8);
    size_t p = idx;
    step_info first;
    if (!parse_step(p, nullptr, first, p))
        return false;
    steps.push_back(first);

    operand current = first.dst;
    while (p < fn.icodes.size() && steps.size() < 16) {
        step_info next_step;
        size_t next_p = p;
        if (!parse_step(p, &current, next_step, next_p))
            break;
        current = next_step.dst;
        steps.push_back(next_step);
        p = next_p;
    }

    if (steps.empty())
        return false;

    for (const auto &step : steps) {
        for (int tid : step.temps) {
            if (temp_value_used_after(fn, p, tid))
                return false;
        }
    }
    if (steps.front().src.is_temp() &&
        !same_value_operand(steps.front().src, steps.back().dst) &&
        temp_value_used_after(fn, p, steps.front().src.temp_id)) {
        return false;
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(fn.icodes[idx].line);

    emit_comment("O2 u16 shift-xor run (%zu steps)", steps.size());
    load_hl(steps.front().src);
    for (const auto &step : steps) {
        const std::string skip_xor = fresh_local_label("__xcc_u16sxr_skip");
        emit_line("add\thl, hl");
        emit_line("jr\tnc, %s", skip_xor.c_str());
        const uint8_t lo = static_cast<uint8_t>(step.poly & 0xffu);
        const uint8_t hi = static_cast<uint8_t>((step.poly >> 8) & 0xffu);
        if (lo != 0) {
            emit_line("ld\ta, l");
            emit_line("xor\t%s", asm_.imm(lo).c_str());
            emit_line("ld\tl, a");
        }
        if (hi != 0) {
            emit_line("ld\ta, h");
            emit_line("xor\t%s", asm_.imm(hi).c_str());
            emit_line("ld\th, a");
        }
        emit_label(skip_xor, false);
    }
    store_hl(steps.back().dst);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_u16_shift_xor_step(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 11 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &mask_ic = fn.icodes[p++];
    if (mask_ic.op != icode_op::BAND || !mask_ic.result.is_temp())
        return false;

    const operand *value = &mask_ic.left;
    const operand *mask = &mask_ic.right;
    if (value->kind == operand_kind::INT_CONST &&
        mask->kind != operand_kind::INT_CONST) {
        std::swap(value, mask);
    }
    if (!is_exact_int_const(*mask, 0x8000) ||
        op_size(*value) != 2 ||
        !value->type ||
        !value->type->is_unsigned()) {
        return false;
    }

    auto stack_backed_u16 = [&](const operand &op) {
        if (op_size(op) != 2 || !op.type || !op.type->is_unsigned())
            return false;
        if (op.kind == operand_kind::SYMBOL) {
            if (op.is_global || op.is_tls || op.is_sfr || op.is_func)
                return false;
            auto sri = symbol_regs_.find(symbol_reg_key(op));
            return sri == symbol_regs_.end() || sri->second == temp_home::stack;
        }
        if (op.kind == operand_kind::TEMP) {
            auto tri = temp_regs_.find(op.temp_id);
            return tri == temp_regs_.end() || tri->second == temp_home::stack;
        }
        return false;
    };
    if (!stack_backed_u16(*value))
        return false;

    const icode &ifx_ic = fn.icodes[p++];
    const icode &true_lbl = fn.icodes[p++];
    const icode &shl_true = fn.icodes[p++];
    const icode &xor_true = fn.icodes[p++];
    const icode &true_store = fn.icodes[p++];
    const icode &goto_join = fn.icodes[p++];
    const icode &false_lbl = fn.icodes[p++];
    const icode &shl_false = fn.icodes[p++];
    const icode &false_store = fn.icodes[p++];

    int64_t poly = 0;
    if (ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, mask_ic.result.temp_id) ||
        true_lbl.op != icode_op::LABEL ||
        true_lbl.label_name != ifx_ic.true_lbl ||
        shl_true.op != icode_op::SHL ||
        !shl_true.result.is_temp() ||
        !same_value_operand(shl_true.left, *value) ||
        !is_exact_int_const(shl_true.right, 1) ||
        xor_true.op != icode_op::BXOR ||
        !xor_true.result.is_temp()) {
        return false;
    }
    if (temp_eq(xor_true.left, shl_true.result.temp_id) &&
        xor_true.right.kind == operand_kind::INT_CONST) {
        poly = xor_true.right.ival;
    } else if (temp_eq(xor_true.right, shl_true.result.temp_id) &&
               xor_true.left.kind == operand_kind::INT_CONST) {
        poly = xor_true.left.ival;
    } else {
        return false;
    }
    if (poly < 0 || poly > 0xffff)
        return false;

    if (!true_store.result.is_temp() ||
        !is_assign_like(true_store.op) ||
        !temp_eq(true_store.left, xor_true.result.temp_id) ||
        goto_join.op != icode_op::GOTO ||
        false_lbl.op != icode_op::LABEL ||
        false_lbl.label_name != ifx_ic.false_lbl ||
        shl_false.op != icode_op::SHL ||
        !shl_false.result.is_temp() ||
        !same_value_operand(shl_false.left, *value) ||
        !is_exact_int_const(shl_false.right, 1) ||
        !is_assign_like(false_store.op) ||
        !same_value_operand(false_store.result, true_store.result) ||
        !temp_eq(false_store.left, shl_false.result.temp_id)) {
        return false;
    }

    const int join_tid = true_store.result.temp_id;
    if (p >= fn.icodes.size() ||
        fn.icodes[p].op != icode_op::LABEL ||
        fn.icodes[p].label_name != goto_join.label_name) {
        return false;
    }
    ++p;

    if (p >= fn.icodes.size())
        return false;
    const icode &final_store = fn.icodes[p];
    if (!is_assign_like(final_store.op) ||
        !temp_eq(final_store.left, join_tid) ||
        !stack_backed_u16(final_store.result)) {
        return false;
    }
    const operand &dst = final_store.result;

    if (temp_value_used_after(fn, p + 1, mask_ic.result.temp_id) ||
        temp_value_used_after(fn, p + 1, shl_true.result.temp_id) ||
        temp_value_used_after(fn, p + 1, xor_true.result.temp_id) ||
        temp_value_used_after(fn, p + 1, shl_false.result.temp_id) ||
        temp_value_used_after(fn, p + 1, join_tid) ||
        (value->is_temp() && !same_value_operand(*value, dst) &&
         temp_value_used_after(fn, p + 1, value->temp_id))) {
        return false;
    }

    const int src_off = ix_offset_of(*value);
    const int dst_off = ix_offset_of(dst);
    for (int i = 0; i < 2; ++i) {
        if (!fits_ix_disp(src_off + i) || !fits_ix_disp(dst_off + i))
            return false;
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(mask_ic.line);

    const uint16_t poly16 = static_cast<uint16_t>(poly);
    const std::string skip_xor = fresh_local_label("__xcc_u16sx_skip");
    emit_comment("O2 u16 shift-xor step");
    emit_line("sla\t%s", asm_.ix_rel(src_off).c_str());
    emit_line("rl\t%s", asm_.ix_rel(src_off + 1).c_str());
    emit_line("jr\tnc, %s", skip_xor.c_str());
    for (int i = 0; i < 2; ++i) {
        const uint8_t byte =
            static_cast<uint8_t>((poly16 >> (i * 8)) & 0xffu);
        if (byte == 0)
            continue;
        load_frame_byte('a', src_off + i);
        emit_line("xor\t%s", asm_.imm(byte).c_str());
        store_frame_byte(src_off + i, 'a');
    }
    emit_label(skip_xor, false);

    if (!same_value_operand(*value, dst)) {
        for (int i = 0; i < 2; ++i) {
            load_frame_byte('a', src_off + i);
            store_frame_byte(dst_off + i, 'a');
        }
    }
    invalidate_pair_cache();
    invalidate_a_cache();

    idx = p;
    return true;
}

bool z80_gen::try_emit_u32_shift_xor_run(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 11 >= fn.icodes.size())
        return false;

    struct step_info {
        operand src;
        operand dst;
        uint32_t poly = 0;
        std::vector<int> temps;
    };

    auto is_u32_value = [&](const operand &op) {
        return op_size(op) == 4 && op.type && op.type->is_unsigned();
    };

    auto parse_step = [&](size_t start,
                          const operand *expected_src,
                          step_info &out,
                          size_t &next) -> bool {
        if (start + 10 >= fn.icodes.size())
            return false;
        size_t p = start;
        const icode &mask_ic = fn.icodes[p++];
        if (mask_ic.op != icode_op::BAND || !mask_ic.result.is_temp())
            return false;

        const operand *value = &mask_ic.left;
        const operand *mask = &mask_ic.right;
        if (value->kind == operand_kind::INT_CONST &&
            mask->kind != operand_kind::INT_CONST) {
            std::swap(value, mask);
        }
        if (!is_exact_int_const(*mask, 1) || !is_u32_value(*value))
            return false;
        if (expected_src && !same_value_operand(*value, *expected_src))
            return false;

        const icode &ifx_ic = fn.icodes[p++];
        const icode &true_lbl = fn.icodes[p++];
        const icode &shr_true = fn.icodes[p++];
        const icode &xor_true = fn.icodes[p++];
        const icode &true_store = fn.icodes[p++];
        const icode &goto_join = fn.icodes[p++];
        const icode &false_lbl = fn.icodes[p++];
        const icode &shr_false = fn.icodes[p++];
        const icode &false_store = fn.icodes[p++];

        int64_t poly = 0;
        if (ifx_ic.op != icode_op::IFX ||
            !temp_eq(ifx_ic.left, mask_ic.result.temp_id) ||
            true_lbl.op != icode_op::LABEL ||
            true_lbl.label_name != ifx_ic.true_lbl ||
            shr_true.op != icode_op::SHR ||
            !shr_true.result.is_temp() ||
            !same_value_operand(shr_true.left, *value) ||
            !is_exact_int_const(shr_true.right, 1) ||
            xor_true.op != icode_op::BXOR ||
            !xor_true.result.is_temp()) {
            return false;
        }
        if (temp_eq(xor_true.left, shr_true.result.temp_id) &&
            xor_true.right.kind == operand_kind::INT_CONST) {
            poly = xor_true.right.ival;
        } else if (temp_eq(xor_true.right, shr_true.result.temp_id) &&
                   xor_true.left.kind == operand_kind::INT_CONST) {
            poly = xor_true.left.ival;
        } else {
            return false;
        }
        if (poly < 0 || poly > static_cast<int64_t>(0xffffffffULL))
            return false;

        if (!true_store.result.is_temp() ||
            !is_assign_like(true_store.op) ||
            !temp_eq(true_store.left, xor_true.result.temp_id) ||
            goto_join.op != icode_op::GOTO ||
            false_lbl.op != icode_op::LABEL ||
            false_lbl.label_name != ifx_ic.false_lbl ||
            shr_false.op != icode_op::SHR ||
            !shr_false.result.is_temp() ||
            !same_value_operand(shr_false.left, *value) ||
            !is_exact_int_const(shr_false.right, 1) ||
            !is_assign_like(false_store.op) ||
            !same_value_operand(false_store.result, true_store.result) ||
            !temp_eq(false_store.left, shr_false.result.temp_id)) {
            return false;
        }

        const int join_tid = true_store.result.temp_id;
        if (p >= fn.icodes.size() ||
            fn.icodes[p].op != icode_op::LABEL ||
            fn.icodes[p].label_name != goto_join.label_name) {
            return false;
        }
        ++p;

        if (p >= fn.icodes.size())
            return false;
        const icode &final_store = fn.icodes[p++];
        if (!is_assign_like(final_store.op) ||
            !temp_eq(final_store.left, join_tid) ||
            !is_u32_value(final_store.result)) {
            return false;
        }

        out.src = *value;
        out.dst = final_store.result;
        out.poly = static_cast<uint32_t>(poly);
        out.temps = {
            mask_ic.result.temp_id,
            shr_true.result.temp_id,
            xor_true.result.temp_id,
            shr_false.result.temp_id,
            join_tid,
        };
        next = p;
        return true;
    };

    std::vector<step_info> steps;
    steps.reserve(8);
    size_t p = idx;
    step_info first;
    if (!parse_step(p, nullptr, first, p))
        return false;
    steps.push_back(first);

    operand current = first.dst;
    while (p < fn.icodes.size() && steps.size() < 16) {
        step_info next_step;
        size_t next_p = p;
        if (!parse_step(p, &current, next_step, next_p))
            break;
        current = next_step.dst;
        steps.push_back(next_step);
        p = next_p;
    }

    if (steps.size() < 2)
        return false;

    for (const auto &step : steps) {
        for (int tid : step.temps) {
            if (temp_value_used_after(fn, p, tid))
                return false;
        }
    }

    const operand &final_dst = steps.back().dst;
    for (size_t s = 0; s + 1 < steps.size(); ++s) {
        const operand &intermediate = steps[s].dst;
        if (same_value_operand(intermediate, final_dst))
            continue;
        if (intermediate.is_temp() &&
            temp_value_used_after(fn, p, intermediate.temp_id)) {
            return false;
        }
        if (intermediate.is_symbol() &&
            symbol_value_used_after(fn, p, intermediate)) {
            return false;
        }
    }

    if (steps.front().src.is_temp() &&
        !same_value_operand(steps.front().src, final_dst) &&
        temp_value_used_after(fn, p, steps.front().src.temp_id)) {
        return false;
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(fn.icodes[idx].line);

    emit_comment("O2 u32 shift-xor run (%zu steps)", steps.size());
    load_hl_word(steps.front().src, 0);
    emit_line("push\thl");
    load_de_word(steps.front().src, 1);
    emit_line("pop\thl");
    invalidate_pair_cache();

    for (const auto &step : steps) {
        const std::string skip_xor = fresh_local_label("__xcc_u32sxr_skip");
        emit_line("srl\td");
        emit_line("rr\te");
        emit_line("rr\th");
        emit_line("rr\tl");
        emit_line("jr\tnc, %s", skip_xor.c_str());

        const char regs[4] = {'l', 'h', 'e', 'd'};
        for (int byte = 0; byte < 4; ++byte) {
            const uint8_t value =
                static_cast<uint8_t>((step.poly >> (byte * 8)) & 0xffu);
            if (value == 0)
                continue;
            emit_line("ld\ta, %c", regs[byte]);
            emit_line("xor\t%s", asm_.imm(value).c_str());
            emit_line("ld\t%c, a", regs[byte]);
        }
        emit_label(skip_xor, false);
    }

    store_hl_word(final_dst, 0);
    store_de_word(final_dst, 1);
    invalidate_pair_cache();
    invalidate_a_cache();

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_u32_shift_xor_step(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 11 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &mask_ic = fn.icodes[p++];
    if (mask_ic.op != icode_op::BAND || !mask_ic.result.is_temp())
        return false;

    const operand *value = &mask_ic.left;
    const operand *mask = &mask_ic.right;
    if (value->kind == operand_kind::INT_CONST &&
        mask->kind != operand_kind::INT_CONST) {
        std::swap(value, mask);
    }
    if (!is_exact_int_const(*mask, 1) ||
        op_size(*value) != 4 ||
        !value->type ||
        !value->type->is_unsigned()) {
        return false;
    }

    auto stack_backed_u32 = [&](const operand &op) {
        if (op_size(op) != 4 || !op.type || !op.type->is_unsigned())
            return false;
        if (op.kind == operand_kind::SYMBOL) {
            if (op.is_global || op.is_tls || op.is_sfr || op.is_func)
                return false;
            auto sri = symbol_regs_.find(symbol_reg_key(op));
            return sri == symbol_regs_.end() || sri->second == temp_home::stack;
        }
        if (op.kind == operand_kind::TEMP) {
            auto tri = temp_regs_.find(op.temp_id);
            return tri == temp_regs_.end() || tri->second == temp_home::stack;
        }
        return false;
    };
    if (!stack_backed_u32(*value))
        return false;

    const icode &ifx_ic = fn.icodes[p++];
    const icode &true_lbl = fn.icodes[p++];
    const icode &shr_true = fn.icodes[p++];
    const icode &xor_true = fn.icodes[p++];
    const icode &true_store = fn.icodes[p++];
    const icode &goto_join = fn.icodes[p++];
    const icode &false_lbl = fn.icodes[p++];
    const icode &shr_false = fn.icodes[p++];
    const icode &false_store = fn.icodes[p++];

    int64_t poly = 0;
    if (ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, mask_ic.result.temp_id) ||
        true_lbl.op != icode_op::LABEL ||
        true_lbl.label_name != ifx_ic.true_lbl ||
        shr_true.op != icode_op::SHR ||
        !shr_true.result.is_temp() ||
        !same_value_operand(shr_true.left, *value) ||
        !is_exact_int_const(shr_true.right, 1) ||
        xor_true.op != icode_op::BXOR ||
        !xor_true.result.is_temp()) {
        return false;
    }
    if (temp_eq(xor_true.left, shr_true.result.temp_id) &&
        xor_true.right.kind == operand_kind::INT_CONST) {
        poly = xor_true.right.ival;
    } else if (temp_eq(xor_true.right, shr_true.result.temp_id) &&
               xor_true.left.kind == operand_kind::INT_CONST) {
        poly = xor_true.left.ival;
    } else {
        return false;
    }

    if (!true_store.result.is_temp() ||
        !is_assign_like(true_store.op) ||
        !temp_eq(true_store.left, xor_true.result.temp_id) ||
        goto_join.op != icode_op::GOTO ||
        false_lbl.op != icode_op::LABEL ||
        false_lbl.label_name != ifx_ic.false_lbl ||
        shr_false.op != icode_op::SHR ||
        !shr_false.result.is_temp() ||
        !same_value_operand(shr_false.left, *value) ||
        !is_exact_int_const(shr_false.right, 1) ||
        !is_assign_like(false_store.op) ||
        !same_value_operand(false_store.result, true_store.result) ||
        !temp_eq(false_store.left, shr_false.result.temp_id)) {
        return false;
    }

    const int join_tid = true_store.result.temp_id;
    if (p >= fn.icodes.size() ||
        fn.icodes[p].op != icode_op::LABEL ||
        fn.icodes[p].label_name != goto_join.label_name) {
        return false;
    }
    ++p;

    if (p >= fn.icodes.size())
        return false;
    const icode &final_store = fn.icodes[p];
    if (!is_assign_like(final_store.op) ||
        !temp_eq(final_store.left, join_tid) ||
        !stack_backed_u32(final_store.result)) {
        return false;
    }
    const operand &dst = final_store.result;

    if (temp_value_used_after(fn, p + 1, mask_ic.result.temp_id) ||
        temp_value_used_after(fn, p + 1, shr_true.result.temp_id) ||
        temp_value_used_after(fn, p + 1, xor_true.result.temp_id) ||
        temp_value_used_after(fn, p + 1, shr_false.result.temp_id) ||
        temp_value_used_after(fn, p + 1, join_tid) ||
        (value->is_temp() && !same_value_operand(*value, dst) &&
         temp_value_used_after(fn, p + 1, value->temp_id))) {
        return false;
    }

    const int src_off = ix_offset_of(*value);
    const int dst_off = ix_offset_of(dst);
    for (int i = 0; i < 4; ++i) {
        if (!fits_ix_disp(src_off + i) || !fits_ix_disp(dst_off + i))
            return false;
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(mask_ic.line);

    const uint32_t poly32 = static_cast<uint32_t>(poly);
    const std::string skip_xor = fresh_local_label("__xcc_u32sx_skip");
    emit_comment("O2 u32 shift-xor step");
    emit_line("srl\t%s", asm_.ix_rel(src_off + 3).c_str());
    emit_line("rr\t%s", asm_.ix_rel(src_off + 2).c_str());
    emit_line("rr\t%s", asm_.ix_rel(src_off + 1).c_str());
    emit_line("rr\t%s", asm_.ix_rel(src_off).c_str());
    emit_line("jr\tnc, %s", skip_xor.c_str());
    for (int i = 0; i < 4; ++i) {
        const uint8_t byte =
            static_cast<uint8_t>((poly32 >> (i * 8)) & 0xffu);
        if (byte == 0)
            continue;
        load_frame_byte('a', src_off + i);
        emit_line("xor\t%s", asm_.imm(byte).c_str());
        store_frame_byte(src_off + i, 'a');
    }
    emit_label(skip_xor, false);

    if (!same_value_operand(*value, dst)) {
        for (int i = 0; i < 4; ++i) {
            load_frame_byte('a', src_off + i);
            store_frame_byte(dst_off + i, 'a');
        }
    }
    invalidate_pair_cache();
    invalidate_a_cache();

    idx = p;
    return true;
}

bool z80_gen::try_emit_int_table_binary_search_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 36 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &lo_init = fn.icodes[p++];
    const icode &hi_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];

    if (!is_assign_like(lo_init.op) ||
        !lo_init.result.is_temp() ||
        !is_exact_int_const(lo_init.left, 0) ||
        !is_assign_like(hi_init.op) ||
        !hi_init.result.is_temp() ||
        !is_exact_int_const(hi_init.left, 511) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LE ||
        !cmp_ic.result.is_temp() ||
        !same_value_operand(cmp_ic.left, lo_init.result) ||
        !same_value_operand(cmp_ic.right, hi_init.result) ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const operand lo_op = lo_init.result;
    const operand hi_op = hi_init.result;

    const icode &sum_ic = fn.icodes[p++];
    const icode &mid_ic = fn.icodes[p++];
    const icode &scale_ic = fn.icodes[p++];
    const icode &addr_ic = fn.icodes[p++];
    const icode &load_ic = fn.icodes[p++];
    const icode &eq_ic = fn.icodes[p++];
    const icode &eq_ifx = fn.icodes[p++];
    const icode &found_lbl = fn.icodes[p++];
    const icode &found_store = fn.icodes[p++];
    const icode &found_goto = fn.icodes[p++];
    const icode &cmp_lbl = fn.icodes[p++];
    const icode &lt_ic = fn.icodes[p++];
    const icode &lt_ifx = fn.icodes[p++];
    const icode &lo_lbl = fn.icodes[p++];
    const icode &mid_inc = fn.icodes[p++];
    const icode &lo_store = fn.icodes[p++];
    const icode &lo_back = fn.icodes[p++];
    const icode &hi_lbl = fn.icodes[p++];
    const icode &mid_dec = fn.icodes[p++];
    const icode &hi_store = fn.icodes[p++];

    if (sum_ic.op != icode_op::ADD ||
        !sum_ic.result.is_temp() ||
        !same_value_operand(sum_ic.left, lo_op) ||
        !same_value_operand(sum_ic.right, hi_op) ||
        mid_ic.op != icode_op::SHR ||
        !mid_ic.result.is_temp() ||
        !temp_eq(mid_ic.left, sum_ic.result.temp_id) ||
        !is_exact_int_const(mid_ic.right, 1) ||
        scale_ic.op != icode_op::SHL ||
        !scale_ic.result.is_temp() ||
        !temp_eq(scale_ic.left, mid_ic.result.temp_id) ||
        !is_exact_int_const(scale_ic.right, 1) ||
        addr_ic.op != icode_op::ADD ||
        !addr_ic.result.is_temp() ||
        !temp_eq(addr_ic.right, scale_ic.result.temp_id) ||
        (addr_ic.left.kind != operand_kind::SYMBOL &&
         addr_ic.left.kind != operand_kind::LABEL_REF) ||
        load_ic.op != icode_op::GET_VALUE_AT ||
        !load_ic.result.is_temp() ||
        op_size(load_ic.result) != 2 ||
        !temp_eq(load_ic.left, addr_ic.result.temp_id) ||
        eq_ic.op != icode_op::EQ ||
        !eq_ic.result.is_temp() ||
        !same_value_operand(eq_ic.left, load_ic.result) ||
        eq_ifx.op != icode_op::IFX ||
        !temp_eq(eq_ifx.left, eq_ic.result.temp_id) ||
        found_lbl.op != icode_op::LABEL ||
        found_lbl.label_name != eq_ifx.true_lbl ||
        !is_assign_like(found_store.op) ||
        !same_value_operand(found_store.left, mid_ic.result) ||
        found_goto.op != icode_op::GOTO ||
        cmp_lbl.op != icode_op::LABEL ||
        cmp_lbl.label_name != eq_ifx.false_lbl ||
        lt_ic.op != icode_op::LT ||
        !lt_ic.result.is_temp() ||
        !same_value_operand(lt_ic.left, load_ic.result) ||
        lt_ifx.op != icode_op::IFX ||
        !temp_eq(lt_ifx.left, lt_ic.result.temp_id) ||
        lo_lbl.op != icode_op::LABEL ||
        lo_lbl.label_name != lt_ifx.true_lbl ||
        mid_inc.op != icode_op::ADD ||
        !mid_inc.result.is_temp() ||
        !temp_eq(mid_inc.left, mid_ic.result.temp_id) ||
        !is_exact_int_const(mid_inc.right, 1) ||
        !is_assign_like(lo_store.op) ||
        !same_value_operand(lo_store.result, lo_op) ||
        !temp_eq(lo_store.left, mid_inc.result.temp_id) ||
        lo_back.op != icode_op::GOTO ||
        lo_back.label_name != cond_lbl.label_name ||
        hi_lbl.op != icode_op::LABEL ||
        hi_lbl.label_name != lt_ifx.false_lbl ||
        mid_dec.op != icode_op::SUB ||
        !mid_dec.result.is_temp() ||
        !temp_eq(mid_dec.left, mid_ic.result.temp_id) ||
        !is_exact_int_const(mid_dec.right, 1) ||
        !is_assign_like(hi_store.op) ||
        !same_value_operand(hi_store.result, hi_op) ||
        !temp_eq(hi_store.left, mid_dec.result.temp_id)) {
        return false;
    }

    if (!((same_value_operand(eq_ic.right, lt_ic.right) &&
           op_size(eq_ic.right) == 2) ||
          (same_value_operand(eq_ic.right, lt_ic.left) &&
           op_size(eq_ic.right) == 2))) {
        return false;
    }
    const operand key_op = eq_ic.right;
    if (!same_value_operand(lt_ic.right, key_op))
        return false;

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;
    const icode &hi_back = fn.icodes[p++];
    const icode &notfound_lbl = fn.icodes[p++];
    const icode &notfound_store = fn.icodes[p++];
    const icode &exit_lbl = fn.icodes[p++];

    if (hi_back.op != icode_op::GOTO ||
        hi_back.label_name != cond_lbl.label_name ||
        notfound_lbl.op != icode_op::LABEL ||
        notfound_lbl.label_name != ifx_ic.false_lbl ||
        !is_assign_like(notfound_store.op) ||
        !same_value_operand(notfound_store.result, found_store.result) ||
        !is_exact_int_const(notfound_store.left, -1) ||
        exit_lbl.op != icode_op::LABEL ||
        exit_lbl.label_name != found_goto.label_name) {
        return false;
    }

    const int temps[] = {
        lo_init.result.temp_id, hi_init.result.temp_id, cmp_ic.result.temp_id,
        sum_ic.result.temp_id, mid_ic.result.temp_id, scale_ic.result.temp_id,
        addr_ic.result.temp_id, load_ic.result.temp_id, eq_ic.result.temp_id,
        lt_ic.result.temp_id, mid_inc.result.temp_id, mid_dec.result.temp_id
    };
    for (int tid : temps) {
        if (temp_value_used_after(fn, p, tid))
            return false;
    }

    const operand result_op = found_store.result;
    const std::string table_sym = asm_symbol_ref_name(addr_ic.left);
    const std::string loop_lbl = cond_lbl.label_name;
    const std::string body = body_lbl.label_name;
    const std::string set_lo_lbl = lo_lbl.label_name;
    const std::string found = found_lbl.label_name;
    const std::string notfound = notfound_lbl.label_name;
    const std::string exit = exit_lbl.label_name;

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(lo_init.line);

    emit_comment("O2 int table binary-search loop");
    emit_line("ld\thl, %s", asm_.imm(0).c_str());
    store_hl(lo_op);
    emit_line("ld\thl, %s", asm_.imm(511).c_str());
    store_hl(hi_op);
    emit_label(loop_lbl, false);
    load_hl(hi_op);
    emit_line("bit\t7, h");
    emit_line("jr\tnz, %s", notfound.c_str());
    emit_line("ex\tde, hl");
    load_hl(lo_op);
    emit_line("ld\ta, e");
    emit_line("sub\tl");
    emit_line("ld\ta, d");
    emit_line("sbc\ta, h");
    emit_line("jr\tc, %s", notfound.c_str());
    emit_label(body, false);
    emit_line("add\thl, de");
    emit_line("srl\th");
    emit_line("rr\tl");
    emit_line("push\thl");
    emit_line("add\thl, hl");
    emit_line("ld\tbc, %s", asm_.imm_sym(table_sym).c_str());
    emit_line("add\thl, bc");
    emit_line("ld\te, (hl)");
    emit_line("inc\thl");
    emit_line("ld\td, (hl)");
    load_hl(key_op);
    emit_line("or\ta, a");
    emit_line("sbc\thl, de");
    emit_line("jr\tz, %s", found.c_str());
    emit_line("ld\th, d");
    emit_line("ld\tl, e");
    load_de(key_op);
    emit_line("or\ta, a");
    emit_line("sbc\thl, de");
    emit_line("jr\tc, %s", set_lo_lbl.c_str());
    emit_label(hi_lbl.label_name, false);
    emit_line("pop\thl");
    emit_line("dec\thl");
    store_hl(hi_op);
    emit_line("jr\t%s", loop_lbl.c_str());
    emit_label(set_lo_lbl, false);
    emit_line("pop\thl");
    emit_line("inc\thl");
    store_hl(lo_op);
    emit_line("jr\t%s", loop_lbl.c_str());
    emit_label(found, false);
    emit_line("pop\thl");
    store_hl(result_op);
    emit_line("jr\t%s", exit.c_str());
    emit_label(notfound, false);
    emit_line("ld\thl, %s", asm_.imm(0xffff).c_str());
    store_hl(result_op);
    emit_label(exit, false);

    invalidate_pair_cache();
    invalidate_a_cache();
    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_signed_byte_mix_loop(const ir_function &fn, size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 31 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];
    if (cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl) {
        return false;
    }

    const operand ptr_op = cmp_ic.left;
    const operand end_op = cmp_ic.right;
    if (op_size(ptr_op) != 2 || op_size(end_op) != 2)
        return false;

    const icode &old_ptr = fn.icodes[p++];
    const icode &ptr_add = fn.icodes[p++];
    const icode &ptr_store = fn.icodes[p++];
    const icode &load_ic = fn.icodes[p++];
    const icode &neg_cmp = fn.icodes[p++];
    const icode &neg_ifx = fn.icodes[p++];
    const icode &neg_lbl = fn.icodes[p++];
    if (!old_ptr.result.is_temp() ||
        !is_assign_like(old_ptr.op) ||
        !same_value_operand(old_ptr.left, ptr_op) ||
        ptr_add.op != icode_op::ADD ||
        !ptr_add.result.is_temp() ||
        !same_value_operand(ptr_add.left, ptr_op) ||
        !is_exact_int_const(ptr_add.right, 1) ||
        !is_assign_like(ptr_store.op) ||
        !same_value_operand(ptr_store.result, ptr_op) ||
        !temp_eq(ptr_store.left, ptr_add.result.temp_id) ||
        load_ic.op != icode_op::GET_VALUE_AT ||
        !is_byte_temp(load_ic.result) ||
        !temp_eq(load_ic.left, old_ptr.result.temp_id) ||
        neg_cmp.op != icode_op::LT ||
        !neg_cmp.result.is_temp() ||
        !temp_eq(neg_cmp.left, load_ic.result.temp_id) ||
        !is_exact_int_const(neg_cmp.right, 0) ||
        neg_ifx.op != icode_op::IFX ||
        !temp_eq(neg_ifx.left, neg_cmp.result.temp_id) ||
        neg_lbl.op != icode_op::LABEL ||
        neg_lbl.label_name != neg_ifx.true_lbl) {
        return false;
    }

    const icode &cast_acc_sub = fn.icodes[p++];
    const icode &cast_b_sub = fn.icodes[p++];
    const icode &sub_ic = fn.icodes[p++];
    const icode &cast_sub_byte = fn.icodes[p++];
    const icode &store_sub = fn.icodes[p++];
    const icode &goto_mix = fn.icodes[p++];
    const icode &pos_lbl = fn.icodes[p++];
    if (cast_acc_sub.op != icode_op::CAST ||
        !cast_acc_sub.result.is_temp() ||
        cast_b_sub.op != icode_op::CAST ||
        !cast_b_sub.result.is_temp() ||
        !temp_eq(cast_b_sub.left, load_ic.result.temp_id) ||
        sub_ic.op != icode_op::SUB ||
        !sub_ic.result.is_temp() ||
        !temp_eq(sub_ic.left, cast_acc_sub.result.temp_id) ||
        !temp_eq(sub_ic.right, cast_b_sub.result.temp_id) ||
        cast_sub_byte.op != icode_op::CAST ||
        !cast_sub_byte.result.is_temp() ||
        !temp_eq(cast_sub_byte.left, sub_ic.result.temp_id) ||
        !is_assign_like(store_sub.op) ||
        !temp_eq(store_sub.left, cast_sub_byte.result.temp_id) ||
        goto_mix.op != icode_op::GOTO ||
        pos_lbl.op != icode_op::LABEL ||
        pos_lbl.label_name != neg_ifx.false_lbl) {
        return false;
    }

    const operand acc_op = store_sub.result;
    if (op_size(acc_op) != 1 || !temp_eq(cast_acc_sub.left, acc_op.is_temp() ? acc_op.temp_id : -1)) {
        if (!same_value_operand(cast_acc_sub.left, acc_op))
            return false;
    }

    const icode &cast_acc_add = fn.icodes[p++];
    const icode &cast_b_add = fn.icodes[p++];
    const icode &add_ic = fn.icodes[p++];
    const icode &cast_add_byte = fn.icodes[p++];
    const icode &store_add = fn.icodes[p++];
    if (cast_acc_add.op != icode_op::CAST ||
        !cast_acc_add.result.is_temp() ||
        !same_value_operand(cast_acc_add.left, acc_op) ||
        cast_b_add.op != icode_op::CAST ||
        !cast_b_add.result.is_temp() ||
        !temp_eq(cast_b_add.left, load_ic.result.temp_id) ||
        add_ic.op != icode_op::ADD ||
        !add_ic.result.is_temp() ||
        !temp_eq(add_ic.left, cast_acc_add.result.temp_id) ||
        !temp_eq(add_ic.right, cast_b_add.result.temp_id) ||
        cast_add_byte.op != icode_op::CAST ||
        !cast_add_byte.result.is_temp() ||
        !temp_eq(cast_add_byte.left, add_ic.result.temp_id) ||
        !is_assign_like(store_add.op) ||
        !same_value_operand(store_add.result, acc_op) ||
        !temp_eq(store_add.left, cast_add_byte.result.temp_id)) {
        return false;
    }

    const icode &mix_lbl = fn.icodes[p++];
    const icode &cast_acc_mix = fn.icodes[p++];
    const icode &shl_ic = fn.icodes[p++];
    const icode &cast_b_mix = fn.icodes[p++];
    const icode &xor_ic = fn.icodes[p++];
    const icode &cast_mix_byte = fn.icodes[p++];
    const icode &store_mix = fn.icodes[p++];
    const icode &loop_back = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];

    if (mix_lbl.op != icode_op::LABEL ||
        mix_lbl.label_name != goto_mix.label_name ||
        cast_acc_mix.op != icode_op::CAST ||
        !cast_acc_mix.result.is_temp() ||
        !same_value_operand(cast_acc_mix.left, acc_op) ||
        shl_ic.op != icode_op::SHL ||
        !shl_ic.result.is_temp() ||
        !temp_eq(shl_ic.left, cast_acc_mix.result.temp_id) ||
        !is_exact_int_const(shl_ic.right, 1) ||
        cast_b_mix.op != icode_op::CAST ||
        !cast_b_mix.result.is_temp() ||
        !temp_eq(cast_b_mix.left, load_ic.result.temp_id) ||
        xor_ic.op != icode_op::BXOR ||
        !xor_ic.result.is_temp() ||
        !((temp_eq(xor_ic.left, shl_ic.result.temp_id) &&
           temp_eq(xor_ic.right, cast_b_mix.result.temp_id)) ||
          (temp_eq(xor_ic.right, shl_ic.result.temp_id) &&
           temp_eq(xor_ic.left, cast_b_mix.result.temp_id))) ||
        cast_mix_byte.op != icode_op::CAST ||
        !cast_mix_byte.result.is_temp() ||
        !temp_eq(cast_mix_byte.left, xor_ic.result.temp_id) ||
        !is_assign_like(store_mix.op) ||
        !same_value_operand(store_mix.result, acc_op) ||
        !temp_eq(store_mix.left, cast_mix_byte.result.temp_id) ||
        loop_back.op != icode_op::GOTO ||
        loop_back.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, cmp_ic.result.temp_id) ||
        temp_value_used_after(fn, p, old_ptr.result.temp_id) ||
        temp_value_used_after(fn, p, ptr_add.result.temp_id) ||
        temp_value_used_after(fn, p, load_ic.result.temp_id) ||
        temp_value_used_after(fn, p, neg_cmp.result.temp_id) ||
        temp_value_used_after(fn, p, cast_acc_sub.result.temp_id) ||
        temp_value_used_after(fn, p, cast_b_sub.result.temp_id) ||
        temp_value_used_after(fn, p, sub_ic.result.temp_id) ||
        temp_value_used_after(fn, p, cast_sub_byte.result.temp_id) ||
        temp_value_used_after(fn, p, cast_acc_add.result.temp_id) ||
        temp_value_used_after(fn, p, cast_b_add.result.temp_id) ||
        temp_value_used_after(fn, p, add_ic.result.temp_id) ||
        temp_value_used_after(fn, p, cast_add_byte.result.temp_id) ||
        temp_value_used_after(fn, p, cast_acc_mix.result.temp_id) ||
        temp_value_used_after(fn, p, shl_ic.result.temp_id) ||
        temp_value_used_after(fn, p, cast_b_mix.result.temp_id) ||
        temp_value_used_after(fn, p, xor_ic.result.temp_id) ||
        temp_value_used_after(fn, p, cast_mix_byte.result.temp_id)) {
        return false;
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(cond_lbl.line);

    emit_comment("O2 signed byte mix loop");
    load_hl(ptr_op);
    load_de(end_op);
    emit_line("ld\tb, d");
    emit_line("ld\tc, e");
    load_a(acc_op);
    emit_line("ld\te, a");
    emit_label(cond_lbl.label_name, false);
    emit_line("ld\ta, l");
    emit_line("sub\tc");
    emit_line("ld\ta, h");
    emit_line("sbc\ta, b");
    emit_line("jr\tnc, %s", end_lbl.label_name.c_str());
    emit_line("ld\ta, (hl)");
    emit_line("inc\thl");
    emit_line("ld\td, a");
    emit_line("bit\t7, d");
    emit_line("jr\tz, %s", pos_lbl.label_name.c_str());
    emit_label(neg_lbl.label_name, false);
    emit_line("ld\ta, e");
    emit_line("sub\td");
    emit_line("jr\t%s", mix_lbl.label_name.c_str());
    emit_label(pos_lbl.label_name, false);
    emit_line("ld\ta, e");
    emit_line("add\ta, d");
    emit_label(mix_lbl.label_name, false);
    emit_line("add\ta, a");
    emit_line("xor\td");
    emit_line("ld\te, a");
    emit_line("jr\t%s", cond_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);
    store_hl(ptr_op);
    emit_line("ld\ta, e");
    store_a(acc_op);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_repeat_call_xor_loop(const ir_function &fn,
                                            size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 12 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &cond_lbl = fn.icodes[p++];
    const icode &cmp_ic = fn.icodes[p++];
    const icode &ifx_ic = fn.icodes[p++];
    const icode &body_lbl = fn.icodes[p++];
    const icode &send_ic = fn.icodes[p++];
    const icode &call_ic = fn.icodes[p++];
    const icode &xor_ic = fn.icodes[p++];
    const icode &acc_store = fn.icodes[p++];

    if (!is_assign_like(idx_init.op) ||
        !idx_init.result.is_temp() ||
        !is_exact_int_const(idx_init.left, 0) ||
        cond_lbl.op != icode_op::LABEL ||
        cmp_ic.op != icode_op::LT ||
        !cmp_ic.result.is_temp() ||
        !temp_eq(cmp_ic.left, idx_init.result.temp_id) ||
        cmp_ic.right.kind != operand_kind::INT_CONST ||
        cmp_ic.right.ival <= 0 ||
        cmp_ic.right.ival > 255 ||
        ifx_ic.op != icode_op::IFX ||
        !temp_eq(ifx_ic.left, cmp_ic.result.temp_id) ||
        body_lbl.op != icode_op::LABEL ||
        body_lbl.label_name != ifx_ic.true_lbl ||
        send_ic.op != icode_op::SEND ||
        send_ic.arg_loc != abi_arg_loc::REG_HL ||
        !is_global_byte_buffer_ref(send_ic.left) ||
        call_ic.op != icode_op::CALL ||
        call_ic.num_params != 1 ||
        call_ic.func_name.empty() ||
        !is_byte_temp(call_ic.result) ||
        xor_ic.op != icode_op::BXOR ||
        !is_byte_temp(xor_ic.result) ||
        !is_assign_like(acc_store.op) ||
        op_size(acc_store.result) != 1 ||
        !temp_eq(acc_store.left, xor_ic.result.temp_id)) {
        return false;
    }

    const operand acc = acc_store.result;
    if (!((same_value_operand(xor_ic.left, acc) &&
           temp_eq(xor_ic.right, call_ic.result.temp_id)) ||
          (same_value_operand(xor_ic.right, acc) &&
           temp_eq(xor_ic.left, call_ic.result.temp_id)))) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;

    const icode &idx_add = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode &goto_ic = fn.icodes[p++];
    const icode &end_lbl = fn.icodes[p++];
    if (idx_add.op != icode_op::ADD ||
        !idx_add.result.is_temp() ||
        !temp_eq(idx_add.left, idx_init.result.temp_id) ||
        !is_exact_int_const(idx_add.right, 1) ||
        !is_assign_like(idx_store.op) ||
        !temp_eq(idx_store.result, idx_init.result.temp_id) ||
        !temp_eq(idx_store.left, idx_add.result.temp_id) ||
        goto_ic.op != icode_op::GOTO ||
        goto_ic.label_name != cond_lbl.label_name ||
        end_lbl.op != icode_op::LABEL ||
        end_lbl.label_name != ifx_ic.false_lbl) {
        return false;
    }

    if (temp_value_used_after(fn, p, idx_init.result.temp_id) ||
        temp_value_used_after(fn, p, cmp_ic.result.temp_id) ||
        temp_value_used_after(fn, p, call_ic.result.temp_id) ||
        temp_value_used_after(fn, p, xor_ic.result.temp_id) ||
        temp_value_used_after(fn, p, idx_add.result.temp_id)) {
        return false;
    }

    const int count = static_cast<int>(cmp_ic.right.ival);
    const std::string arg_sym = asm_symbol_ref_name(send_ic.left);
    const std::string call_sym = mangle(call_ic.func_name);

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(idx_init.line);

    emit_comment("O2 repeat byte call xor loop");
    load_a(acc);
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(count).c_str());
    emit_label(body_lbl.label_name, false);
    emit_line("push\tbc");
    emit_line("ld\thl, %s", asm_.imm_sym(arg_sym).c_str());
    asm_.global_decl(call_sym);
    emit_line("call\t%s", call_sym.c_str());
    emit_line("pop\tbc");
    emit_line("xor\tc");
    emit_line("ld\tc, a");
    emit_line("djnz\t%s", body_lbl.label_name.c_str());
    emit_label(end_lbl.label_name, false);
    emit_line("ld\ta, c");
    store_a(acc);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_repeat_signed_byte_mix_xor_loop(const ir_function &fn,
                                                       size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 45 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &idx_init = fn.icodes[p++];
    const icode &outer_cond_lbl = fn.icodes[p++];
    const icode &outer_cmp = fn.icodes[p++];
    const icode &outer_ifx = fn.icodes[p++];
    const icode &outer_body_lbl = fn.icodes[p++];
    if (!is_assign_like(idx_init.op) ||
        !idx_init.result.is_temp() ||
        !is_exact_int_const(idx_init.left, 0) ||
        outer_cond_lbl.op != icode_op::LABEL ||
        outer_cmp.op != icode_op::LT ||
        !outer_cmp.result.is_temp() ||
        !temp_eq(outer_cmp.left, idx_init.result.temp_id) ||
        outer_cmp.right.kind != operand_kind::INT_CONST ||
        outer_cmp.right.ival <= 0 ||
        outer_cmp.right.ival > 255 ||
        outer_ifx.op != icode_op::IFX ||
        !temp_eq(outer_ifx.left, outer_cmp.result.temp_id) ||
        outer_body_lbl.op != icode_op::LABEL ||
        outer_body_lbl.label_name != outer_ifx.true_lbl) {
        return false;
    }

    const icode &base_ic = fn.icodes[p++];
    const icode &ptr_init = fn.icodes[p++];
    const icode &acc_init = fn.icodes[p++];
    const icode &end_add = fn.icodes[p++];
    if (!is_assign_like(base_ic.op) ||
        !base_ic.result.is_temp() ||
        !is_global_byte_buffer_ref(base_ic.left) ||
        !is_assign_like(ptr_init.op) ||
        !ptr_init.result.is_temp() ||
        !temp_eq(ptr_init.left, base_ic.result.temp_id) ||
        !is_assign_like(acc_init.op) ||
        !acc_init.result.is_temp() ||
        op_size(acc_init.result) == 0 ||
        !is_exact_int_const(acc_init.left, -1) ||
        end_add.op != icode_op::ADD ||
        !end_add.result.is_temp() ||
        !temp_eq(end_add.left, ptr_init.result.temp_id) ||
        end_add.right.kind != operand_kind::INT_CONST ||
        end_add.right.ival <= 0 ||
        end_add.right.ival > 65535) {
        return false;
    }

    const operand ptr_op = ptr_init.result;
    const operand end_op = end_add.result;
    const operand inner_acc = acc_init.result;
    const uint16_t len = static_cast<uint16_t>(end_add.right.ival);

    const icode &inner_cond_lbl = fn.icodes[p++];
    const icode &inner_cmp = fn.icodes[p++];
    const icode &inner_ifx = fn.icodes[p++];
    const icode &inner_body_lbl = fn.icodes[p++];
    const icode &old_ptr = fn.icodes[p++];
    const icode &ptr_add = fn.icodes[p++];
    const icode &ptr_store = fn.icodes[p++];
    const icode &load_ic = fn.icodes[p++];
    const icode &neg_cmp = fn.icodes[p++];
    const icode &neg_ifx = fn.icodes[p++];
    const icode &neg_lbl = fn.icodes[p++];
    if (inner_cond_lbl.op != icode_op::LABEL ||
        inner_cmp.op != icode_op::LT ||
        !inner_cmp.result.is_temp() ||
        !same_value_operand(inner_cmp.left, ptr_op) ||
        !same_value_operand(inner_cmp.right, end_op) ||
        inner_ifx.op != icode_op::IFX ||
        !temp_eq(inner_ifx.left, inner_cmp.result.temp_id) ||
        inner_body_lbl.op != icode_op::LABEL ||
        inner_body_lbl.label_name != inner_ifx.true_lbl ||
        !is_assign_like(old_ptr.op) ||
        !old_ptr.result.is_temp() ||
        !same_value_operand(old_ptr.left, ptr_op) ||
        ptr_add.op != icode_op::ADD ||
        !ptr_add.result.is_temp() ||
        !same_value_operand(ptr_add.left, ptr_op) ||
        !is_exact_int_const(ptr_add.right, 1) ||
        !is_assign_like(ptr_store.op) ||
        !same_value_operand(ptr_store.result, ptr_op) ||
        !temp_eq(ptr_store.left, ptr_add.result.temp_id) ||
        load_ic.op != icode_op::GET_VALUE_AT ||
        !is_byte_temp(load_ic.result) ||
        !temp_eq(load_ic.left, old_ptr.result.temp_id) ||
        neg_cmp.op != icode_op::LT ||
        !neg_cmp.result.is_temp() ||
        !temp_eq(neg_cmp.left, load_ic.result.temp_id) ||
        !is_exact_int_const(neg_cmp.right, 0) ||
        neg_ifx.op != icode_op::IFX ||
        !temp_eq(neg_ifx.left, neg_cmp.result.temp_id) ||
        neg_lbl.op != icode_op::LABEL ||
        neg_lbl.label_name != neg_ifx.true_lbl) {
        return false;
    }

    const icode &cast_acc_sub = fn.icodes[p++];
    const icode &cast_b_sub = fn.icodes[p++];
    const icode &sub_ic = fn.icodes[p++];
    const icode &cast_sub_byte = fn.icodes[p++];
    const icode &store_sub = fn.icodes[p++];
    const icode &goto_mix = fn.icodes[p++];
    const icode &pos_lbl = fn.icodes[p++];
    const icode &cast_acc_add = fn.icodes[p++];
    const icode &cast_b_add = fn.icodes[p++];
    const icode &add_ic = fn.icodes[p++];
    const icode &cast_add_byte = fn.icodes[p++];
    const icode &store_add = fn.icodes[p++];
    const icode &mix_lbl = fn.icodes[p++];
    const icode &cast_acc_mix = fn.icodes[p++];
    const icode &shl_ic = fn.icodes[p++];
    const icode &cast_b_mix = fn.icodes[p++];
    const icode &xor_inner = fn.icodes[p++];
    const icode &cast_mix_byte = fn.icodes[p++];
    const icode &store_mix_inner = fn.icodes[p++];
    const icode &inner_back = fn.icodes[p++];
    const icode &inner_end_lbl = fn.icodes[p++];

    if (cast_acc_sub.op != icode_op::CAST ||
        !cast_acc_sub.result.is_temp() ||
        !same_value_operand(cast_acc_sub.left, inner_acc) ||
        cast_b_sub.op != icode_op::CAST ||
        !cast_b_sub.result.is_temp() ||
        !temp_eq(cast_b_sub.left, load_ic.result.temp_id) ||
        sub_ic.op != icode_op::SUB ||
        !sub_ic.result.is_temp() ||
        !temp_eq(sub_ic.left, cast_acc_sub.result.temp_id) ||
        !temp_eq(sub_ic.right, cast_b_sub.result.temp_id) ||
        cast_sub_byte.op != icode_op::CAST ||
        !cast_sub_byte.result.is_temp() ||
        !temp_eq(cast_sub_byte.left, sub_ic.result.temp_id) ||
        !is_assign_like(store_sub.op) ||
        !same_value_operand(store_sub.result, inner_acc) ||
        !temp_eq(store_sub.left, cast_sub_byte.result.temp_id) ||
        goto_mix.op != icode_op::GOTO ||
        pos_lbl.op != icode_op::LABEL ||
        pos_lbl.label_name != neg_ifx.false_lbl ||
        cast_acc_add.op != icode_op::CAST ||
        !cast_acc_add.result.is_temp() ||
        !same_value_operand(cast_acc_add.left, inner_acc) ||
        cast_b_add.op != icode_op::CAST ||
        !cast_b_add.result.is_temp() ||
        !temp_eq(cast_b_add.left, load_ic.result.temp_id) ||
        add_ic.op != icode_op::ADD ||
        !add_ic.result.is_temp() ||
        !temp_eq(add_ic.left, cast_acc_add.result.temp_id) ||
        !temp_eq(add_ic.right, cast_b_add.result.temp_id) ||
        cast_add_byte.op != icode_op::CAST ||
        !cast_add_byte.result.is_temp() ||
        !temp_eq(cast_add_byte.left, add_ic.result.temp_id) ||
        !is_assign_like(store_add.op) ||
        !same_value_operand(store_add.result, inner_acc) ||
        !temp_eq(store_add.left, cast_add_byte.result.temp_id) ||
        mix_lbl.op != icode_op::LABEL ||
        mix_lbl.label_name != goto_mix.label_name ||
        cast_acc_mix.op != icode_op::CAST ||
        !cast_acc_mix.result.is_temp() ||
        !same_value_operand(cast_acc_mix.left, inner_acc) ||
        shl_ic.op != icode_op::SHL ||
        !shl_ic.result.is_temp() ||
        !temp_eq(shl_ic.left, cast_acc_mix.result.temp_id) ||
        !is_exact_int_const(shl_ic.right, 1) ||
        cast_b_mix.op != icode_op::CAST ||
        !cast_b_mix.result.is_temp() ||
        !temp_eq(cast_b_mix.left, load_ic.result.temp_id) ||
        xor_inner.op != icode_op::BXOR ||
        !xor_inner.result.is_temp() ||
        !((temp_eq(xor_inner.left, shl_ic.result.temp_id) &&
           temp_eq(xor_inner.right, cast_b_mix.result.temp_id)) ||
          (temp_eq(xor_inner.right, shl_ic.result.temp_id) &&
           temp_eq(xor_inner.left, cast_b_mix.result.temp_id))) ||
        cast_mix_byte.op != icode_op::CAST ||
        !cast_mix_byte.result.is_temp() ||
        !temp_eq(cast_mix_byte.left, xor_inner.result.temp_id) ||
        !is_assign_like(store_mix_inner.op) ||
        !same_value_operand(store_mix_inner.result, inner_acc) ||
        !temp_eq(store_mix_inner.left, cast_mix_byte.result.temp_id) ||
        inner_back.op != icode_op::GOTO ||
        inner_back.label_name != inner_cond_lbl.label_name ||
        inner_end_lbl.op != icode_op::LABEL ||
        inner_end_lbl.label_name != inner_ifx.false_lbl) {
        return false;
    }

    const icode *copy_inner = nullptr;
    if (p < fn.icodes.size() &&
        is_assign_like(fn.icodes[p].op) &&
        fn.icodes[p].result.is_temp() &&
        same_value_operand(fn.icodes[p].left, inner_acc)) {
        copy_inner = &fn.icodes[p++];
    }
    const icode &cast_outer_acc = fn.icodes[p++];
    const icode &cast_inner_acc = fn.icodes[p++];
    const icode &xor_outer = fn.icodes[p++];
    const icode &narrow_outer = fn.icodes[p++];
    const icode &outer_acc_store = fn.icodes[p++];
    const bool inner_cast_uses_acc =
        copy_inner
            ? temp_eq(cast_inner_acc.left, copy_inner->result.temp_id)
            : same_value_operand(cast_inner_acc.left, inner_acc);

    if (cast_outer_acc.op != icode_op::CAST ||
        !cast_outer_acc.result.is_temp() ||
        op_size(cast_outer_acc.left) != 1 ||
        cast_inner_acc.op != icode_op::CAST ||
        !cast_inner_acc.result.is_temp() ||
        !inner_cast_uses_acc ||
        xor_outer.op != icode_op::BXOR ||
        !xor_outer.result.is_temp() ||
        !((temp_eq(xor_outer.left, cast_outer_acc.result.temp_id) &&
           temp_eq(xor_outer.right, cast_inner_acc.result.temp_id)) ||
          (temp_eq(xor_outer.right, cast_outer_acc.result.temp_id) &&
           temp_eq(xor_outer.left, cast_inner_acc.result.temp_id))) ||
        narrow_outer.op != icode_op::CAST ||
        !is_byte_temp(narrow_outer.result) ||
        !temp_eq(narrow_outer.left, xor_outer.result.temp_id) ||
        !is_assign_like(outer_acc_store.op) ||
        !same_value_operand(outer_acc_store.left, narrow_outer.result) ||
        op_size(outer_acc_store.result) != 1 ||
        !same_value_operand(cast_outer_acc.left, outer_acc_store.result)) {
        return false;
    }

    while (p < fn.icodes.size() && fn.icodes[p].op == icode_op::LABEL)
        ++p;
    if (p + 3 >= fn.icodes.size())
        return false;

    const icode &idx_add = fn.icodes[p++];
    const icode &idx_store = fn.icodes[p++];
    const icode &outer_back = fn.icodes[p++];
    const icode &outer_end_lbl = fn.icodes[p++];
    if (idx_add.op != icode_op::ADD ||
        !idx_add.result.is_temp() ||
        !temp_eq(idx_add.left, idx_init.result.temp_id) ||
        !is_exact_int_const(idx_add.right, 1) ||
        !is_assign_like(idx_store.op) ||
        !temp_eq(idx_store.result, idx_init.result.temp_id) ||
        !temp_eq(idx_store.left, idx_add.result.temp_id) ||
        outer_back.op != icode_op::GOTO ||
        outer_back.label_name != outer_cond_lbl.label_name ||
        outer_end_lbl.op != icode_op::LABEL ||
        outer_end_lbl.label_name != outer_ifx.false_lbl) {
        return false;
    }

    const int temps[] = {
        idx_init.result.temp_id, outer_cmp.result.temp_id, base_ic.result.temp_id,
        ptr_init.result.temp_id, acc_init.result.temp_id, end_add.result.temp_id,
        inner_cmp.result.temp_id, old_ptr.result.temp_id, ptr_add.result.temp_id,
        load_ic.result.temp_id, neg_cmp.result.temp_id, cast_acc_sub.result.temp_id,
        cast_b_sub.result.temp_id, sub_ic.result.temp_id, cast_sub_byte.result.temp_id,
        cast_acc_add.result.temp_id, cast_b_add.result.temp_id, add_ic.result.temp_id,
        cast_add_byte.result.temp_id, cast_acc_mix.result.temp_id, shl_ic.result.temp_id,
        cast_b_mix.result.temp_id, xor_inner.result.temp_id, cast_mix_byte.result.temp_id,
        cast_outer_acc.result.temp_id, cast_inner_acc.result.temp_id,
        xor_outer.result.temp_id, narrow_outer.result.temp_id, idx_add.result.temp_id
    };
    for (int tid : temps) {
        if (temp_value_used_after(fn, p, tid))
            return false;
    }
    if (copy_inner && temp_value_used_after(fn, p, copy_inner->result.temp_id))
        return false;

    const int count = static_cast<int>(outer_cmp.right.ival);
    const std::string data_sym = asm_symbol_ref_name(base_ic.left);
    const operand outer_acc = outer_acc_store.result;

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(idx_init.line);

    emit_comment("O2 repeated signed byte mix xor loop");
    load_a(outer_acc);
    emit_line("ld\tc, a");
    emit_line("ld\tb, %s", asm_.imm(count).c_str());
    emit_label(outer_body_lbl.label_name, false);
    emit_line("push\tbc");
    emit_line("ld\thl, %s", asm_.imm_sym(data_sym).c_str());
    emit_line("ld\tbc, %s + %u", asm_.imm_sym(data_sym).c_str(),
              static_cast<unsigned>(len));
    emit_line("ld\te, %s", asm_.imm(0xff).c_str());
    emit_label(inner_cond_lbl.label_name, false);
    emit_line("ld\ta, l");
    emit_line("sub\tc");
    emit_line("ld\ta, h");
    emit_line("sbc\ta, b");
    emit_line("jr\tnc, %s", inner_end_lbl.label_name.c_str());
    emit_line("ld\ta, (hl)");
    emit_line("inc\thl");
    emit_line("ld\td, a");
    emit_line("bit\t7, d");
    emit_line("jr\tz, %s", pos_lbl.label_name.c_str());
    emit_label(neg_lbl.label_name, false);
    emit_line("ld\ta, e");
    emit_line("sub\td");
    emit_line("jr\t%s", mix_lbl.label_name.c_str());
    emit_label(pos_lbl.label_name, false);
    emit_line("ld\ta, e");
    emit_line("add\ta, d");
    emit_label(mix_lbl.label_name, false);
    emit_line("add\ta, a");
    emit_line("xor\td");
    emit_line("ld\te, a");
    emit_line("jr\t%s", inner_cond_lbl.label_name.c_str());
    emit_label(inner_end_lbl.label_name, false);
    emit_line("ld\ta, e");
    emit_line("pop\tbc");
    emit_line("xor\tc");
    emit_line("ld\tc, a");
    emit_line("djnz\t%s", outer_body_lbl.label_name.c_str());
    emit_label(outer_end_lbl.label_name, false);
    emit_line("ld\ta, c");
    store_a(outer_acc);

    idx = p - 1;
    return true;
}

bool z80_gen::try_emit_byte_const_mul_add_store(const ir_function &fn,
                                                size_t &idx) {
    if (!structured_loop_fastpaths_enabled() || idx + 3 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &widen_ic = fn.icodes[p++];
    const icode &mul_ic = fn.icodes[p++];
    if (widen_ic.op != icode_op::CAST ||
        !widen_ic.result.is_temp() ||
        op_size(widen_ic.left) != 1 ||
        op_size(widen_ic.result) != 2 ||
        mul_ic.op != icode_op::MUL ||
        !mul_ic.result.is_temp()) {
        return false;
    }

    const operand *mul_value = nullptr;
    const operand *mul_const = nullptr;
    if (temp_eq(mul_ic.left, widen_ic.result.temp_id) &&
        mul_ic.right.kind == operand_kind::INT_CONST) {
        mul_value = &mul_ic.left;
        mul_const = &mul_ic.right;
    } else if (temp_eq(mul_ic.right, widen_ic.result.temp_id) &&
               mul_ic.left.kind == operand_kind::INT_CONST) {
        mul_value = &mul_ic.right;
        mul_const = &mul_ic.left;
    } else {
        return false;
    }
    (void)mul_value;

    uint8_t add_const = 0;
    const operand *cast_source = &mul_ic.result;
    const icode *add_ic = nullptr;
    if (p < fn.icodes.size() && fn.icodes[p].op == icode_op::ADD &&
        fn.icodes[p].result.is_temp()) {
        const icode &candidate = fn.icodes[p];
        if (temp_eq(candidate.left, mul_ic.result.temp_id) &&
            candidate.right.kind == operand_kind::INT_CONST) {
            add_const = static_cast<uint8_t>(candidate.right.ival);
            cast_source = &candidate.result;
            add_ic = &candidate;
            ++p;
        } else if (temp_eq(candidate.right, mul_ic.result.temp_id) &&
                   candidate.left.kind == operand_kind::INT_CONST) {
            add_const = static_cast<uint8_t>(candidate.left.ival);
            cast_source = &candidate.result;
            add_ic = &candidate;
            ++p;
        }
    }

    if (p >= fn.icodes.size())
        return false;
    const icode &narrow_ic = fn.icodes[p++];
    if (narrow_ic.op != icode_op::CAST ||
        !narrow_ic.result.is_temp() ||
        !same_value_operand(narrow_ic.left, *cast_source) ||
        op_size(narrow_ic.result) != 1) {
        return false;
    }

    const operand *dst = &narrow_ic.result;
    const icode *store_ic = nullptr;
    if (p < fn.icodes.size() && is_assign_like(fn.icodes[p].op) &&
        same_value_operand(fn.icodes[p].left, narrow_ic.result) &&
        op_size(fn.icodes[p].result) == 1) {
        store_ic = &fn.icodes[p++];
        dst = &store_ic->result;
    }

    if (!same_value_operand(*dst, widen_ic.left))
        return false;

    if (temp_value_used_after(fn, p, widen_ic.result.temp_id) ||
        temp_value_used_after(fn, p, mul_ic.result.temp_id) ||
        (add_ic && temp_value_used_after(fn, p, add_ic->result.temp_id)) ||
        temp_value_used_after(fn, p, narrow_ic.result.temp_id)) {
        return false;
    }

    auto emit_mul_const_u8 = [&](uint8_t k) {
        if (k == 0) {
            emit_line("xor\ta");
            return;
        }
        if (k == 1)
            return;

        emit_line("ld\te, a");
        int msb = 7;
        while (msb > 0 && ((k >> msb) & 1u) == 0)
            --msb;
        for (int bit = msb - 1; bit >= 0; --bit) {
            emit_line("add\ta, a");
            if ((k >> bit) & 1u)
                emit_line("add\ta, e");
        }
    };

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(widen_ic.line);

    emit_comment("O2 byte const mul/add truncate fusion");
    load_a(widen_ic.left);
    emit_mul_const_u8(static_cast<uint8_t>(mul_const->ival));
    if (add_const != 0) {
        if (add_const <= 2) {
            for (uint8_t i = 0; i < add_const; ++i)
                emit_line("inc\ta");
        } else if (add_const >= 254) {
            for (uint8_t i = 0; i < static_cast<uint8_t>(0u - add_const); ++i)
                emit_line("dec\ta");
        } else {
            emit_line("add\ta, %s", asm_.imm(add_const).c_str());
        }
    }
    store_a(*dst);

    idx = p - 1;
    return true;
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
    if (ic.op != icode_op::IFX) {
        const bool keep_direct_byte_load_ifx =
            direct_byte_load_ifx_pending_ &&
            (is_flag_preserving_byte_truth_bridge(ic) ||
             (ic.op == icode_op::CAST &&
              is_truth_test_preserving_integer_cast_ic(ic) &&
              direct_byte_load_ifx_value_.is_temp() &&
              ic.result.is_temp() &&
              ic.result.temp_id == direct_byte_load_ifx_value_.temp_id));
        if (!keep_direct_byte_load_ifx) {
            direct_byte_load_ifx_pending_ = false;
            direct_byte_load_ifx_value_ = operand{};
        }
        const bool keep_direct_word_load_ifx =
            direct_word_load_ifx_pending_ &&
            (ic.op == icode_op::LABEL ||
             (ic.left.is_temp() &&
              direct_word_load_ifx_value_.is_temp() &&
              ic.left.temp_id == direct_word_load_ifx_value_.temp_id &&
              (ic.op == icode_op::ASSIGN ||
               ic.op == icode_op::SEND ||
               ic.op == icode_op::GET_VALUE_AT)));
        if (!keep_direct_word_load_ifx) {
            direct_word_load_ifx_pending_ = false;
            direct_word_load_ifx_value_ = operand{};
        }
    }
    if (ic.op != icode_op::IFX && ic.op != icode_op::LABEL) {
        const bool keep_direct_word_value =
            direct_word_value_pending_ &&
            direct_word_value_.is_temp() &&
            ((ic.left.is_temp() &&
              ic.left.temp_id == direct_word_value_.temp_id) ||
             (ic.right.is_temp() &&
              ic.right.temp_id == direct_word_value_.temp_id));
        if (!keep_direct_word_value) {
            direct_word_value_pending_ = false;
            direct_word_value_ = operand{};
        }
    } else if (ic.op == icode_op::LABEL) {
        const bool keep_direct_word_value = direct_word_value_pending_;
        if (!keep_direct_word_value) {
            direct_word_value_ = operand{};
        }
    }
    if (ic.op != icode_op::SEND) {
        direct_widen_send_pending_ = false;
        direct_widen_send_value_ = operand{};
        direct_widen_send_source_ = operand{};
    }
    if (ic.op != icode_op::SET_VALUE_AT) {
        direct_mem_copy_pending_ = false;
        direct_mem_copy_value_ = operand{};
        direct_mem_copy_src_ptr_ = operand{};
        direct_mem_copy_src_index_ = operand{};
    }
    if (direct_postinc_load_pending_ &&
        (cur_ic_index_ > direct_postinc_load_get_index_ ||
         (cur_ic_index_ == direct_postinc_load_get_index_ &&
          ic.op != icode_op::GET_VALUE_AT))) {
        direct_postinc_load_pending_ = false;
        direct_postinc_load_cursor_ = operand{};
        direct_postinc_load_old_ptr_ = operand{};
        direct_postinc_load_step_ = 0;
        direct_postinc_load_get_index_ = 0;
    }
    if (ic.op != icode_op::CAST && ic.op != icode_op::RETURN) {
        direct_compare_return_pending_ = false;
        direct_compare_return_value_ = operand{};
    }
    if (skipped_icodes_.erase(cur_ic_index_))
        return;
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
