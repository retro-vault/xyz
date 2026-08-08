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
    // These namespaces are reserved for labels created by xcc/xopt and
    // already live in assembler symbol space. Ordinary C identifiers,
    // including names beginning with two underscores, still receive the
    // target's leading C-symbol underscore.
    return name.rfind("__xcc_", 0) == 0 ||
           name.rfind("__xopt_", 0) == 0;
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
    // Very large straight-line functions can make the speed profile exceed
    // the Z80's 64 KiB address space. Retain the speed-profile IR but select
    // compact backend forms for such functions so generated programs remain
    // viable.
    // Keep the threshold below the point where a successful IR combine can
    // accidentally switch a still-large straight-line function back to the
    // dedicated-slot speed backend.  A 32-bit rotate combine, for example,
    // removes many IR shifts but the remaining hash round still benefits from
    // liveness-coloured spill slots and compact address forms.
    compact_codegen_ =
        opt_settings_.level != opt_level::Os && fn.icodes.size() > 500;
    local_bytes_    = fn.local_bytes;
    fn_end_lbl_     = "__" + fn.name + "_end";
    temp_slots_.clear();
    temp_regs_.clear();
    incoming_symbol_homes_.clear();
    symbol_regs_.clear();
    iy_preserved_call_indices_.clear();
    bc_preserved_call_indices_.clear();
    bounded_induction_limits_.clear();
    bounded_symbol_induction_comparisons_.clear();
    bounded_symbol_induction_increments_.clear();
    jump_table_selector_loads_.clear();
    iy_u32_remat_offsets_.clear();
    next_temp_slot_ = 0;
    temp_stack_bytes_ = 0;
    temp_frame_bytes_ = 0;
    reserving_prologue_spills_ = false;
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
    sibling_tail_call_pending_ = false;
    sibling_tail_call_value_ = operand{};
    last_frameless_return_terminated_ = false;
    clear_known_sp_ix_delta();
    invalidate_pair_cache();
    invalidate_a_cache();
    if (fn.bank < 0) asm_.section_code();
    else asm_.section_code_named(banked_code_section_name(fn.bank));

    if (regalloc_enabled())
        regalloc_prepass(fn);
    temp_stack_bytes_ = compute_temp_frame_bytes(fn);
    bool auto_temp_frame =
        opt_settings_.level == opt_level::O0 ||
        opt_settings_.level == opt_level::O1 ||
        opt_settings_.level == opt_level::O2 ||
        opt_settings_.level == opt_level::Os || compact_codegen_;
    if (temp_frame_prealloc_enabled() ||
        (auto_temp_frame && temp_stack_bytes_ > 0 && !can_omit_frame_pointer(fn)))
        temp_frame_bytes_ = temp_stack_bytes_;

    // The structured loop/pattern matchers below each scan ahead in the
    // instruction stream (some via temp_value_used_after), so running them at
    // every index is O(n^2).  For pathologically large machine-generated
    // functions (e.g. the C23 translation-limit stress test) none of these
    // small-loop patterns can match, so skip them and emit straight-line code.
    const bool structured_match = fn.icodes.size() <= 1200;

    // The jump-table matcher runs at the first compare, one instruction after
    // a common byte-load selector.  Discover those exact loads up front so
    // store_a can leave the selector in A instead of writing a register home
    // that the jump-table dispatch immediately supersedes.
    if (structured_match && switch_jump_tables_enabled()) {
        for (size_t start = 1; start + 1 < fn.icodes.size(); ++start) {
            std::vector<int64_t> values;
            int selector_tid = -1;
            size_t pos = start;
            while (pos + 1 < fn.icodes.size()) {
                const icode &cmp = fn.icodes[pos];
                const icode &ifx = fn.icodes[pos + 1];
                if (cmp.op != icode_op::EQ ||
                    ifx.op != icode_op::IFX ||
                    ifx.true_lbl.empty() || !ifx.false_lbl.empty() ||
                    !cmp.result.is_temp() || !ifx.left.is_temp() ||
                    cmp.result.temp_id != ifx.left.temp_id)
                    break;
                const operand *selector = &cmp.left;
                const operand *constant = &cmp.right;
                if (selector->kind == operand_kind::INT_CONST)
                    std::swap(selector, constant);
                if (!selector->is_temp() ||
                    constant->kind != operand_kind::INT_CONST ||
                    op_size(*selector) != 1 ||
                    (selector_tid >= 0 &&
                     selector_tid != selector->temp_id))
                    break;
                selector_tid = selector->temp_id;
                values.push_back(constant->ival);
                pos += 2;
            }
            if (values.size() < 4 || pos >= fn.icodes.size() ||
                fn.icodes[pos].op != icode_op::GOTO)
                continue;
            std::sort(values.begin(), values.end());
            if (values.front() < 0 || values.back() > 255 ||
                std::adjacent_find(values.begin(), values.end()) !=
                    values.end())
                continue;
            const int64_t span = values.back() - values.front() + 1;
            if (span <= 0 || span > 16 ||
                span > static_cast<int64_t>(values.size() * 2))
                continue;
            const icode &load = fn.icodes[start - 1];
            if (load.op == icode_op::GET_VALUE_AT &&
                load.result.is_temp() &&
                load.result.temp_id == selector_tid &&
                op_size(load.result) == 1)
                jump_table_selector_loads_.insert(start - 1);
        }
    }

    for (size_t i = 0; i < fn.icodes.size(); ++i) {
      // A producer-side fusion can mark following icodes as consumed.  Do
      // not let a structured matcher consume one of those same icodes a
      // second time before gen_icode() gets a chance to erase the marker.
      // In particular, GET_VALUE_AT+BAND can otherwise be followed by the
      // BAND+IFX matcher, which tests the unmaterialized load temporary
      // instead of the already-computed mask result.
      if (skipped_icodes_.find(i) != skipped_icodes_.end()) {
          gen_icode(fn.icodes[i]);
          continue;
      }
      if (structured_match) {
        if (try_emit_byte_mask_walk_loop(fn, i))
            continue;
        if (try_emit_byte_copy_walk_loop(fn, i))
            continue;
        if (try_emit_zero_byte_walk_loop(fn, i))
            continue;
        if (try_emit_inplace_byte_step_ifx(fn, i))
            continue;
        if (try_emit_inplace_pointer_update(fn, i))
            continue;
        if (try_emit_scaled_frame_load(fn, i))
            continue;
        if (try_emit_scaled_global_load(fn, i))
            continue;
        if (try_emit_iy_indexed_load(fn, i))
            continue;
        if (try_emit_iy_indexed_store(fn, i))
            continue;
        if (try_emit_postinc_indexed_load(fn, i))
            continue;
        if (try_emit_postinc_indexed_store(fn, i))
            continue;
        if (try_emit_postdec_truth(fn, i))
            continue;
        if (try_emit_shift_xor_self(fn, i))
            continue;
        if (try_emit_msb_byte_shift_xor_diamonds(fn, i))
            continue;
        if (try_emit_shift_add_byte_accumulate(fn, i))
            continue;
        if (try_emit_switch_jump_table(fn, i))
            continue;
        if (try_emit_lsb32_shift_xor_diamond(fn, i))
            continue;
        if (try_emit_band_ifx(fn, i))
            continue;
        if (try_emit_byte_load_compare_ifx(fn, i))
            continue;
        if (try_emit_guarded_zero_arg_indirect_call(fn, i))
            continue;
        if (try_emit_word_select_send(fn, i))
            continue;
        if (try_emit_compare_ifx(fn, i))
            continue;
      }
        gen_icode(fn.icodes[i]);
    }

    cur_fn_ = nullptr;
    compact_codegen_ = false;
}


void z80_gen::emit_prologue(const ir_function &fn) {
    reserving_prologue_spills_ = true;
    cur_convention_->emit_prologue(*this, fn);
    reserving_prologue_spills_ = false;
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
        if (ic.op != icode_op::SET_VALUE_AT &&
            ic.result.is_temp() && ic.result.temp_id == temp_id)
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
            if (ic.op != icode_op::SET_VALUE_AT &&
                ic.result.is_temp() && ic.result.temp_id == temp_id) {
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
            // Recursive widening rematerialization creates a use that is not
            // visible to frame-slot liveness. Require an ordinary IR use at
            // or beyond this point so the source's colored slot/register is
            // still reserved; otherwise load the already-widened value.
            if (!cur_fn_ ||
                !temp_value_used_after(*cur_fn_, cur_ic_index_,
                                       candidate.temp_id)) {
                return false;
            }
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

    if (def->op == icode_op::BAND) {
        const operand *lhs = &def->left;
        const operand *rhs = &def->right;
        if (lhs->kind == operand_kind::INT_CONST)
            std::swap(lhs, rhs);

        if (rhs->kind == operand_kind::INT_CONST) {
            const bool mask_clears_high_byte =
                rhs->ival >= 0 && rhs->ival <= 0xff;
            if (mask_clears_high_byte ||
                get_zero_extended_u8_source(*lhs, src)) {
                src = op;
                src.type = type::make_uchar();
                src.byte_offset = 0;
                return true;
            }
        } else {
            operand lhs_byte;
            operand rhs_byte;
            if (get_zero_extended_u8_source(*lhs, lhs_byte) &&
                get_zero_extended_u8_source(*rhs, rhs_byte)) {
                src = op;
                src.type = type::make_uchar();
                src.byte_offset = 0;
                return true;
            }
        }
    }

    return false;
}

bool z80_gen::get_sign_extended_i8_source(const operand &op, operand &src) const {
    auto temp_has_single_def = [&](int temp_id) {
        if (!cur_fn_)
            return false;
        int defs = 0;
        for (const auto &ic : cur_fn_->icodes) {
            if (ic.op != icode_op::SET_VALUE_AT &&
                ic.result.is_temp() && ic.result.temp_id == temp_id) {
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
            if (!cur_fn_ ||
                !temp_value_used_after(*cur_fn_, cur_ic_index_,
                                       candidate.temp_id)) {
                return false;
            }
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
                    std::string address = mangle(cur_def->left.name);
                    const int total_offset =
                        cur_def->left.byte_offset + cur.byte_offset;
                    if (total_offset > 0)
                        address += " + " + std::to_string(total_offset);
                    else if (total_offset < 0)
                        address += " - " + std::to_string(-total_offset);
                    emit_line("ld\thl, %s",
                              asm_.imm_sym(address).c_str());
                } else {
                    int off = cur_def->left.is_param
                                  ? param_ix_offset(cur_def->left)
                                  : cur_def->left.stack_offset;
                    if (cur_def->left.kind == operand_kind::TEMP)
                        off = ix_offset_of(cur_def->left);
                    else
                        off += cur_def->left.byte_offset;
                    off += cur.byte_offset;
                    load_ix_addr_hl(off);
                }
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
            case icode_op::GET_VALUE_AT: {
                if (!cur_def->result.type || cur_def->result.type->size() != 2)
                    return false;
                if (cur_def->result.type->is_volatile)
                    return false;
                if (!cur_def->left.type || !cur_def->left.type->is_ptr() ||
                    cur_def->left.type->is_far_ptr())
                    return false;
                if (cur_def->left.type &&
                    cur_def->left.type->base &&
                    cur_def->left.type->base->is_volatile) {
                    return false;
                }
                if (!cur_def->right.is_none())
                    return false;
                if (cur.byte_offset < 0 || cur.byte_offset > 1)
                    return false;

                invalidate_pair_cache();
                invalidate_a_cache();
                if (cur_def->left.kind == operand_kind::INT_CONST) {
                    emit_line("ld\thl, %s",
                              asm_.imm(cur_def->left.ival).c_str());
                } else if (cur_def->left.kind == operand_kind::LABEL_REF) {
                    emit_line("ld\thl, %s",
                              asm_.imm_sym(
                                  asm_label_ref_name(cur_def->left.name)).c_str());
                } else if (cur_def->left.is_temp()) {
                    if (!remat(cur_def->left, depth + 1))
                        return false;
                } else {
                    return false;
                }
                emit_line("ld\ta, (hl)");
                emit_line("inc\thl");
                emit_line("ld\th, (hl)");
                emit_line("ld\tl, a");
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

bool z80_gen::symbol_home_in_iy(const operand &op) const {
    if (op.kind != operand_kind::SYMBOL || op.is_global)
        return false;
    auto it = symbol_regs_.find(symbol_reg_key(op));
    return it != symbol_regs_.end() && it->second == temp_home::main_iy;
}

bool z80_gen::operand_home_in_bc(const operand &op) const {
    if (symbol_home_in_bc(op))
        return true;
    if (!op.is_temp())
        return false;
    auto it = temp_regs_.find(op.temp_id);
    return it != temp_regs_.end() && it->second == temp_home::main_bc;
}

void z80_gen::maybe_materialize_incoming_arg_temp(
    const operand &op, bool scan_across_branches) {
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

    bool used_later = false;
    if (scan_across_branches) {
        // A conditional first use can be followed textually by a definition
        // on one arm before another arm uses the incoming value.  The normal
        // linear helper stops at that first definition, even though it does
        // not dominate the other arm.  Conservatively scan all following
        // operand uses for this control-flow consumer.
        for (size_t i = cur_ic_index_ + 1;
             i < cur_fn_->icodes.size() && !used_later; ++i) {
            const icode &ic = cur_fn_->icodes[i];
            used_later = operand_uses_temp(ic.left, op.temp_id) ||
                         operand_uses_temp(ic.right, op.temp_id) ||
                         (ic.op == icode_op::SET_VALUE_AT &&
                          operand_uses_temp(ic.result, op.temp_id));
        }
    } else {
        used_later = temp_value_used_after(
            *cur_fn_, cur_ic_index_ + 1, op.temp_id);
    }
    if (!used_later)
        return;

    // A first consumer may use a byte view of a word argument (for example,
    // the high byte in `value < 0`).  The incoming ABI register still holds
    // the complete value, so materialize it at the base spill slot rather
    // than shifting the word by that consumer's byte offset.
    operand base = op;
    base.byte_offset = 0;
    const int spill_offset = ix_offset_of(base);

    emit_comment("materialize incoming arg temp t%d for later reuse", op.temp_id);
    switch (it->second) {
    case temp_home::arg_a:
        store_frame_byte(spill_offset, 'a');
        break;
    case temp_home::arg_l:
        store_frame_byte(spill_offset, 'l');
        break;
    case temp_home::arg_hl:
        store_frame_word(reg_pair{"hl", 'l', 'h', false}, spill_offset);
        break;
    case temp_home::arg_de:
        store_frame_word(reg_pair{"de", 'e', 'd', true}, spill_offset);
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
    emit_comment("optimized byte mask walk loop (count=%d)", count);
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
    emit_comment("optimized byte copy walk loop (count=%d)", count);
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
    emit_comment("optimized byte zero loop (count=%d)", count);
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

bool z80_gen::try_emit_inplace_byte_step_ifx(const ir_function &fn,
                                             size_t &idx) {
    if (!compare_ifx_fusion_enabled() || idx + 1 >= fn.icodes.size())
        return false;

    const icode &step = fn.icodes[idx];
    const icode &branch = fn.icodes[idx + 1];
    if ((step.op != icode_op::ADD && step.op != icode_op::SUB) ||
        !step.result.is_temp() || !step.left.is_temp() ||
        !operands_equivalent(step.result, step.left) ||
        !step.result.type || step.result.type->size() != 1 ||
        step.right.kind != operand_kind::INT_CONST ||
        step.right.ival != 1 || branch.op != icode_op::IFX ||
        !branch.left.is_temp() ||
        branch.left.temp_id != step.result.temp_id ||
        (branch.true_lbl.empty() && branch.false_lbl.empty()) ||
        temp_value_used_after(fn, idx + 2, step.result.temp_id)) {
        return false;
    }

    std::string target;
    auto home = temp_regs_.find(step.result.temp_id);
    if (home == temp_regs_.end() || home->second == temp_home::stack) {
        const int off = ix_offset_of(step.result);
        if (!fits_ix_disp(off))
            return false;
        target = asm_.ix_rel(off);
    } else {
        switch (home->second) {
        case temp_home::main_a: target = "a"; break;
        case temp_home::main_b: target = "b"; break;
        case temp_home::main_c: target = "c"; break;
        case temp_home::main_d: target = "d"; break;
        case temp_home::main_e: target = "e"; break;
        default: return false;
        }
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(step.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    emit_line(step.op == icode_op::ADD ? "inc\t%s" : "dec\t%s",
              target.c_str());
    if (!branch.true_lbl.empty())
        emit_line("jp\tnz, %s", branch.true_lbl.c_str());
    if (!branch.false_lbl.empty())
        emit_line("jp\t%s", branch.false_lbl.c_str());
    idx += 1;
    return true;
}

bool z80_gen::try_emit_inplace_pointer_update(const ir_function &fn,
                                              size_t &idx) {
    const icode &step_ic = fn.icodes[idx];
    if ((step_ic.op != icode_op::ADD && step_ic.op != icode_op::SUB) ||
        !step_ic.result.is_temp() || !step_ic.left.is_temp() ||
        !step_ic.left.type || step_ic.left.type->size() != 2 ||
        step_ic.right.kind != operand_kind::INT_CONST ||
        step_ic.right.ival == 0 || step_ic.right.ival < -32767 ||
        step_ic.right.ival > 32767) {
        return false;
    }

    const bool direct_update =
        operands_equivalent(step_ic.result, step_ic.left);
    if (!direct_update) {
        if (idx + 1 >= fn.icodes.size())
            return false;
        const icode &commit_ic = fn.icodes[idx + 1];
        if (commit_ic.op != icode_op::ASSIGN ||
            !operands_equivalent(commit_ic.result, step_ic.left) ||
            !temp_eq(commit_ic.left, step_ic.result.temp_id) ||
            temp_value_used_after(fn, idx + 2, step_ic.result.temp_id)) {
            return false;
        }
    }

    auto home_it = temp_regs_.find(step_ic.left.temp_id);
    if (home_it == temp_regs_.end() ||
        (home_it->second != temp_home::main_iy &&
         home_it->second != temp_home::main_bc)) {
        return false;
    }
    if (home_it->second == temp_home::main_iy &&
        !step_ic.left.type->is_ptr()) {
        return false;
    }

    if (debug_)
        debug_->emit_location(step_ic.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    const char *reg = home_it->second == temp_home::main_iy ? "iy" : "bc";
    const int64_t delta = step_ic.op == icode_op::ADD
                              ? step_ic.right.ival
                              : -step_ic.right.ival;
    if (delta == 1 || delta == -1) {
        emit_line(delta > 0 ? "inc\t%s" : "dec\t%s", reg);
    } else if (home_it->second == temp_home::main_iy) {
        emit_line("ld\tde, %s", asm_.imm(delta).c_str());
        emit_line("add\tiy, de");
    } else {
        emit_line("ld\thl, %s", asm_.imm(delta).c_str());
        emit_line("add\thl, bc");
        emit_line("ld\tb, h");
        emit_line("ld\tc, l");
    }
    if (!direct_update)
        idx += 1;
    return true;
}

bool z80_gen::try_emit_iy_indexed_load(const ir_function &fn, size_t &idx) {
    if (idx + 1 >= fn.icodes.size())
        return false;

    const icode &add_ic = fn.icodes[idx];
    const icode &load_ic = fn.icodes[idx + 1];
    if (add_ic.op != icode_op::ADD || !add_ic.result.is_temp() ||
        load_ic.op != icode_op::GET_VALUE_AT ||
        !load_ic.left.is_temp() ||
        load_ic.left.temp_id != add_ic.result.temp_id ||
        !load_ic.right.is_none() ||
        (op_size(load_ic.result) != 1 && op_size(load_ic.result) != 2 &&
         op_size(load_ic.result) != 4)) {
        return false;
    }

    // Leave an adjacent read/modify/write chain to gen_get_value_at(), which
    // can update the addressed memory in place.  Consuming ADD+GET here would
    // hide that stronger fusion and force the updated value through a temp.
    if (idx + 3 < fn.icodes.size()) {
        const icode &update = fn.icodes[idx + 2];
        const icode &store = fn.icodes[idx + 3];
        const bool update_uses_load =
            update.left.is_temp() && load_ic.result.is_temp() &&
            update.left.temp_id == load_ic.result.temp_id;
        const bool stores_update =
            update.result.is_temp() && store.left.is_temp() &&
            store.left.temp_id == update.result.temp_id;
        const bool same_address =
            store.op == icode_op::SET_VALUE_AT && store.result.is_temp() &&
            store.result.temp_id == add_ic.result.temp_id &&
            store.right.is_none();
        if (update_uses_load && stores_update && same_address)
            return false;
    }

    // A derived address may remain live after the first load.  Omit its
    // materialization when every use is a narrow direct dereference; later
    // accesses recover the IY displacement from this IR definition.
    const int address_tid = add_ic.result.temp_id;
    for (size_t k = idx + 1; k < fn.icodes.size(); ++k) {
        const icode &use = fn.icodes[k];
        const bool in_result = use.result.is_temp() &&
                               use.result.temp_id == address_tid;
        const bool in_left = use.left.is_temp() &&
                             use.left.temp_id == address_tid;
        const bool in_right = use.right.is_temp() &&
                              use.right.temp_id == address_tid;
        if (!in_result && !in_left && !in_right)
            continue;
        const bool direct_load =
            use.op == icode_op::GET_VALUE_AT && in_left && !in_result &&
            !in_right && use.right.is_none() &&
            (op_size(use.result) == 1 || op_size(use.result) == 2 ||
             op_size(use.result) == 4);
        const bool direct_store =
            use.op == icode_op::SET_VALUE_AT && in_result && !in_left &&
            !in_right && use.right.is_none() &&
            (op_size(use.left) == 1 || op_size(use.left) == 2);
        if (!direct_load && !direct_store)
            return false;
    }

    const operand *cursor = &add_ic.left;
    const operand *offset = &add_ic.right;
    if (cursor->kind == operand_kind::INT_CONST)
        std::swap(cursor, offset);
    if (!cursor->is_temp() || offset->kind != operand_kind::INT_CONST)
        return false;
    auto home = temp_regs_.find(cursor->temp_id);
    if (home == temp_regs_.end() || home->second != temp_home::main_iy)
        return false;

    const int64_t disp = offset->ival;
    const int size = op_size(load_ic.result);
    if (disp < -128 || disp + size - 1 > 127)
        return false;

    if (debug_)
        debug_->emit_location(load_ic.line ? load_ic.line : add_ic.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    if (size == 4 && idx + 2 < fn.icodes.size()) {
        const icode &consumer = fn.icodes[idx + 2];
        const bool used_by_consumer =
            load_ic.result.is_temp() &&
            ((consumer.left.is_temp() &&
              consumer.left.temp_id == load_ic.result.temp_id) ||
             (consumer.right.is_temp() &&
              consumer.right.temp_id == load_ic.result.temp_id));
        if (consumer.op == icode_op::ADD && op_size(consumer.result) == 4 &&
            used_by_consumer &&
            !temp_value_used_after(fn, idx + 3, load_ic.result.temp_id)) {
            iy_u32_remat_offsets_[load_ic.result.temp_id] = disp;
            idx += 1;
            return true;
        }
    }
    if (size == 1) {
        emit_line("ld\ta, %lld(iy)", static_cast<long long>(disp));
        store_a(load_ic.result);
    } else if (size == 2) {
        emit_line("ld\te, %lld(iy)", static_cast<long long>(disp));
        emit_line("ld\td, %lld(iy)", static_cast<long long>(disp + 1));
        store_de(load_ic.result);
    } else {
        emit_line("ld\te, %lld(iy)", static_cast<long long>(disp));
        emit_line("ld\td, %lld(iy)", static_cast<long long>(disp + 1));
        store_de_word(load_ic.result, 0);
        emit_line("ld\te, %lld(iy)", static_cast<long long>(disp + 2));
        emit_line("ld\td, %lld(iy)", static_cast<long long>(disp + 3));
        store_de_word(load_ic.result, 1);
    }
    idx += 1;
    return true;
}

bool z80_gen::try_emit_scaled_frame_load(const ir_function &fn,
                                         size_t &idx) {
    if (idx + 2 >= fn.icodes.size())
        return false;

    const icode &scale = fn.icodes[idx];
    const icode &address = fn.icodes[idx + 1];
    const icode &load = fn.icodes[idx + 2];
    if (scale.op != icode_op::SHL || !scale.result.is_temp() ||
        scale.right.kind != operand_kind::INT_CONST ||
        scale.right.ival < 1 || scale.right.ival > 7 ||
        op_size(scale.left) != 2 || op_size(scale.result) != 2 ||
        address.op != icode_op::ADD || !address.result.is_temp() ||
        load.op != icode_op::GET_VALUE_AT || !load.left.is_temp() ||
        load.left.temp_id != address.result.temp_id ||
        !load.right.is_none() ||
        (op_size(load.result) != 1 && op_size(load.result) != 2) ||
        (load.result.type && load.result.type->is_volatile) ||
        (load.left.type && load.left.type->base &&
         load.left.type->base->is_volatile)) {
        return false;
    }

    const operand *base = nullptr;
    if (address.left.is_temp() &&
        address.left.temp_id == scale.result.temp_id) {
        base = &address.right;
    } else if (address.right.is_temp() &&
               address.right.temp_id == scale.result.temp_id) {
        base = &address.left;
    } else {
        return false;
    }

    const operand *frame_object = nullptr;
    if (base->kind == operand_kind::SYMBOL && !base->is_global &&
        !base->is_tls && !base->is_func &&
        base->type && base->type->kind == type_kind::ARRAY) {
        frame_object = base;
    } else if (base->is_temp()) {
        const icode *base_def = find_temp_def_before(base->temp_id, idx + 1);
        if (base_def && base_def->op == icode_op::ADDRESS_OF &&
            !base_def->left.is_global && !base_def->left.is_tls &&
            !base_def->left.is_func &&
            (base_def->left.kind == operand_kind::SYMBOL ||
             base_def->left.kind == operand_kind::TEMP)) {
            frame_object = &base_def->left;
        }
    }
    if (!frame_object ||
        (frame_object->type && frame_object->type->is_volatile) ||
        temp_value_used_after(fn, idx + 2, scale.result.temp_id)) {
        return false;
    }

    // If the derived pointer remains live, materialize it in its ordinary
    // spill slot before loading through it.  A register-resident pointer
    // cannot be preserved generically because the dereference may own the
    // same pair; leave that uncommon case to the normal lowering.
    const bool address_live =
        temp_value_used_after(fn, idx + 3, address.result.temp_id);
    if (address_live && temp_regs_.count(address.result.temp_id))
        return false;

    int frame_off = frame_object->is_param
                        ? param_ix_offset(*frame_object)
                        : frame_object->stack_offset;
    if (frame_object->kind == operand_kind::TEMP)
        frame_off = ix_offset_of(*frame_object);
    frame_off += frame_object->byte_offset + base->byte_offset;

    if (debug_)
        debug_->emit_location(load.line ? load.line : scale.line);
    invalidate_pair_cache();
    invalidate_a_cache();

    // The ordinary ADD lowering protects the scaled index with push/pop
    // while it materializes the frame base.  Here the index dies at the
    // dereference, so DE can hold it and HL can be formed directly from SP.
    load_hl(scale.left);
    for (int64_t n = 0; n < scale.right.ival; ++n)
        emit_line("add\thl, hl");
    emit_line("ex\tde, hl");
    load_ix_addr_hl(frame_off);
    emit_line("add\thl, de");

    if (address_live)
        store_hl(address.result);

    if (op_size(load.result) == 1) {
        emit_line("ld\ta, (hl)");
        store_a(load.result);
    } else {
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        store_de(load.result);
    }
    idx += 2;
    return true;
}

bool z80_gen::try_emit_scaled_global_load(const ir_function &fn,
                                          size_t &idx) {
    if (idx + 2 >= fn.icodes.size())
        return false;

    const icode &scale = fn.icodes[idx];
    const icode &address = fn.icodes[idx + 1];
    const icode &load = fn.icodes[idx + 2];
    if (scale.op != icode_op::SHL || !scale.result.is_temp() ||
        scale.right.kind != operand_kind::INT_CONST ||
        scale.right.ival < 1 || scale.right.ival > 7 ||
        op_size(scale.left) != 2 || op_size(scale.result) != 2 ||
        address.op != icode_op::ADD || !address.result.is_temp() ||
        load.op != icode_op::GET_VALUE_AT || !load.left.is_temp() ||
        load.left.temp_id != address.result.temp_id ||
        !load.right.is_none() ||
        (op_size(load.result) != 1 && op_size(load.result) != 2) ||
        (load.result.type && load.result.type->is_volatile) ||
        (load.left.type && load.left.type->base &&
         load.left.type->base->is_volatile)) {
        return false;
    }

    const operand *base = nullptr;
    if (address.left.is_temp() &&
        address.left.temp_id == scale.result.temp_id) {
        base = &address.right;
    } else if (address.right.is_temp() &&
               address.right.temp_id == scale.result.temp_id) {
        base = &address.left;
    } else {
        return false;
    }

    const operand *global = nullptr;
    if (base->kind == operand_kind::SYMBOL && base->is_global &&
        !base->is_tls && !base->is_func) {
        global = base;
    } else if (base->is_temp()) {
        const icode *base_def = find_temp_def_before(base->temp_id, idx + 1);
        if (base_def && base_def->op == icode_op::ADDRESS_OF &&
            base_def->left.kind == operand_kind::SYMBOL &&
            base_def->left.is_global && !base_def->left.is_tls &&
            !base_def->left.is_func) {
            global = &base_def->left;
        }
    }
    if (!global ||
        temp_value_used_after(fn, idx + 2, scale.result.temp_id) ||
        temp_value_used_after(fn, idx + 3, address.result.temp_id)) {
        return false;
    }

    if (!scale.left.is_temp())
        return false;
    auto index_home = temp_regs_.find(scale.left.temp_id);
    if (index_home == temp_regs_.end() ||
        (index_home->second != temp_home::main_iy &&
         index_home->second != temp_home::main_de))
        return false;
    const bool index_in_de = index_home->second == temp_home::main_de;

    // Preserve the load+constant-mask fusion in gen_get_value_at().  It keeps
    // the loaded word in DE while applying the mask bytewise and is stronger
    // than folding only the address calculation here.
    if (idx + 3 < fn.icodes.size()) {
        const icode &next = fn.icodes[idx + 3];
        const bool uses_load =
            load.result.is_temp() &&
            ((next.left.is_temp() &&
              next.left.temp_id == load.result.temp_id) ||
             (next.right.is_temp() &&
              next.right.temp_id == load.result.temp_id));
        if (!index_in_de && next.op == icode_op::BAND && uses_load)
            return false;
    }

    if (debug_)
        debug_->emit_location(load.line ? load.line : scale.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    const bool value_still_in_hl = !index_in_de &&
        idx > 0 && fn.icodes[idx - 1].result.is_temp() &&
        fn.icodes[idx - 1].result.temp_id == scale.left.temp_id;
    if (index_in_de) {
        emit_line("ld\th, d");
        emit_line("ld\tl, e");
    } else if (!value_still_in_hl) {
        load_hl(scale.left);
    }
    for (int64_t n = 0; n < scale.right.ival; ++n)
        emit_line("add\thl, hl");
    if (index_in_de) {
        emit_line("push\tbc");
        emit_line("ld\tbc, %s",
                  asm_.imm_sym(mangle(global->name)).c_str());
        emit_line("add\thl, bc");
        emit_line("pop\tbc");
    } else {
        emit_line("ld\tde, %s", asm_.imm_sym(mangle(global->name)).c_str());
        emit_line("add\thl, de");
    }
    if (op_size(load.result) == 1) {
        emit_line("ld\ta, (hl)");
        store_a(load.result);
    } else if (index_in_de) {
        emit_line("ld\ta, (hl)");
        emit_line("inc\thl");
        emit_line("ld\th, (hl)");
        emit_line("ld\tl, a");
        store_hl(load.result);
    } else {
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        store_de(load.result);
    }
    idx += 2;
    return true;
}

bool z80_gen::try_emit_iy_indexed_store(const ir_function &fn, size_t &idx) {
    if (idx + 1 >= fn.icodes.size())
        return false;

    const icode &add_ic = fn.icodes[idx];
    const icode &store_ic = fn.icodes[idx + 1];
    if (add_ic.op != icode_op::ADD || !add_ic.result.is_temp() ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !store_ic.result.is_temp() ||
        store_ic.result.temp_id != add_ic.result.temp_id ||
        !store_ic.right.is_none() ||
        (op_size(store_ic.left) != 1 && op_size(store_ic.left) != 2 &&
         op_size(store_ic.left) != 4) ||
        temp_value_used_after(fn, idx + 2, add_ic.result.temp_id)) {
        return false;
    }

    const operand *cursor = &add_ic.left;
    const operand *offset = &add_ic.right;
    if (cursor->kind == operand_kind::INT_CONST)
        std::swap(cursor, offset);
    if (!cursor->is_temp() || offset->kind != operand_kind::INT_CONST)
        return false;
    auto home = temp_regs_.find(cursor->temp_id);
    if (home == temp_regs_.end() || home->second != temp_home::main_iy)
        return false;

    const int64_t disp = offset->ival;
    const int size = op_size(store_ic.left);
    if (disp < -128 || disp + size - 1 > 127)
        return false;

    if (debug_)
        debug_->emit_location(store_ic.line ? store_ic.line : add_ic.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    if (size == 1) {
        load_a(store_ic.left);
        emit_line("ld\t%lld(iy), a", static_cast<long long>(disp));
    } else if (size == 2) {
        load_de(store_ic.left);
        emit_line("ld\t%lld(iy), e", static_cast<long long>(disp));
        emit_line("ld\t%lld(iy), d", static_cast<long long>(disp + 1));
    } else {
        load_de_word(store_ic.left, 0);
        emit_line("ld\t%lld(iy), e", static_cast<long long>(disp));
        emit_line("ld\t%lld(iy), d", static_cast<long long>(disp + 1));
        load_de_word(store_ic.left, 1);
        emit_line("ld\t%lld(iy), e", static_cast<long long>(disp + 2));
        emit_line("ld\t%lld(iy), d", static_cast<long long>(disp + 3));
    }
    idx += 1;
    return true;
}

bool z80_gen::try_emit_postinc_indexed_load(const ir_function &fn, size_t &idx) {
    if (idx + 3 >= fn.icodes.size())
        return false;

    size_t p = idx;
    const icode &old_ic = fn.icodes[p++];
    const icode &step_ic = fn.icodes[p++];
    const icode &store_ic = fn.icodes[p++];

    if (!is_assign_like(old_ic.op) ||
        !old_ic.result.is_temp() ||
        !old_ic.right.is_none() ||
        !old_ic.left.type ||
        old_ic.left.type->size() != 2 ||
        !step_ic.result.is_temp() ||
        !is_assign_like(store_ic.op) ||
        !operands_equivalent(store_ic.result, old_ic.left) ||
        !temp_eq(store_ic.left, step_ic.result.temp_id)) {
        return false;
    }

    int step = 0;
    if (step_ic.op == icode_op::ADD &&
        operands_equivalent(step_ic.left, old_ic.left) &&
        is_exact_int_const(step_ic.right, 1)) {
        step = 1;
    } else if (step_ic.op == icode_op::SUB &&
               operands_equivalent(step_ic.left, old_ic.left) &&
               is_exact_int_const(step_ic.right, 1)) {
        step = -1;
    } else {
        return false;
    }

    const operand &cursor = old_ic.left;
    const bool cursor_in_bc = [&]() {
        if (cursor.is_temp()) {
            auto it = temp_regs_.find(cursor.temp_id);
            return it != temp_regs_.end() &&
                   it->second == temp_home::main_bc;
        }
        return cursor.is_symbol() && symbol_home_in_bc(cursor);
    }();
    auto cursor_frame_offset = [&]() -> std::optional<int> {
        if (cursor.kind == operand_kind::TEMP) {
            auto ri = temp_regs_.find(cursor.temp_id);
            if (ri != temp_regs_.end() &&
                !temp_home_uses_spill_slot(ri->second)) {
                return std::nullopt;
            }
            int off = ix_offset_of(cursor);
            if (fits_ix_disp(off) && fits_ix_disp(off + 1))
                return off;
            return std::nullopt;
        }
        if (cursor.kind == operand_kind::SYMBOL &&
            !cursor.is_global &&
            !cursor.is_param) {
            auto sri = symbol_regs_.find(symbol_reg_key(cursor));
            if (sri != symbol_regs_.end() &&
                !temp_home_uses_spill_slot(sri->second)) {
                return std::nullopt;
            }
            int off = ix_offset_of(cursor);
            if (fits_ix_disp(off) && fits_ix_disp(off + 1))
                return off;
        }
        return std::nullopt;
    }();
    if (!cursor_in_bc && !cursor_frame_offset)
        return false;

    auto emit_cursor_step = [&](int off) {
        const std::string done_lbl = fresh_local_label(
            step > 0 ? "__postinc_done" : "__postdec_done");
        if (step > 0) {
            emit_line("inc\t%s", asm_.ix_rel(off).c_str());
            emit_line("jr\tnz, %s", done_lbl.c_str());
            emit_line("inc\t%s", asm_.ix_rel(off + 1).c_str());
            emit_label(done_lbl, false);
        } else {
            emit_line("ld\ta, %s", asm_.ix_rel(off).c_str());
            emit_line("or\ta, a");
            emit_line("jr\tnz, %s", done_lbl.c_str());
            emit_line("dec\t%s", asm_.ix_rel(off + 1).c_str());
            emit_label(done_lbl, false);
            emit_line("dec\t%s", asm_.ix_rel(off).c_str());
        }
    };

    if (p < fn.icodes.size()) {
        const icode &get_candidate = fn.icodes[p];
        if (get_candidate.op == icode_op::GET_VALUE_AT &&
            get_candidate.left.is_temp() &&
            get_candidate.left.temp_id == old_ic.result.temp_id &&
            get_candidate.right.is_none()) {
            const int load_size = op_size(get_candidate.result);
            if (load_size != 1 && load_size != 2)
                return false;
            if (temp_value_used_after(fn, p + 1, old_ic.result.temp_id) ||
                temp_value_used_after(fn, idx + 3, step_ic.result.temp_id)) {
                return false;
            }

            if (debug_)
                debug_->emit_location(old_ic.line);

            invalidate_pair_cache();
            invalidate_a_cache();
            if (cursor_in_bc && load_size == 1) {
                emit_line("ld\ta, (bc)");
                emit_line(step > 0 ? "inc\tbc" : "dec\tbc");
                store_a(get_candidate.result);
                idx = p;
                return true;
            }
            load_hl(cursor);
            invalidate_pair_cache();
            emit_cursor_step(*cursor_frame_offset);

            if (load_size == 1) {
                emit_line("ld\ta, (hl)");
                store_a(get_candidate.result);
            } else {
                emit_line("ld\te, (hl)");
                emit_line("inc\thl");
                emit_line("ld\td, (hl)");
                store_de(get_candidate.result);
            }

            idx = p;
            return true;
        }
    }

    if (p >= fn.icodes.size())
        return false;
    const icode &scale_ic = fn.icodes[p++];
    if (scale_ic.op != icode_op::SHL ||
        !scale_ic.result.is_temp() ||
        !temp_eq(scale_ic.left, old_ic.result.temp_id) ||
        !is_exact_int_const(scale_ic.right, 1)) {
        return false;
    }

    auto is_global_data_ref = [](const operand &op) {
        if (op.kind == operand_kind::LABEL_REF)
            return true;
        return op.kind == operand_kind::SYMBOL &&
               op.is_global &&
               !op.is_tls &&
               !op.is_sfr &&
               !op.is_func &&
               !op.is_param;
    };

    const operand *base = nullptr;
    const icode *get_ic = nullptr;
    const icode *ptr_add_ic = nullptr;
    size_t get_idx = p;

    if (p < fn.icodes.size()) {
        const icode &candidate = fn.icodes[p];
        if (candidate.op == icode_op::GET_VALUE_AT &&
            is_global_data_ref(candidate.left) &&
            temp_eq(candidate.right, scale_ic.result.temp_id)) {
            base = &candidate.left;
            get_ic = &candidate;
            get_idx = p;
            ++p;
        }
    }

    if (!get_ic && p + 1 < fn.icodes.size()) {
        const icode &ptr_ic = fn.icodes[p++];
        const icode &candidate = fn.icodes[p++];
        if (ptr_ic.op != icode_op::ADD ||
            !ptr_ic.result.is_temp() ||
            candidate.op != icode_op::GET_VALUE_AT ||
            !candidate.left.is_temp() ||
            candidate.left.temp_id != ptr_ic.result.temp_id ||
            !candidate.right.is_none()) {
            return false;
        }

        if (is_global_data_ref(ptr_ic.left) &&
            temp_eq(ptr_ic.right, scale_ic.result.temp_id)) {
            base = &ptr_ic.left;
        } else if (is_global_data_ref(ptr_ic.right) &&
                   temp_eq(ptr_ic.left, scale_ic.result.temp_id)) {
            base = &ptr_ic.right;
        } else {
            return false;
        }

        ptr_add_ic = &ptr_ic;
        get_ic = &candidate;
        get_idx = p - 1;
    }

    if (!base || !get_ic)
        return false;

    const int load_size = op_size(get_ic->result);
    if (load_size != 1 && load_size != 2)
        return false;

    if (temp_value_used_after(fn, idx + 4, old_ic.result.temp_id) ||
        temp_value_used_after(fn, idx + 3, step_ic.result.temp_id) ||
        temp_value_used_after(fn, get_idx + 1, scale_ic.result.temp_id)) {
        return false;
    }
    if (ptr_add_ic &&
        temp_value_used_after(fn, get_idx + 1, ptr_add_ic->result.temp_id)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(old_ic.line);

    invalidate_pair_cache();
    invalidate_a_cache();
    load_hl(cursor);
    invalidate_pair_cache();

    const int off = *cursor_frame_offset;
    emit_cursor_step(off);

    emit_line("add\thl, hl");
    emit_line("ld\tde, %s", asm_.imm_sym(asm_symbol_ref_name(*base)).c_str());
    emit_line("add\thl, de");

    if (load_size == 1) {
        emit_line("ld\ta, (hl)");
        store_a(get_ic->result);
    } else {
        emit_line("ld\te, (hl)");
        emit_line("inc\thl");
        emit_line("ld\td, (hl)");
        store_de(get_ic->result);
    }

    idx = get_idx;
    return true;
}

bool z80_gen::try_emit_postdec_truth(const ir_function &fn, size_t &idx) {
    if (idx + 2 >= fn.icodes.size())
        return false;

    const icode &old_ic = fn.icodes[idx];
    const icode &step_ic = fn.icodes[idx + 1];
    const icode &ifx_ic = fn.icodes[idx + 2];
    if (old_ic.op != icode_op::ASSIGN || !old_ic.result.is_temp() ||
        old_ic.right.is_none() == false || op_size(old_ic.left) != 2 ||
        !operands_equivalent(step_ic.result, old_ic.left) ||
        step_ic.op != icode_op::SUB ||
        !operands_equivalent(step_ic.left, old_ic.left) ||
        step_ic.right.kind != operand_kind::INT_CONST ||
        step_ic.right.ival != 1 || ifx_ic.op != icode_op::IFX ||
        !operands_equivalent(ifx_ic.left, old_ic.result) ||
        (old_ic.left.type && old_ic.left.type->is_volatile) ||
        temp_value_used_after(fn, idx + 3, old_ic.result.temp_id)) {
        return false;
    }

    auto compact_frame_value = [&](const operand &op) {
        if (op.kind == operand_kind::TEMP) {
            auto home = temp_regs_.find(op.temp_id);
            if (home != temp_regs_.end() &&
                !temp_home_uses_spill_slot(home->second)) {
                return false;
            }
        } else if (op.kind == operand_kind::SYMBOL) {
            if (op.is_global || op.is_tls || op.is_sfr || op.is_func)
                return false;
            if (symbol_regs_.count(symbol_reg_key(op)) ||
                incoming_symbol_homes_.count(op.stack_offset)) {
                return false;
            }
        } else {
            return false;
        }
        const int off = ix_offset_of(op);
        return fits_ix_disp(off) && fits_ix_disp(off + 1);
    };
    if (!compact_frame_value(old_ic.left))
        return false;

    if (debug_)
        debug_->emit_location(old_ic.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    load_hl(old_ic.left);
    emit_line("ld\ta, h");
    emit_line("or\ta, l");
    emit_line("push\taf");
    emit_line("dec\thl");
    store_hl(old_ic.left);
    emit_line("pop\taf");
    if (!ifx_ic.true_lbl.empty() && !ifx_ic.false_lbl.empty()) {
        emit_line("jp\tnz, %s", ifx_ic.true_lbl.c_str());
        emit_line("jp\t%s", ifx_ic.false_lbl.c_str());
    } else if (!ifx_ic.true_lbl.empty()) {
        emit_line("jp\tnz, %s", ifx_ic.true_lbl.c_str());
    } else if (!ifx_ic.false_lbl.empty()) {
        emit_line("jp\tz, %s", ifx_ic.false_lbl.c_str());
    }
    idx += 2;
    return true;
}

bool z80_gen::try_emit_postinc_indexed_store(const ir_function &fn,
                                              size_t &idx) {
    if (idx + 3 >= fn.icodes.size())
        return false;

    const icode &old_ic = fn.icodes[idx];
    const icode &step_ic = fn.icodes[idx + 1];
    const icode &address_ic = fn.icodes[idx + 2];
    const icode &store_ic = fn.icodes[idx + 3];
    if (old_ic.op != icode_op::ASSIGN || !old_ic.result.is_temp() ||
        !old_ic.right.is_none() || op_size(old_ic.left) != 2 ||
        step_ic.op != icode_op::ADD ||
        !operands_equivalent(step_ic.left, old_ic.left) ||
        step_ic.right.kind != operand_kind::INT_CONST ||
        step_ic.right.ival != 1 ||
        !operands_equivalent(step_ic.result, old_ic.left) ||
        address_ic.op != icode_op::ADD || !address_ic.result.is_temp() ||
        store_ic.op != icode_op::SET_VALUE_AT ||
        !store_ic.result.is_temp() ||
        store_ic.result.temp_id != address_ic.result.temp_id ||
        !store_ic.right.is_none() || op_size(store_ic.left) != 1 ||
        (old_ic.left.type && old_ic.left.type->is_volatile) ||
        temp_value_used_after(fn, idx + 3, old_ic.result.temp_id) ||
        temp_value_used_after(fn, idx + 4, address_ic.result.temp_id)) {
        return false;
    }

    const operand *base = nullptr;
    if (address_ic.left.is_temp() &&
        address_ic.left.temp_id == old_ic.result.temp_id) {
        base = &address_ic.right;
    } else if (address_ic.right.is_temp() &&
               address_ic.right.temp_id == old_ic.result.temp_id) {
        base = &address_ic.left;
    } else {
        return false;
    }
    if (!base->type || !base->type->is_ptr() || base->type->is_far_ptr())
        return false;

    auto compact_frame_word = [&](const operand &op) {
        if (op.kind == operand_kind::TEMP) {
            auto home = temp_regs_.find(op.temp_id);
            if (home != temp_regs_.end() &&
                !temp_home_uses_spill_slot(home->second)) {
                return false;
            }
        } else if (op.kind == operand_kind::SYMBOL) {
            if (op.is_global || op.is_tls || op.is_sfr || op.is_func ||
                symbol_regs_.count(symbol_reg_key(op)) ||
                incoming_symbol_homes_.count(op.stack_offset)) {
                return false;
            }
        } else {
            return false;
        }
        const int off = ix_offset_of(op);
        return fits_ix_disp(off) && fits_ix_disp(off + 1);
    };
    if (!compact_frame_word(old_ic.left) || !compact_frame_word(*base))
        return false;

    if (debug_)
        debug_->emit_location(old_ic.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    // Load the byte first; the accepted direct frame pair operations below
    // preserve A while updating the counter and forming the old-index address.
    load_a(store_ic.left);
    load_hl(old_ic.left);
    emit_line("ld\td, h");
    emit_line("ld\te, l");
    emit_line("inc\thl");
    store_hl(old_ic.left);
    load_hl(*base);
    emit_line("add\thl, de");
    emit_line("ld\t(hl), a");
    idx += 3;
    invalidate_pair_cache();
    invalidate_a_cache();
    return true;
}

bool z80_gen::try_emit_shift_add_byte_accumulate(const ir_function &fn,
                                                  size_t &idx) {
    if (opt_settings_.level != opt_level::Of &&
        opt_settings_.level != opt_level::O3 &&
        opt_settings_.level != opt_level::Os)
        return false;
    if (idx + 3 >= fn.icodes.size())
        return false;

    const icode &shift = fn.icodes[idx];
    const icode &sum = fn.icodes[idx + 1];
    const icode &load = fn.icodes[idx + 2];
    const icode *widen = nullptr;
    const icode *accumulate = &fn.icodes[idx + 3];
    if (shift.op != icode_op::SHL || !shift.result.is_temp() ||
        shift.right.kind != operand_kind::INT_CONST ||
        op_size(shift.left) != 2 || op_size(shift.result) != 2 ||
        sum.op != icode_op::ADD || !sum.result.is_temp() ||
        load.op != icode_op::GET_VALUE_AT || !load.result.is_temp() ||
        op_size(load.result) != 1 || !load.right.is_none() ||
        !load.left.type || !load.left.type->is_ptr() ||
        load.left.type->is_far_ptr() ||
        !load.result.type || !load.result.type->is_unsigned() ||
        (load.result.type && load.result.type->is_volatile) ||
        (load.left.type->base && load.left.type->base->is_volatile) ||
        accumulate->op != icode_op::ADD ||
        op_size(accumulate->result) != 2) {
        return false;
    }

    // Value propagation may leave the unsigned-byte operand directly on the
    // word ADD instead of retaining an explicit byte-to-word CAST.  Accept
    // both equivalent IR forms so this target fusion is not coupled to the
    // exact cleanup order of an earlier pass.
    if (idx + 4 < fn.icodes.size()) {
        const icode &possible_widen = fn.icodes[idx + 3];
        if (possible_widen.op == icode_op::CAST &&
            possible_widen.result.is_temp() &&
            temp_eq(possible_widen.left, load.result.temp_id) &&
            op_size(possible_widen.result) == 2 &&
            possible_widen.left.type &&
            possible_widen.left.type->size() == 1 &&
            possible_widen.left.type->is_unsigned()) {
            widen = &possible_widen;
            accumulate = &fn.icodes[idx + 4];
            if (accumulate->op != icode_op::ADD ||
                op_size(accumulate->result) != 2)
                return false;
        }
    }

    const int count = static_cast<int>(shift.right.ival);
    if (count <= 0 || count > 7)
        return false;
    const bool sum_shift_left =
        temp_eq(sum.left, shift.result.temp_id) &&
        operands_equivalent(sum.right, shift.left);
    const bool sum_shift_right =
        temp_eq(sum.right, shift.result.temp_id) &&
        operands_equivalent(sum.left, shift.left);
    const int byte_value_tid =
        widen ? widen->result.temp_id : load.result.temp_id;
    const bool final_sum_left =
        temp_eq(accumulate->left, sum.result.temp_id) &&
        temp_eq(accumulate->right, byte_value_tid);
    const bool final_sum_right =
        temp_eq(accumulate->right, sum.result.temp_id) &&
        temp_eq(accumulate->left, byte_value_tid);
    if ((!sum_shift_left && !sum_shift_right) ||
        (!final_sum_left && !final_sum_right) ||
        temp_value_used_after(fn, idx + 2, shift.result.temp_id) ||
        temp_value_used_after(fn, idx + (widen ? 5 : 4),
                              sum.result.temp_id) ||
        temp_value_used_after(fn, idx + 4,
                              load.result.temp_id) ||
        (widen && temp_value_used_after(fn, idx + 5,
                                       widen->result.temp_id))) {
        return false;
    }

    if (debug_)
        debug_->emit_location(accumulate->line ? accumulate->line :
                                                  shift.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    load_hl(shift.left);
    emit_line("ld\td, h");
    emit_line("ld\te, l");
    for (int i = 0; i < count; ++i)
        emit_line("add\thl, hl");
    emit_line("add\thl, de");

    auto pointer_home = load.left.is_temp()
                            ? temp_regs_.find(load.left.temp_id)
                            : temp_regs_.end();
    if (pointer_home != temp_regs_.end() &&
        pointer_home->second == temp_home::main_iy) {
        emit_line("ld\te, 0(iy)");
    } else if (pointer_home != temp_regs_.end() &&
               pointer_home->second == temp_home::main_bc) {
        emit_line("ld\ta, (bc)");
        emit_line("ld\te, a");
    } else {
        emit_line("push\thl");
        load_hl(load.left);
        emit_line("ld\te, (hl)");
        emit_line("pop\thl");
    }
    emit_line("ld\td, %s", asm_.imm(0).c_str());
    emit_line("add\thl, de");
    store_hl(accumulate->result);
    invalidate_pair_cache();
    invalidate_a_cache();
    idx += widen ? 4 : 3;
    return true;
}

bool z80_gen::try_emit_shift_xor_self(const ir_function &fn, size_t &idx) {
    if (idx + 1 >= fn.icodes.size())
        return false;

    const icode &shift_ic = fn.icodes[idx];
    const icode &xor_ic = fn.icodes[idx + 1];

    if (shift_ic.op != icode_op::SHR ||
        !shift_ic.result.is_temp() ||
        shift_ic.right.kind != operand_kind::INT_CONST ||
        op_size(shift_ic.left) != 2 ||
        op_size(shift_ic.result) != 2 ||
        xor_ic.op != icode_op::BXOR ||
        !xor_ic.result.is_temp()) {
        return false;
    }

    const bool logical_shift =
        (shift_ic.left.type && shift_ic.left.type->is_unsigned()) ||
        (shift_ic.result.type && shift_ic.result.type->is_unsigned());
    if (!logical_shift)
        return false;

    const int count = static_cast<int>(shift_ic.right.ival);
    if (count <= 0 || count > 8)
        return false;

    const bool shift_on_left = temp_eq(xor_ic.left, shift_ic.result.temp_id) &&
                               operands_equivalent(xor_ic.right, shift_ic.left);
    const bool shift_on_right = temp_eq(xor_ic.right, shift_ic.result.temp_id) &&
                                operands_equivalent(xor_ic.left, shift_ic.left);
    if (!shift_on_left && !shift_on_right)
        return false;

    operand target = xor_ic.result;
    size_t last_idx = idx + 1;
    if (idx + 2 < fn.icodes.size()) {
        const icode &assign_ic = fn.icodes[idx + 2];
        if (is_assign_like(assign_ic.op) &&
            assign_ic.left.is_temp() &&
            assign_ic.left.temp_id == xor_ic.result.temp_id &&
            assign_ic.right.is_none() &&
            op_size(assign_ic.result) == 2 &&
            !temp_value_used_after(fn, idx + 3, xor_ic.result.temp_id)) {
            target = assign_ic.result;
            last_idx = idx + 2;
        }
    }

    struct shift_xor_update {
        const icode *shift = nullptr;
        const icode *bxor = nullptr;
        const icode *assign = nullptr;
        operand target;
        int count = 0;
        size_t last_idx = 0;
    };

    auto match_shift_xor_update =
        [&](size_t start, const operand &base,
            shift_xor_update &out) -> bool {
            if (start + 2 >= fn.icodes.size() || !base.is_temp())
                return false;
            const icode &s = fn.icodes[start];
            const icode &x = fn.icodes[start + 1];
            const icode &a = fn.icodes[start + 2];
            if (s.op != icode_op::SHR ||
                !s.result.is_temp() ||
                s.right.kind != operand_kind::INT_CONST ||
                !operands_equivalent(s.left, base) ||
                op_size(s.left) != 2 ||
                op_size(s.result) != 2 ||
                x.op != icode_op::BXOR ||
                !x.result.is_temp()) {
                return false;
            }
            const bool logical =
                (s.left.type && s.left.type->is_unsigned()) ||
                (s.result.type && s.result.type->is_unsigned());
            if (!logical)
                return false;
            const int c = static_cast<int>(s.right.ival);
            if (c <= 0 || c > 8)
                return false;
            const bool shift_left =
                temp_eq(x.left, s.result.temp_id) &&
                operands_equivalent(x.right, base);
            const bool shift_right =
                temp_eq(x.right, s.result.temp_id) &&
                operands_equivalent(x.left, base);
            if (!shift_left && !shift_right)
                return false;
            if (!is_assign_like(a.op) ||
                !a.left.is_temp() ||
                a.left.temp_id != x.result.temp_id ||
                !a.right.is_none() ||
                op_size(a.result) != 2) {
                return false;
            }
            if (temp_value_used_after(fn, start + 2, s.result.temp_id) ||
                temp_value_used_after(fn, start + 3, x.result.temp_id)) {
                return false;
            }
            out.shift = &s;
            out.bxor = &x;
            out.assign = &a;
            out.target = a.result;
            out.count = c;
            out.last_idx = start + 2;
            return true;
        };

    if (last_idx == idx + 2 && target.is_temp()) {
        shift_xor_update next_step;
        if (match_shift_xor_update(last_idx + 1, target, next_step) &&
            !temp_value_used_after(fn, idx + 2, shift_ic.result.temp_id) &&
            !temp_value_used_after(fn, idx + 3, xor_ic.result.temp_id)) {
            if (debug_)
                debug_->emit_location(shift_ic.line);

            auto emit_step = [&](int c) {
                emit_line("ld\td, h");
                emit_line("ld\te, l");
                if (c == 8) {
                    emit_line("ld\tl, h");
                    emit_line("ld\th, %s", asm_.imm(0).c_str());
                } else if (c <= (size_opt_enabled() ? 5 :
                                 (tuned_profile_enabled() ? 7 : 5))) {
                    for (int k = 0; k < c; ++k) {
                        emit_line("srl\th");
                        emit_line("rr\tl");
                    }
                } else {
                    emit_line("ld\tb, %s", asm_.imm(c).c_str());
                    std::string loop_lbl = fresh_local_label("__shiftxor");
                    emit_label(loop_lbl, false);
                    emit_line("srl\th");
                    emit_line("rr\tl");
                    emit_line("djnz\t%s", loop_lbl.c_str());
                }
                emit_line("ld\ta, l");
                emit_line("xor\ta, e");
                emit_line("ld\tl, a");
                emit_line("ld\ta, h");
                emit_line("xor\ta, d");
                emit_line("ld\th, a");
            };

            invalidate_pair_cache();
            invalidate_a_cache();
            load_hl(shift_ic.left);
            emit_step(count);
            emit_step(next_step.count);
            store_hl(next_step.target);
            idx = next_step.last_idx;
            return true;
        }
    }

    if (temp_value_used_after(fn, idx + 2, shift_ic.result.temp_id))
        return false;
    if (last_idx == idx + 1 &&
        temp_value_used_after(fn, idx + 2, xor_ic.result.temp_id)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(shift_ic.line);

    invalidate_pair_cache();
    invalidate_a_cache();
    load_hl(shift_ic.left);
    emit_line("ld\td, h");
    emit_line("ld\te, l");
    if (count == 8) {
        emit_line("ld\tl, h");
        emit_line("ld\th, %s", asm_.imm(0).c_str());
    } else if (count <= (size_opt_enabled() ? 5 :
                         (tuned_profile_enabled() ? 7 : 5))) {
        for (int k = 0; k < count; ++k) {
            emit_line("srl\th");
            emit_line("rr\tl");
        }
    } else {
        emit_line("ld\tb, %s", asm_.imm(count).c_str());
        std::string loop_lbl = fresh_local_label("__shiftxor");
        emit_label(loop_lbl, false);
        emit_line("srl\th");
        emit_line("rr\tl");
        emit_line("djnz\t%s", loop_lbl.c_str());
    }
    emit_line("ld\ta, l");
    emit_line("xor\ta, e");
    emit_line("ld\tl, a");
    emit_line("ld\ta, h");
    emit_line("xor\ta, d");
    emit_line("ld\th, a");
    store_hl(target);

    idx = last_idx;
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

bool z80_gen::try_emit_byte_load_compare_ifx(const ir_function &fn,
                                             size_t &idx) {
    if (!compare_ifx_fusion_enabled() || idx + 2 >= fn.icodes.size())
        return false;

    const icode &first = fn.icodes[idx];
    if (first.op != icode_op::GET_VALUE_AT ||
        !first.result.is_temp() || op_size(first.result) != 1 ||
        !first.right.is_none() || !first.left.type ||
        !first.left.type->is_ptr() || first.left.type->is_far_ptr() ||
        (first.result.type && first.result.type->is_volatile) ||
        (first.left.type->base && first.left.type->base->is_volatile)) {
        return false;
    }

    // Compare a byte loaded through an IY-resident cursor directly against
    // an already-live byte.  This is the single-load counterpart of the
    // two-dereference fusion below and is especially useful after a preceding
    // short-circuit truth test kept the first byte in a register.
    {
        const icode &cmp = fn.icodes[idx + 1];
        const icode &ifx = fn.icodes[idx + 2];
        const bool first_on_left =
            cmp.left.is_temp() &&
            cmp.left.temp_id == first.result.temp_id;
        const bool first_on_right =
            cmp.right.is_temp() &&
            cmp.right.temp_id == first.result.temp_id;
        const operand *other = first_on_left ? &cmp.right : &cmp.left;
        auto pointer_home = first.left.is_temp()
                                ? temp_regs_.find(first.left.temp_id)
                                : temp_regs_.end();
        if ((cmp.op == icode_op::EQ || cmp.op == icode_op::NE) &&
            first_on_left != first_on_right && other->type &&
            op_size(*other) == 1 &&
            pointer_home != temp_regs_.end() &&
            pointer_home->second == temp_home::main_iy &&
            cmp.result.is_temp() && ifx.op == icode_op::IFX &&
            ifx.left.is_temp() &&
            ifx.left.temp_id == cmp.result.temp_id &&
            (!ifx.true_lbl.empty() || !ifx.false_lbl.empty()) &&
            !temp_value_used_after(fn, idx + 2, first.result.temp_id) &&
            !temp_value_used_after(fn, idx + 3, cmp.result.temp_id)) {
            if (debug_)
                debug_->emit_location(ifx.line ? ifx.line : cmp.line);
            invalidate_pair_cache();
            invalidate_a_cache();
            load_a(*other);
            emit_line("cp\t0(iy)");
            const char *condition = cmp.op == icode_op::EQ ? "z" : "nz";
            if (!ifx.true_lbl.empty())
                emit_line("jp\t%s, %s", condition, ifx.true_lbl.c_str());
            if (!ifx.false_lbl.empty())
                emit_line("jp\t%s", ifx.false_lbl.c_str());
            invalidate_pair_cache();
            invalidate_a_cache();
            idx += 2;
            cur_ic_index_ = idx;
            return true;
        }
    }

    if (idx + 3 >= fn.icodes.size())
        return false;

    // Accept either two adjacent dereferences or one ordinary pointer ADD
    // between them. The latter is the canonical lowering of a[i] != b[i].
    size_t second_idx = idx + 1;
    const icode *address = nullptr;
    if (fn.icodes[second_idx].op != icode_op::GET_VALUE_AT) {
        address = &fn.icodes[second_idx];
        if (address->op != icode_op::ADD ||
            !address->result.is_temp() || op_size(address->result) != 2) {
            return false;
        }
        ++second_idx;
    }
    if (second_idx + 2 >= fn.icodes.size())
        return false;

    const icode &second = fn.icodes[second_idx];
    const icode &cmp = fn.icodes[second_idx + 1];
    const icode &ifx = fn.icodes[second_idx + 2];
    if (second.op != icode_op::GET_VALUE_AT ||
        !second.result.is_temp() || op_size(second.result) != 1 ||
        !second.right.is_none() || !second.left.type ||
        !second.left.type->is_ptr() || second.left.type->is_far_ptr() ||
        (second.result.type && second.result.type->is_volatile) ||
        (second.left.type->base && second.left.type->base->is_volatile) ||
        (address && (!second.left.is_temp() ||
                     second.left.temp_id != address->result.temp_id)) ||
        (cmp.op != icode_op::EQ && cmp.op != icode_op::NE) ||
        !cmp.result.is_temp() || ifx.op != icode_op::IFX ||
        !ifx.left.is_temp() || ifx.left.temp_id != cmp.result.temp_id ||
        (ifx.true_lbl.empty() && ifx.false_lbl.empty())) {
        return false;
    }

    const bool normal_order =
        cmp.left.is_temp() && cmp.left.temp_id == first.result.temp_id &&
        cmp.right.is_temp() && cmp.right.temp_id == second.result.temp_id;
    const bool reverse_order =
        cmp.right.is_temp() && cmp.right.temp_id == first.result.temp_id &&
        cmp.left.is_temp() && cmp.left.temp_id == second.result.temp_id;
    if ((!normal_order && !reverse_order) ||
        temp_value_used_after(fn, second_idx + 2, first.result.temp_id) ||
        temp_value_used_after(fn, second_idx + 2, second.result.temp_id) ||
        temp_value_used_after(fn, second_idx + 3, cmp.result.temp_id) ||
        (address && temp_value_used_after(fn, second_idx + 1,
                                          address->result.temp_id))) {
        return false;
    }

    if (debug_)
        debug_->emit_location(ifx.line ? ifx.line : cmp.line);
    invalidate_pair_cache();
    invalidate_a_cache();
    auto first_home = first.left.is_temp()
                          ? temp_regs_.find(first.left.temp_id)
                          : temp_regs_.end();
    if (first_home != temp_regs_.end() &&
        first_home->second == temp_home::main_iy) {
        emit_line("ld\ta, 0(iy)");
    } else if (first_home != temp_regs_.end() &&
               first_home->second == temp_home::main_bc) {
        emit_line("ld\ta, (bc)");
    } else {
        load_hl(first.left);
        emit_line("ld\ta, (hl)");
    }
    emit_line("push\taf");
    if (address) {
        // Materialize the intervening address exactly as normal codegen would;
        // AF protects the first byte across arbitrary pointer operands.
        cur_ic_index_ = idx + 1;
        gen_icode(*address);
    }
    load_hl(second.left);
    emit_line("pop\taf");
    emit_line("cp\t(hl)");

    const char *condition = cmp.op == icode_op::EQ ? "z" : "nz";
    if (!ifx.true_lbl.empty())
        emit_line("jp\t%s, %s", condition, ifx.true_lbl.c_str());
    if (!ifx.false_lbl.empty())
        emit_line("jp\t%s", ifx.false_lbl.c_str());
    invalidate_pair_cache();
    invalidate_a_cache();
    idx = second_idx + 2;
    cur_ic_index_ = idx;
    return true;
}

bool z80_gen::try_emit_guarded_zero_arg_indirect_call(
    const ir_function &fn, size_t &idx) {
    if (!compare_ifx_fusion_enabled() || idx + 2 >= fn.icodes.size())
        return false;

    auto is_plain_callback_global = [&](const operand &op) {
        return op.kind == operand_kind::SYMBOL && op.is_global &&
               !op.is_param && !op.is_func && !op.is_tls && !op.is_sfr &&
               op.type && !op.type->is_volatile && op_size(op) == 2;
    };
    auto target_count = [&](const std::string &label) {
        size_t count = 0;
        for (const icode &ic : fn.icodes) {
            if (ic.op == icode_op::GOTO && ic.label_name == label)
                ++count;
            if (ic.op == icode_op::IFX) {
                if (ic.true_lbl == label)
                    ++count;
                if (ic.false_lbl == label)
                    ++count;
            }
        }
        return count;
    };
    auto emit_guarded_call = [&](const operand &target,
                                 const icode &branch,
                                 const icode &call_label,
                                 size_t consumed) {
        cur_ic_index_ = idx;
        if (debug_)
            debug_->emit_location(branch.line);
        load_hl(target);
        emit_line("ld\ta, h");
        emit_line("or\ta, l");
        emit_line("jp\tz, %s", branch.false_lbl.c_str());
        emit_label(call_label.label_name, false);
        asm_.global_decl("__sdcc_call_hl");
        emit_line("call\t__sdcc_call_hl");
        invalidate_pair_cache();
        invalidate_a_cache();
        idx += consumed;
        cur_ic_index_ = idx;
        return true;
    };

    const icode &direct_branch = fn.icodes[idx];
    const icode &direct_label = fn.icodes[idx + 1];
    const icode &direct_call = fn.icodes[idx + 2];
    if (direct_branch.op == icode_op::IFX &&
        is_plain_callback_global(direct_branch.left) &&
        !direct_branch.true_lbl.empty() &&
        !direct_branch.false_lbl.empty() &&
        direct_label.op == icode_op::LABEL &&
        direct_label.label_name == direct_branch.true_lbl &&
        direct_call.op == icode_op::CALL &&
        direct_call.func_name.empty() && direct_call.result.is_none() &&
        operands_equivalent(direct_call.left, direct_branch.left) &&
        direct_call.num_params == 0 && direct_call.arg_bytes == 0 &&
        target_count(direct_label.label_name) == 1) {
        return emit_guarded_call(direct_branch.left, direct_branch,
                                 direct_label, 2);
    }

    if (idx + 4 >= fn.icodes.size())
        return false;

    const icode &load = fn.icodes[idx];
    const icode &branch = fn.icodes[idx + 1];
    const icode &call_label = fn.icodes[idx + 2];
    const icode &reload = fn.icodes[idx + 3];
    const icode &call = fn.icodes[idx + 4];

    if (load.op != icode_op::ASSIGN || !load.result.is_temp() ||
        !load.right.is_none() || !is_plain_callback_global(load.left) ||
        branch.op != icode_op::IFX || !branch.left.is_temp() ||
        branch.left.temp_id != load.result.temp_id ||
        branch.true_lbl.empty() || branch.false_lbl.empty() ||
        call_label.op != icode_op::LABEL ||
        call_label.label_name != branch.true_lbl ||
        reload.op != icode_op::ASSIGN || !reload.result.is_temp() ||
        !reload.right.is_none() ||
        !operands_equivalent(reload.left, load.left) ||
        call.op != icode_op::CALL || !call.func_name.empty() ||
        !call.result.is_none() || !call.left.is_temp() ||
        call.left.temp_id != reload.result.temp_id ||
        call.num_params != 0 || call.arg_bytes != 0 ||
        temp_value_used_after(fn, idx + 2, load.result.temp_id) ||
        temp_value_used_after(fn, idx + 5, reload.result.temp_id)) {
        return false;
    }

    if (target_count(call_label.label_name) != 1)
        return false;

    return emit_guarded_call(load.left, branch, call_label, 4);
}

bool z80_gen::try_emit_word_select_send(const ir_function &fn, size_t &idx) {
    if (!compare_ifx_fusion_enabled() || idx + 8 >= fn.icodes.size())
        return false;

    const icode &compare = fn.icodes[idx];
    const icode &branch = fn.icodes[idx + 1];
    const icode &true_label = fn.icodes[idx + 2];
    const icode &true_value = fn.icodes[idx + 3];
    const icode &true_goto = fn.icodes[idx + 4];
    const icode &false_label = fn.icodes[idx + 5];
    const icode &false_value = fn.icodes[idx + 6];
    const icode &join_label = fn.icodes[idx + 7];
    const icode &send = fn.icodes[idx + 8];

    if (!is_compare_op(compare.op) || !compare.result.is_temp() ||
        branch.op != icode_op::IFX || !branch.left.is_temp() ||
        branch.left.temp_id != compare.result.temp_id ||
        branch.true_lbl.empty() || branch.false_lbl.empty() ||
        true_label.op != icode_op::LABEL ||
        true_label.label_name != branch.true_lbl ||
        false_label.op != icode_op::LABEL ||
        false_label.label_name != branch.false_lbl ||
        true_value.op != icode_op::ASSIGN ||
        false_value.op != icode_op::ASSIGN ||
        !true_value.result.is_temp() || !false_value.result.is_temp() ||
        true_value.result.temp_id != false_value.result.temp_id ||
        !true_value.right.is_none() || !false_value.right.is_none() ||
        op_size(true_value.result) != 2 ||
        op_size(false_value.result) != 2 ||
        op_size(true_value.left) != 2 ||
        op_size(false_value.left) != 2 ||
        true_goto.op != icode_op::GOTO ||
        join_label.op != icode_op::LABEL ||
        true_goto.label_name != join_label.label_name ||
        send.op != icode_op::SEND || send.arg_loc != abi_arg_loc::STACK ||
        !send.left.is_temp() ||
        send.left.temp_id != true_value.result.temp_id ||
        op_size(send.left) != 2 ||
        temp_value_used_after(fn, idx + 2, compare.result.temp_id) ||
        temp_value_used_after(fn, idx + 9, true_value.result.temp_id)) {
        return false;
    }

    auto target_count = [&](const std::string &label) {
        size_t count = 0;
        for (const icode &ic : fn.icodes) {
            if (ic.op == icode_op::GOTO && ic.label_name == label)
                ++count;
            if (ic.op == icode_op::IFX) {
                if (ic.true_lbl == label)
                    ++count;
                if (ic.false_lbl == label)
                    ++count;
            }
        }
        return count;
    };
    if (target_count(true_label.label_name) != 1 ||
        target_count(false_label.label_name) != 1 ||
        target_count(join_label.label_name) != 1) {
        return false;
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(branch.line ? branch.line : compare.line);
    emit_compare_branch(compare, compare.op,
                        branch.true_lbl, branch.false_lbl);
    emit_label(true_label.label_name, false);
    load_hl(true_value.left);
    emit_line("jp\t%s", join_label.label_name.c_str());
    emit_label(false_label.label_name, false);
    load_hl(false_value.left);
    emit_label(join_label.label_name, false);
    emit_line("push\thl");
    invalidate_pair_cache();
    invalidate_a_cache();

    idx += 8;
    cur_ic_index_ = idx;
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

bool z80_gen::try_emit_lsb32_shift_xor_diamond(const ir_function &fn,
                                               size_t &idx) {
    if (!compare_ifx_fusion_enabled() || idx + 11 >= fn.icodes.size())
        return false;

    const auto &band_ic = fn.icodes[idx];
    const auto &ifx_ic = fn.icodes[idx + 1];
    const auto &true_lbl_ic = fn.icodes[idx + 2];
    const auto &true_shift_ic = fn.icodes[idx + 3];
    const auto &xor_ic = fn.icodes[idx + 4];
    const auto &true_assign_ic = fn.icodes[idx + 5];
    const auto &goto_ic = fn.icodes[idx + 6];
    const auto &false_lbl_ic = fn.icodes[idx + 7];
    const auto &false_shift_ic = fn.icodes[idx + 8];
    const auto &false_assign_ic = fn.icodes[idx + 9];
    const auto &join_lbl_ic = fn.icodes[idx + 10];
    const auto &store_ic = fn.icodes[idx + 11];

    if (band_ic.op != icode_op::BAND ||
        ifx_ic.op != icode_op::IFX ||
        true_lbl_ic.op != icode_op::LABEL ||
        true_shift_ic.op != icode_op::SHR ||
        xor_ic.op != icode_op::BXOR ||
        true_assign_ic.op != icode_op::ASSIGN ||
        goto_ic.op != icode_op::GOTO ||
        false_lbl_ic.op != icode_op::LABEL ||
        false_shift_ic.op != icode_op::SHR ||
        false_assign_ic.op != icode_op::ASSIGN ||
        join_lbl_ic.op != icode_op::LABEL ||
        store_ic.op != icode_op::ASSIGN) {
        return false;
    }

    if (!band_ic.result.is_temp() ||
        !ifx_ic.left.is_temp() ||
        ifx_ic.left.temp_id != band_ic.result.temp_id ||
        ifx_ic.true_lbl.empty() ||
        ifx_ic.false_lbl.empty() ||
        true_lbl_ic.label_name != ifx_ic.true_lbl ||
        false_lbl_ic.label_name != ifx_ic.false_lbl ||
        goto_ic.label_name.empty() ||
        join_lbl_ic.label_name != goto_ic.label_name) {
        return false;
    }

    operand value = band_ic.left;
    const operand *mask = &band_ic.right;
    if (value.kind == operand_kind::INT_CONST &&
        mask->kind != operand_kind::INT_CONST) {
        value = band_ic.right;
        mask = &band_ic.left;
    }
    if (mask->kind != operand_kind::INT_CONST || mask->ival != 1)
        return false;

    // Byte promotion may expose only the low-byte view used by `(value & 1)`.
    // Recover the full object from the two shifts before validating the fusion.
    if ((!value.type || op_size(value) != 4) &&
        operands_equivalent(value, true_shift_ic.left) &&
        operands_equivalent(value, false_shift_ic.left) &&
        true_shift_ic.left.type &&
        true_shift_ic.left.type->is_integer() &&
        true_shift_ic.left.type->is_unsigned() &&
        op_size(true_shift_ic.left) == 4 &&
        false_shift_ic.left.type &&
        false_shift_ic.left.type->is_integer() &&
        false_shift_ic.left.type->is_unsigned() &&
        op_size(false_shift_ic.left) == 4) {
        value = true_shift_ic.left;
    }
    if (!value.type || !value.type->is_integer() ||
        !value.type->is_unsigned() ||
        op_size(value) != 4) {
        return false;
    }

    auto is_shift_from_value = [&](const icode &ic) {
        return ic.op == icode_op::SHR &&
               operands_equivalent(ic.left, value) &&
               ic.right.kind == operand_kind::INT_CONST &&
               ic.right.ival == 1 &&
               ic.result.is_temp() &&
               op_size(ic.result) == 4;
    };
    if (!is_shift_from_value(true_shift_ic) ||
        !is_shift_from_value(false_shift_ic)) {
        return false;
    }

    const operand *poly = nullptr;
    if (operands_equivalent(xor_ic.left, true_shift_ic.result) &&
        xor_ic.right.kind == operand_kind::INT_CONST) {
        poly = &xor_ic.right;
    } else if (operands_equivalent(xor_ic.right, true_shift_ic.result) &&
               xor_ic.left.kind == operand_kind::INT_CONST) {
        poly = &xor_ic.left;
    } else {
        return false;
    }

    if (!xor_ic.result.is_temp() || op_size(xor_ic.result) != 4)
        return false;
    if (!operands_equivalent(true_assign_ic.left, xor_ic.result) ||
        !true_assign_ic.result.is_temp() ||
        !operands_equivalent(false_assign_ic.result,
                             true_assign_ic.result) ||
        !operands_equivalent(false_assign_ic.left,
                             false_shift_ic.result) ||
        !operands_equivalent(store_ic.left, true_assign_ic.result) ||
        !operands_equivalent(store_ic.result, value)) {
        return false;
    }

    const size_t after_window = idx + 12;
    auto temp_dead_after = [&](const operand &op) {
        return !op.is_temp() ||
               !temp_value_used_after(fn, after_window, op.temp_id);
    };
    if (!temp_dead_after(band_ic.result) ||
        !temp_dead_after(true_shift_ic.result) ||
        !temp_dead_after(xor_ic.result) ||
        !temp_dead_after(true_assign_ic.result) ||
        !temp_dead_after(false_shift_ic.result)) {
        return false;
    }

    auto label_referenced_outside =
        [&](const std::string &label, size_t first, size_t last) {
            for (size_t pos = 0; pos < fn.icodes.size(); ++pos) {
                if (pos >= first && pos <= last)
                    continue;
                const auto &ic = fn.icodes[pos];
                if (ic.op == icode_op::GOTO && ic.label_name == label)
                    return true;
                if (ic.op == icode_op::IFX &&
                    (ic.true_lbl == label || ic.false_lbl == label)) {
                    return true;
                }
            }
            return false;
        };
    if (label_referenced_outside(ifx_ic.true_lbl, idx, idx + 11) ||
        label_referenced_outside(ifx_ic.false_lbl, idx, idx + 11) ||
        label_referenced_outside(join_lbl_ic.label_name, idx, idx + 11)) {
        return false;
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(ifx_ic.line ? ifx_ic.line : band_ic.line);

    const uint32_t poly_bits = static_cast<uint32_t>(poly->ival);
    const uint8_t p0 = static_cast<uint8_t>(poly_bits & 0xffu);
    const uint8_t p1 = static_cast<uint8_t>((poly_bits >> 8) & 0xffu);
    const uint8_t p2 = static_cast<uint8_t>((poly_bits >> 16) & 0xffu);
    const uint8_t p3 = static_cast<uint8_t>((poly_bits >> 24) & 0xffu);
    const std::string done = fresh_local_label("__shiftxor32_done");

    emit_comment("O2 32-bit shift/xor diamond");
    load_de_word(value, 1);
    load_hl_word(value, 0);
    emit_line("srl\td");
    emit_line("rr\te");
    emit_line("rr\th");
    emit_line("rr\tl");
    invalidate_pair_cache();
    invalidate_a_cache();
    emit_line("jr\tnc, %s", done.c_str());

    auto xor_reg = [&](char reg, uint8_t imm) {
        if (imm == 0)
            return;
        emit_line("ld\ta, %c", reg);
        emit_line("xor\t%s", asm_.imm(imm).c_str());
        emit_line("ld\t%c, a", reg);
    };
    xor_reg('l', p0);
    xor_reg('h', p1);
    xor_reg('e', p2);
    xor_reg('d', p3);
    invalidate_pair_cache();
    invalidate_a_cache();

    emit_label(done, false);

    // Adjacent bit-at-a-time recurrences often lower to a chain of these
    // diamonds.  In speed mode, and in size mode where the same rewrite is
    // also strictly smaller, keep the updated
    // word in DEHL while the next *fully validated* diamond consumes the same
    // object.  The recursive matcher performs all of the normal temporary and
    // label-liveness checks before emitting anything; this small precheck only
    // ensures that a different object's update cannot observe stale memory.
    // The last diamond still commits the value once, so calls, aliases, and
    // ordinary intervening IR remain hard barriers.
    if ((opt_settings_.level == opt_level::Of ||
         opt_settings_.level == opt_level::O3 ||
         opt_settings_.level == opt_level::Os) &&
        after_window < fn.icodes.size()) {
        const auto &next_band = fn.icodes[after_window];
        const operand *next_value = nullptr;
        const operand *next_mask = nullptr;
        if (next_band.op == icode_op::BAND) {
            if (next_band.left.kind == operand_kind::INT_CONST) {
                next_mask = &next_band.left;
                next_value = &next_band.right;
            } else {
                next_value = &next_band.left;
                next_mask = &next_band.right;
            }
        }
        if (next_value && next_mask &&
            next_mask->kind == operand_kind::INT_CONST &&
            next_mask->ival == 1 &&
            operands_equivalent(*next_value, value)) {
            set_pair_cache(reg_pair{"hl", 'l', 'h', false},
                           pair_word_cache_key(value, 0));
            set_pair_cache(reg_pair{"de", 'e', 'd', true},
                           pair_word_cache_key(value, 1));
            size_t next_idx = after_window;
            if (try_emit_lsb32_shift_xor_diamond(fn, next_idx)) {
                idx = next_idx;
                return true;
            }
        }
    }

    store_hl_word(value, 0);
    store_de_word(value, 1);

    idx += 11;
    return true;
}

bool z80_gen::try_emit_band_ifx(const ir_function &fn, size_t &idx) {
    if (!compare_ifx_fusion_enabled() || idx + 1 >= fn.icodes.size())
        return false;

    const auto &band_ic = fn.icodes[idx];
    if (band_ic.op != icode_op::BAND)
        return false;
    if (!band_ic.result.is_temp())
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
    if (mask->kind != operand_kind::INT_CONST)
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
                case temp_home::main_d:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('d');
                        return true;
                    }
                    break;
                case temp_home::main_e:
                    if (op.byte_offset == 0) {
                        emit_reg_bit('e');
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

        if (op.kind == operand_kind::TEMP ||
            (op.kind == operand_kind::SYMBOL && !op.is_global)) {
            if (op.kind == operand_kind::TEMP) {
                auto ri = temp_regs_.find(op.temp_id);
                if (ri != temp_regs_.end() &&
                    !temp_home_uses_spill_slot(ri->second)) {
                    return false;
                }
            }
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

    const int value_size = op_size(*value);
    if (value_size <= 0)
        return false;

    const int mask_width = std::min(value_size, 8);
    uint64_t mask_bits = static_cast<uint64_t>(mask->ival);
    if (mask_width < 8) {
        const unsigned width_bits = static_cast<unsigned>(mask_width * 8);
        mask_bits &= (UINT64_C(1) << width_bits) - 1;
    }

    const uint8_t imm = static_cast<uint8_t>(mask_bits & 0xffu);
    if (imm == 0) {
        const bool wide_zero = (mask_bits == 0);
        if (wide_zero) {
            emit_const_truth(false);
            idx += consume_count;
            return true;
        }
    }

    if (mask_bits != 0 && (mask_bits & (mask_bits - 1u)) == 0) {
        int bit = 0;
        while (((mask_bits >> bit) & 1u) == 0u)
            ++bit;
        operand byte_value = *value;
        byte_value.byte_offset += bit / 8;
        if (emit_bit_test(byte_value, bit % 8)) {
            emit_truth_branch(true);
            idx += consume_count;
            return true;
        }
    }

    if (value_size != 1)
        return false;

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

bool z80_gen::try_emit_msb_byte_shift_xor_diamonds(
    const ir_function &fn, size_t &idx) {
    if (!compare_ifx_fusion_enabled())
        return false;

    struct byte_step {
        operand value;
        operand target;
        operand polynomial;
        size_t last_index = 0;
        std::string true_label;
        std::string false_label;
        std::string join_label;
    };

    auto label_referenced_outside =
        [&](const std::string &label, size_t first, size_t last) {
            for (size_t pos = 0; pos < fn.icodes.size(); ++pos) {
                if (pos >= first && pos <= last)
                    continue;
                const icode &ic = fn.icodes[pos];
                if (ic.op == icode_op::GOTO && ic.label_name == label)
                    return true;
                if (ic.op == icode_op::IFX &&
                    (ic.true_lbl == label || ic.false_lbl == label)) {
                    return true;
                }
            }
            return false;
    };

    auto match_step = [&](size_t start, byte_step &step) {
        // The join may feed a byte store/copy, or copy propagation may feed
        // the selected temporary straight into RETURN.  The latter compact
        // form has ten icodes including the return.
        if (start + 9 >= fn.icodes.size())
            return false;

        const icode &band = fn.icodes[start];
        const icode &branch = fn.icodes[start + 1];
        const icode &true_label = fn.icodes[start + 2];
        if (band.op != icode_op::BAND || !band.result.is_temp() ||
            branch.op != icode_op::IFX || !branch.left.is_temp() ||
            branch.left.temp_id != band.result.temp_id ||
            branch.true_lbl.empty() || branch.false_lbl.empty() ||
            true_label.op != icode_op::LABEL ||
            true_label.label_name != branch.true_lbl) {
            return false;
        }

        operand value = band.left;
        const operand *mask = &band.right;
        if (value.kind == operand_kind::INT_CONST &&
            mask->kind != operand_kind::INT_CONST) {
            value = band.right;
            mask = &band.left;
        }
        if (mask->kind != operand_kind::INT_CONST || mask->ival != 0x80 ||
            !value.type || !value.type->is_integer() ||
            !value.type->is_unsigned() || op_size(value) != 1) {
            return false;
        }

        struct arm_result {
            operand value;
            std::vector<int> temps;
        };
        auto parse_shift_arm = [&](size_t &pos, arm_result &arm) {
            operand shift_input = value;
            if (pos < fn.icodes.size() &&
                fn.icodes[pos].op == icode_op::CAST &&
                fn.icodes[pos].result.is_temp() &&
                operands_equivalent(fn.icodes[pos].left, value) &&
                fn.icodes[pos].result.type &&
                fn.icodes[pos].result.type->is_integer() &&
                op_size(fn.icodes[pos].result) == 2) {
                shift_input = fn.icodes[pos].result;
                arm.temps.push_back(shift_input.temp_id);
                ++pos;
            }
            if (pos >= fn.icodes.size())
                return false;
            const icode &shift = fn.icodes[pos++];
            if (shift.op != icode_op::SHL || !shift.result.is_temp() ||
                shift.right.kind != operand_kind::INT_CONST ||
                shift.right.ival != 1 ||
                !operands_equivalent(shift.left, shift_input)) {
                return false;
            }
            arm.value = shift.result;
            arm.temps.push_back(shift.result.temp_id);
            if (pos < fn.icodes.size() &&
                fn.icodes[pos].op == icode_op::CAST &&
                fn.icodes[pos].result.is_temp() &&
                fn.icodes[pos].left.is_temp() &&
                fn.icodes[pos].left.temp_id == shift.result.temp_id &&
                fn.icodes[pos].result.type &&
                fn.icodes[pos].result.type->is_integer() &&
                fn.icodes[pos].result.type->is_unsigned() &&
                op_size(fn.icodes[pos].result) == 1) {
                arm.value = fn.icodes[pos].result;
                arm.temps.push_back(arm.value.temp_id);
                ++pos;
            }
            return true;
        };

        size_t pos = start + 3;
        arm_result true_arm;
        if (!parse_shift_arm(pos, true_arm) || pos >= fn.icodes.size())
            return false;
        const icode &true_xor = fn.icodes[pos++];
        if (true_xor.op != icode_op::BXOR || !true_xor.result.is_temp())
            return false;

        const operand *polynomial = nullptr;
        if (true_xor.left.is_temp() &&
            true_arm.value.is_temp() &&
            true_xor.left.temp_id == true_arm.value.temp_id) {
            polynomial = &true_xor.right;
        } else if (true_xor.right.is_temp() &&
                   true_arm.value.is_temp() &&
                   true_xor.right.temp_id == true_arm.value.temp_id) {
            polynomial = &true_xor.left;
        }
        const bool byte_polynomial = polynomial &&
            ((polynomial->kind == operand_kind::INT_CONST &&
              polynomial->ival >= 0 && polynomial->ival <= 0xff) ||
             (polynomial->type && polynomial->type->is_integer() &&
              op_size(*polynomial) == 1 &&
              !polynomial->type->is_volatile));
        if (!byte_polynomial || pos >= fn.icodes.size()) {
            return false;
        }

        const icode &true_goto = fn.icodes[pos++];
        if (true_goto.op != icode_op::GOTO || pos >= fn.icodes.size())
            return false;
        const icode &false_label = fn.icodes[pos++];
        if (false_label.op != icode_op::LABEL ||
            false_label.label_name != branch.false_lbl)
            return false;

        arm_result false_arm;
        if (!parse_shift_arm(pos, false_arm) || pos >= fn.icodes.size())
            return false;
        if (fn.icodes[pos].op == icode_op::ASSIGN) {
            const icode &false_assign = fn.icodes[pos++];
            if (!false_assign.result.is_temp() ||
                false_assign.result.temp_id != true_xor.result.temp_id ||
                !false_assign.left.is_temp() ||
                !false_arm.value.is_temp() ||
                false_assign.left.temp_id != false_arm.value.temp_id ||
                !false_assign.right.is_none() || pos >= fn.icodes.size()) {
                return false;
            }
        } else if (!false_arm.value.is_temp() ||
                   false_arm.value.temp_id != true_xor.result.temp_id) {
            // Copy propagation may assign the false shift directly to the
            // join value, eliminating the otherwise canonical phi copy.
            return false;
        }

        const icode &join_label = fn.icodes[pos++];
        if (join_label.op != icode_op::LABEL ||
            join_label.label_name != true_goto.label_name ||
            pos >= fn.icodes.size())
            return false;
        operand target;
        bool terminal_return = false;
        const icode &consumer = fn.icodes[pos];
        if (consumer.op == icode_op::RETURN) {
            if (!consumer.left.is_temp() ||
                consumer.left.temp_id != true_xor.result.temp_id ||
                !consumer.left.type || op_size(consumer.left) != 1 ||
                consumer.left.type->is_volatile) {
                return false;
            }
            // Do not consume RETURN.  Materialize its already allocated
            // input home, then let ordinary return emission handle the ABI
            // move and epilogue on the next iteration.
            target = consumer.left;
            terminal_return = true;
        } else {
            const icode &store = fn.icodes[pos++];
            if ((store.op != icode_op::ASSIGN &&
                 store.op != icode_op::CAST) ||
                !store.left.is_temp() ||
                store.left.temp_id != true_xor.result.temp_id ||
                !store.right.is_none() ||
                !store.result.type || op_size(store.result) != 1 ||
                store.result.type->is_volatile) {
                return false;
            }
            target = store.result;
        }

        const size_t last = pos - 1;
        if ((value.type && value.type->is_volatile) ||
            (target.type && target.type->is_volatile) ||
            temp_value_used_after(fn, last + 1, band.result.temp_id) ||
            temp_value_used_after(fn,
                                  last + (terminal_return ? 2 : 1),
                                  true_xor.result.temp_id))
            return false;
        for (int temp : true_arm.temps) {
            const size_t after = last +
                ((terminal_return &&
                  temp == true_xor.result.temp_id) ? 2 : 1);
            if (temp_value_used_after(fn, after, temp))
                return false;
        }
        for (int temp : false_arm.temps) {
            const size_t after = last +
                ((terminal_return &&
                  temp == true_xor.result.temp_id) ? 2 : 1);
            if (temp_value_used_after(fn, after, temp))
                return false;
        }

        if (label_referenced_outside(branch.true_lbl, start, last) ||
            label_referenced_outside(branch.false_lbl, start, last) ||
            label_referenced_outside(join_label.label_name, start, last))
            return false;

        step.value = value;
        step.target = target;
        step.polynomial = *polynomial;
        step.last_index = last;
        step.true_label = branch.true_lbl;
        step.false_label = branch.false_lbl;
        step.join_label = join_label.label_name;
        return true;
    };

    byte_step first;
    if (!match_step(idx, first))
        return false;

    std::vector<byte_step> steps;
    steps.push_back(first);
    size_t next = first.last_index + 1;
    while (next < fn.icodes.size()) {
        byte_step candidate;
        if (!match_step(next, candidate) ||
            !operands_equivalent(candidate.value, first.target) ||
            !operands_equivalent(candidate.target, first.target)) {
            break;
        }
        steps.push_back(candidate);
        next = candidate.last_index + 1;
    }

    struct xor_source {
        enum class kind { immediate, reg, ix } source_kind;
        int value = 0;
        char reg = 0;
    };
    auto register_for_home = [](temp_home home, int byte_offset) -> char {
        switch (home) {
        case temp_home::main_b:
            return byte_offset == 0 ? 'b' : 0;
        case temp_home::main_c:
            return byte_offset == 0 ? 'c' : 0;
        case temp_home::main_d:
            return byte_offset == 0 ? 'd' : 0;
        case temp_home::main_e:
            return byte_offset == 0 ? 'e' : 0;
        case temp_home::main_bc:
            return byte_offset == 0 ? 'c' :
                   byte_offset == 1 ? 'b' : 0;
        case temp_home::main_de:
            return byte_offset == 0 ? 'e' :
                   byte_offset == 1 ? 'd' : 0;
        case temp_home::main_hl:
        case temp_home::arg_hl:
            return byte_offset == 0 ? 'l' :
                   byte_offset == 1 ? 'h' : 0;
        case temp_home::arg_l:
            return byte_offset == 0 ? 'l' : 0;
        case temp_home::arg_de:
            return byte_offset == 0 ? 'e' :
                   byte_offset == 1 ? 'd' : 0;
        default:
            return 0;
        }
    };
    auto classify_xor_source = [&](const operand &op,
                                   xor_source &source) -> bool {
        if (op.kind == operand_kind::INT_CONST) {
            source.source_kind = xor_source::kind::immediate;
            source.value = static_cast<int>(op.ival & 0xff);
            return true;
        }

        temp_home home = temp_home::stack;
        bool have_home = false;
        if (op.is_temp()) {
            auto it = temp_regs_.find(op.temp_id);
            if (it != temp_regs_.end()) {
                home = it->second;
                have_home = true;
            }
        } else if (op.is_symbol() && !op.is_global) {
            auto reg = symbol_regs_.find(symbol_reg_key(op));
            if (reg != symbol_regs_.end()) {
                home = reg->second;
                have_home = true;
            } else {
                auto incoming = incoming_symbol_homes_.find(op.stack_offset);
                if (incoming != incoming_symbol_homes_.end()) {
                    home = incoming->second;
                    have_home = true;
                }
            }
        }

        if (have_home) {
            const char reg = register_for_home(home, op.byte_offset);
            if (reg != 0) {
                source.source_kind = xor_source::kind::reg;
                source.reg = reg;
                return true;
            }
            if (home != temp_home::stack)
                return false;
        }

        if ((op.is_temp() || (op.is_symbol() && !op.is_global)) &&
            fits_ix_disp(ix_offset_of(op))) {
            source.source_kind = xor_source::kind::ix;
            source.value = ix_offset_of(op);
            return true;
        }
        return false;
    };

    std::vector<xor_source> xor_sources(steps.size());
    for (size_t step_index = 0; step_index < steps.size(); ++step_index) {
        if (!classify_xor_source(steps[step_index].polynomial,
                                 xor_sources[step_index])) {
            return false;
        }
    }

    // A rematerialized HL value or TLS lookup may overwrite a polynomial
    // register before the first XOR.  Ordinary byte/register/frame loads do
    // not, and subsequent steps already keep their value in A.
    if (!xor_sources.empty() &&
        xor_sources.front().source_kind == xor_source::kind::reg) {
        const char poly_reg = xor_sources.front().reg;
        if (first.value.kind == operand_kind::SYMBOL &&
            first.value.is_global && first.value.is_tls) {
            return false;
        }
        if ((poly_reg == 'h' || poly_reg == 'l') &&
            first.value.is_temp()) {
            auto value_home = temp_regs_.find(first.value.temp_id);
            if (value_home != temp_regs_.end() &&
                value_home->second == temp_home::remat_hl) {
                return false;
            }
        }
    }

    cur_ic_index_ = idx;
    if (debug_)
        debug_->emit_location(fn.icodes[idx].line);
    invalidate_pair_cache();
    invalidate_a_cache();
    load_a(first.value);
    for (size_t step_index = 0; step_index < steps.size(); ++step_index) {
        const xor_source &source = xor_sources[step_index];
        const std::string no_xor =
            fresh_local_label("__shiftxor8_done");
        emit_line("add\ta, a");
        emit_line("jr\tnc, %s", no_xor.c_str());
        if (source.source_kind == xor_source::kind::immediate) {
            if (source.value != 0)
                emit_line("xor\t%s", asm_.imm(source.value).c_str());
        } else if (source.source_kind == xor_source::kind::reg) {
            emit_line("xor\ta, %c", source.reg);
        } else {
            emit_line("xor\ta, %s", asm_.ix_rel(source.value).c_str());
        }
        emit_label(no_xor, false);
    }
    // This is a synthetic definition at the matched diamond's join, not the
    // BAND at which the matcher started.  Dead-store analysis must therefore
    // scan from the join; scanning from idx sees the original arm definitions
    // as later overwrites and can incorrectly discard this materialization.
    cur_ic_index_ = steps.back().last_index;
    store_a(first.target);
    invalidate_pair_cache();
    idx = steps.back().last_index;
    cur_ic_index_ = idx;
    return true;
}

bool z80_gen::try_emit_switch_jump_table(const ir_function &fn, size_t &idx) {
    if (!switch_jump_tables_enabled() || idx + 2 >= fn.icodes.size())
        return false;

    cur_ic_index_ = idx;

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

    operand byte_cond = cond;
    const bool cond_is_byte = op_size(cond) == 1;
    const bool cond_is_zero_extended_byte =
        cond_is_byte || get_zero_extended_u8_source(cond, byte_cond);

    if (cond_is_zero_extended_byte && (min_value < 0 || max_value > 0xFF))
        return false;

    std::vector<std::string> table_labels(span, tail.label_name);
    for (const auto &entry : cases)
        table_labels[static_cast<size_t>(entry.value - min_value)] = entry.label;

    if (debug_)
        debug_->emit_location(fn.icodes[idx].line);

    emit_comment("optimized jump-table switch (%zu cases, span=%zu)",
                 cases.size(), span);

    if (cond_is_zero_extended_byte) {
        if (!cond_is_byte && operands_equivalent(byte_cond, cond)) {
            load_hl(cond);
            emit_line("ld\ta, l");
        } else {
            load_a(byte_cond);
        }
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
    if (ic.op != icode_op::ENDFUNCTION)
        last_frameless_return_terminated_ = false;
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
    case icode_op::BLOCK_FILL:    gen_block_fill(ic);   break;
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
