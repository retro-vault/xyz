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
    // the Z80's 64 KiB address space (MD5's fully unrolled transform is the
    // practical example).  Retain the speed-profile IR but select compact
    // backend forms for such a function so the generated program is viable.
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

    // This selector also owns a small set of target-independent, frameless
    // leaf-loop forms that are enabled in the tuned profiles even while the
    // legacy SDCC-style helper specializations remain disabled.
    if (tuned_profile_enabled() && try_emit_sdcc_style_helper(fn)) {
        cur_fn_ = nullptr;
        compact_codegen_ = false;
        return;
    }

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
        if (try_emit_compare_ifx(fn, i))
            continue;
      }
        gen_icode(fn.icodes[i]);
    }

    cur_fn_ = nullptr;
    compact_codegen_ = false;
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
        asm_.symbol_type_function(lbl);
        asm_.label(lbl, fn.is_global);
    };
    auto emit_helper_footer = [&]() {
        const std::string lbl = mangle(fn.name);
        asm_.symbol_size(lbl, ". - " + lbl);
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

    // A widened unsigned 16x16 multiply narrowed after an eight-bit shift is
    // a complete register-to-register leaf on Z80.  Select it before frame
    // planning so the two incoming register arguments are never spilled for
    // virtual 32-bit intermediates that the runtime helper already returns in
    // registers.  The match is purely structural and applies to ordinary
    // Q-format, colour, checksum and scaling helpers alike.
    auto match_frameless_u16_mul_shr8 = [&]() -> bool {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL1 ||
            fn.local_bytes != 0 || fn.stack_param_bytes != 0 ||
            body.size() != 6 || !fn.ret_type || fn.ret_type->size() != 2) {
            return false;
        }
        const icode &recv_left = *body[0];
        const icode &recv_right = *body[1];
        const icode &mul = *body[2];
        const icode &shift = *body[3];
        const icode &narrow = *body[4];
        const icode &ret = *body[5];
        if (recv_left.op != icode_op::RECEIVE ||
            recv_left.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_left.result) ||
            recv_right.op != icode_op::RECEIVE ||
            recv_right.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_right.result) ||
            !recv_left.result.type || !recv_right.result.type ||
            !recv_left.result.type->is_unsigned() ||
            !recv_right.result.type->is_unsigned() ||
            mul.op != icode_op::MUL || !mul.result.is_temp() ||
            op_size(mul.result) != 4 ||
            !operands_equivalent(mul.left, recv_left.result) ||
            !operands_equivalent(mul.right, recv_right.result) ||
            shift.op != icode_op::SHR || !shift.result.is_temp() ||
            !operands_equivalent(shift.left, mul.result) ||
            !is_exact_int_const(shift.right, 8) ||
            !shift.left.type || !shift.left.type->is_unsigned() ||
            narrow.op != icode_op::CAST ||
            !is_word_temp(narrow.result) ||
            !operands_equivalent(narrow.left, shift.result) ||
            ret.op != icode_op::RETURN ||
            !operands_equivalent(ret.left, narrow.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("frameless unsigned 16x16 multiply, shift-8 narrow");
        asm_.global_decl("___muluint2ulong");
        emit_line("call\t___muluint2ulong");
        emit_line("ld\ta, l");
        emit_line("ld\tl, d");
        emit_line("ld\th, a");
        emit_line("ex\tde, hl");
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    // Select complete frameless leaf loops before conservative TEMP-frame
    // planning gets a chance to reserve slots for values that never need to
    // exist in memory.  These are structural IR forms, independent of source
    // names: an advancing byte cursor returning its distance, two advancing
    // cursors returning the first byte difference, and a copy-until-zero
    // loop.  Treating the whole loop as one allocation region also lets HL,
    // DE and BC carry multiple live pointers without an IX frame.
    auto match_frameless_byte_distance = [&]() -> bool {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL1 ||
            fn.local_bytes != 0 || fn.stack_param_bytes != 0 ||
            body.size() != 13 || !fn.ret_type || fn.ret_type->size() != 2) {
            return false;
        }

        const auto &recv = *body[0];
        const auto &cursor_init = *body[1];
        const auto &loop = *body[2];
        const auto &load = *body[3];
        const auto &truth_cast = *body[4];
        const auto &branch = *body[5];
        const auto &advance_label = *body[6];
        const auto &advance = *body[7];
        const auto &cursor_store = *body[8];
        const auto &backedge = *body[9];
        const auto &done = *body[10];
        const auto &distance = *body[11];
        const auto &ret = *body[12];

        if (recv.op != icode_op::RECEIVE ||
            recv.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv.result) ||
            cursor_init.op != icode_op::ASSIGN ||
            !is_word_temp(cursor_init.result) ||
            !operands_equivalent(cursor_init.left, recv.result) ||
            loop.op != icode_op::LABEL ||
            load.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load.result) ||
            !operands_equivalent(load.left, cursor_init.result) ||
            !load.right.is_none() ||
            truth_cast.op != icode_op::CAST ||
            !is_word_temp(truth_cast.result) ||
            !operands_equivalent(truth_cast.left, load.result) ||
            branch.op != icode_op::IFX ||
            !operands_equivalent(branch.left, truth_cast.result) ||
            advance_label.op != icode_op::LABEL ||
            advance_label.label_name != branch.true_lbl ||
            advance.op != icode_op::ADD ||
            !is_word_temp(advance.result) ||
            !operands_equivalent(advance.left, cursor_init.result) ||
            !is_exact_int_const(advance.right, 1) ||
            cursor_store.op != icode_op::ASSIGN ||
            !operands_equivalent(cursor_store.result, cursor_init.result) ||
            !operands_equivalent(cursor_store.left, advance.result) ||
            backedge.op != icode_op::GOTO ||
            backedge.label_name != loop.label_name ||
            done.op != icode_op::LABEL ||
            done.label_name != branch.false_lbl ||
            distance.op != icode_op::SUB ||
            !is_word_temp(distance.result) ||
            !operands_equivalent(distance.left, cursor_init.result) ||
            !operands_equivalent(distance.right, recv.result) ||
            ret.op != icode_op::RETURN ||
            !operands_equivalent(ret.left, distance.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("frameless byte-cursor distance loop");
        emit_line("ld\tb, h");
        emit_line("ld\tc, l");
        emit_label(loop.label_name, false);
        emit_line("ld\ta, (bc)");
        emit_line("or\ta, a");
        emit_line("jr\tz, %s", done.label_name.c_str());
        emit_line("inc\tbc");
        emit_line("jr\t%s", loop.label_name.c_str());
        emit_label(done.label_name, false);
        emit_line("ld\ta, c");
        emit_line("sub\tl");
        emit_line("ld\te, a");
        emit_line("ld\ta, b");
        emit_line("sbc\ta, h");
        emit_line("ld\td, a");
        emit_line("ret");
        emit_helper_footer();
        iy_preserving_local_callees_.insert(fn.name);
        return true;
    };

    auto match_frameless_byte_compare = [&]() -> bool {
        const bool reloaded_byte_difference =
            body.size() == 22 && body[7]->op == icode_op::GET_VALUE_AT &&
            body[8]->op == icode_op::GET_VALUE_AT &&
            body[9]->op == icode_op::EQ;
        const bool direct_byte_difference =
            body.size() == 20 || reloaded_byte_difference;
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL1 ||
            fn.local_bytes != 0 || fn.stack_param_bytes != 0 ||
            (!direct_byte_difference && body.size() != 22) ||
            !fn.ret_type || fn.ret_type->size() != 2) {
            return false;
        }

        const auto &recv_a = *body[0];
        const auto &recv_b = *body[1];
        const auto &loop = *body[2];
        const auto &load_a = *body[3];
        const auto &truth_cast = *body[4];
        const auto &truth_branch = *body[5];
        const auto &compare_label = *body[6];
        const icode &compare_a =
            reloaded_byte_difference ? *body[7] : load_a;
        const auto &load_b = *body[reloaded_byte_difference ? 8 : 7];
        const auto &equal = *body[reloaded_byte_difference ? 9 : 8];
        const auto &equal_branch =
            *body[reloaded_byte_difference ? 10 : 9];
        const auto &advance_label =
            *body[reloaded_byte_difference ? 11 : 10];
        const auto &advance_a =
            *body[reloaded_byte_difference ? 12 : 11];
        const auto &store_a = *body[reloaded_byte_difference ? 13 : 12];
        const auto &advance_b =
            *body[reloaded_byte_difference ? 14 : 13];
        const auto &store_b = *body[reloaded_byte_difference ? 15 : 14];
        const auto &backedge = *body[reloaded_byte_difference ? 16 : 15];
        const auto &done = *body[reloaded_byte_difference ? 17 : 16];
        const icode *final_a =
            reloaded_byte_difference ? body[18] : nullptr;
        const auto &final_b = *body[reloaded_byte_difference ? 19 : 17];
        const icode *cast_a =
            direct_byte_difference ? nullptr : body[18];
        const icode *cast_b =
            direct_byte_difference ? nullptr : body[19];
        const auto &difference =
            *body[body.size() == 20 ? 18 : 20];
        const auto &ret = *body[body.size() == 20 ? 19 : 21];

        if (recv_a.op != icode_op::RECEIVE ||
            recv_a.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_a.result) ||
            recv_b.op != icode_op::RECEIVE ||
            recv_b.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_b.result) ||
            loop.op != icode_op::LABEL ||
            load_a.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_a.result) ||
            !operands_equivalent(load_a.left, recv_a.result) ||
            !load_a.right.is_none() ||
            truth_cast.op != icode_op::CAST ||
            !is_word_temp(truth_cast.result) ||
            !operands_equivalent(truth_cast.left, load_a.result) ||
            truth_branch.op != icode_op::IFX ||
            !operands_equivalent(truth_branch.left, truth_cast.result) ||
            compare_label.op != icode_op::LABEL ||
            compare_label.label_name != truth_branch.true_lbl ||
            (reloaded_byte_difference &&
             (compare_a.op != icode_op::GET_VALUE_AT ||
              !is_byte_temp(compare_a.result) ||
              !operands_equivalent(compare_a.left, recv_a.result) ||
              !compare_a.right.is_none())) ||
            load_b.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_b.result) ||
            !operands_equivalent(load_b.left, recv_b.result) ||
            !load_b.right.is_none() ||
            equal.op != icode_op::EQ || !is_byte_temp(equal.left) ||
            !((operands_equivalent(equal.left, compare_a.result) &&
               operands_equivalent(equal.right, load_b.result)) ||
              (operands_equivalent(equal.right, compare_a.result) &&
               operands_equivalent(equal.left, load_b.result))) ||
            equal_branch.op != icode_op::IFX ||
            !operands_equivalent(equal_branch.left, equal.result) ||
            advance_label.op != icode_op::LABEL ||
            advance_label.label_name != equal_branch.true_lbl ||
            advance_a.op != icode_op::ADD ||
            !is_word_temp(advance_a.result) ||
            !operands_equivalent(advance_a.left, recv_a.result) ||
            !is_exact_int_const(advance_a.right, 1) ||
            store_a.op != icode_op::ASSIGN ||
            !operands_equivalent(store_a.result, recv_a.result) ||
            !operands_equivalent(store_a.left, advance_a.result) ||
            advance_b.op != icode_op::ADD ||
            !is_word_temp(advance_b.result) ||
            !operands_equivalent(advance_b.left, recv_b.result) ||
            !is_exact_int_const(advance_b.right, 1) ||
            store_b.op != icode_op::ASSIGN ||
            !operands_equivalent(store_b.result, recv_b.result) ||
            !operands_equivalent(store_b.left, advance_b.result) ||
            backedge.op != icode_op::GOTO ||
            backedge.label_name != loop.label_name ||
            done.op != icode_op::LABEL ||
            done.label_name != truth_branch.false_lbl ||
            done.label_name != equal_branch.false_lbl ||
            final_b.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(final_b.result) ||
            !operands_equivalent(final_b.left, recv_b.result) ||
            !final_b.right.is_none() ||
            (reloaded_byte_difference &&
             (final_a->op != icode_op::GET_VALUE_AT ||
              !is_byte_temp(final_a->result) ||
              !operands_equivalent(final_a->left, recv_a.result) ||
              !final_a->right.is_none())) ||
            (!direct_byte_difference &&
             (cast_a->op != icode_op::CAST ||
              !is_word_temp(cast_a->result) ||
              !operands_equivalent(cast_a->left, load_a.result) ||
              cast_b->op != icode_op::CAST ||
              !is_word_temp(cast_b->result) ||
              !operands_equivalent(cast_b->left, final_b.result))) ||
            difference.op != icode_op::SUB ||
            !is_word_temp(difference.result) ||
            !operands_equivalent(
                difference.left,
                reloaded_byte_difference
                    ? final_a->result
                    : (direct_byte_difference ? load_a.result
                                              : cast_a->result)) ||
            !operands_equivalent(
                difference.right,
                direct_byte_difference ? final_b.result : cast_b->result) ||
            ret.op != icode_op::RETURN ||
            !operands_equivalent(ret.left, difference.result)) {
            return false;
        }

        emit_helper_header();
        emit_comment("frameless dual byte-cursor compare loop");
        emit_line("ld\tb, h");
        emit_line("ld\tc, l");
        emit_label(loop.label_name, false);
        emit_line("ld\ta, (bc)");
        emit_line("ld\tl, a");
        emit_line("or\ta, a");
        emit_line("jr\tz, %s", done.label_name.c_str());
        emit_line("ld\ta, (de)");
        emit_line("cp\tl");
        emit_line("jr\tnz, %s", done.label_name.c_str());
        emit_line("inc\tbc");
        emit_line("inc\tde");
        emit_line("jr\t%s", loop.label_name.c_str());
        emit_label(done.label_name, false);
        emit_line("ld\ta, (de)");
        emit_line("ld\th, a");
        emit_line("ld\ta, l");
        emit_line("sub\th");
        emit_line("ld\te, a");
        emit_line("sbc\ta, a");
        emit_line("ld\td, a");
        emit_line("ret");
        emit_helper_footer();
        iy_preserving_local_callees_.insert(fn.name);
        return true;
    };

    auto match_frameless_byte_copy_until_zero = [&]() -> bool {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL1 ||
            fn.local_bytes != 0 || fn.stack_param_bytes != 0 ||
            body.size() != 13 || !fn.ret_type ||
            fn.ret_type->kind != type_kind::VOID) {
            return false;
        }

        const auto &recv_dst = *body[0];
        const auto &recv_src = *body[1];
        const auto &loop = *body[2];
        const auto &old_src = *body[3];
        const auto &advance_src = *body[4];
        const auto &store_src = *body[5];
        const auto &load = *body[6];
        const auto &copy = *body[7];
        const auto &advance_dst = *body[8];
        const auto &store_dst = *body[9];
        const auto &truth_cast = *body[10];
        const auto &branch = *body[11];
        const auto &done = *body[12];

        if (recv_dst.op != icode_op::RECEIVE ||
            recv_dst.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_dst.result) ||
            recv_src.op != icode_op::RECEIVE ||
            recv_src.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_src.result) ||
            loop.op != icode_op::LABEL ||
            old_src.op != icode_op::ASSIGN ||
            !is_word_temp(old_src.result) ||
            !operands_equivalent(old_src.left, recv_src.result) ||
            advance_src.op != icode_op::ADD ||
            !is_word_temp(advance_src.result) ||
            !operands_equivalent(advance_src.left, recv_src.result) ||
            !is_exact_int_const(advance_src.right, 1) ||
            store_src.op != icode_op::ASSIGN ||
            !operands_equivalent(store_src.result, recv_src.result) ||
            !operands_equivalent(store_src.left, advance_src.result) ||
            load.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load.result) ||
            !operands_equivalent(load.left, old_src.result) ||
            !load.right.is_none() ||
            copy.op != icode_op::SET_VALUE_AT ||
            !operands_equivalent(copy.result, recv_dst.result) ||
            !operands_equivalent(copy.left, load.result) ||
            !copy.right.is_none() ||
            advance_dst.op != icode_op::ADD ||
            !is_word_temp(advance_dst.result) ||
            !operands_equivalent(advance_dst.left, recv_dst.result) ||
            !is_exact_int_const(advance_dst.right, 1) ||
            store_dst.op != icode_op::ASSIGN ||
            !operands_equivalent(store_dst.result, recv_dst.result) ||
            !operands_equivalent(store_dst.left, advance_dst.result) ||
            truth_cast.op != icode_op::CAST ||
            !is_word_temp(truth_cast.result) ||
            !operands_equivalent(truth_cast.left, load.result) ||
            branch.op != icode_op::IFX ||
            !operands_equivalent(branch.left, truth_cast.result) ||
            branch.true_lbl != loop.label_name ||
            done.op != icode_op::LABEL ||
            done.label_name != branch.false_lbl) {
            return false;
        }

        emit_helper_header();
        emit_comment("frameless copy-until-zero byte loop");
        emit_line("ld\tb, d");
        emit_line("ld\tc, e");
        emit_label(loop.label_name, false);
        emit_line("ld\ta, (bc)");
        emit_line("inc\tbc");
        emit_line("ld\t(hl), a");
        emit_line("inc\thl");
        emit_line("or\ta, a");
        emit_line("jr\tnz, %s", loop.label_name.c_str());
        emit_line("ret");
        emit_helper_footer();
        iy_preserving_local_callees_.insert(fn.name);
        return true;
    };

    auto match_frameless_fixed_byte_equality = [&]() -> bool {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL1 ||
            fn.stack_param_bytes != 0 || body.size() != 24 ||
            !fn.ret_type || fn.ret_type->size() != 2) {
            return false;
        }

        const auto &recv_a = *body[0];
        const auto &recv_b = *body[1];
        const auto &index_init = *body[2];
        const auto &cursor_a_init = *body[3];
        const auto &cursor_b_init = *body[4];
        const auto &loop = *body[5];
        const auto &bound_cmp = *body[6];
        const auto &bound_branch = *body[7];
        const auto &body_label = *body[8];
        const auto &load_a = *body[9];
        const auto &load_b = *body[10];
        const auto &different = *body[11];
        const auto &different_branch = *body[12];
        const auto &different_label = *body[13];
        const auto &return_false = *body[14];
        const auto &step_label = *body[15];
        const auto &index_step = *body[16];
        const auto &cursor_b_step = *body[17];
        const auto &cursor_b_store = *body[18];
        const auto &cursor_a_step = *body[19];
        const auto &cursor_a_store = *body[20];
        const auto &backedge = *body[21];
        const auto &done = *body[22];
        const auto &return_true = *body[23];

        if (recv_a.op != icode_op::RECEIVE ||
            recv_a.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_a.result) ||
            recv_b.op != icode_op::RECEIVE ||
            recv_b.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_b.result) ||
            index_init.op != icode_op::ASSIGN ||
            !index_init.result.is_temp() ||
            !is_exact_int_const(index_init.left, 0) ||
            cursor_a_init.op != icode_op::ASSIGN ||
            !is_word_temp(cursor_a_init.result) ||
            !operands_equivalent(cursor_a_init.left, recv_a.result) ||
            cursor_b_init.op != icode_op::ASSIGN ||
            !is_word_temp(cursor_b_init.result) ||
            !operands_equivalent(cursor_b_init.left, recv_b.result) ||
            loop.op != icode_op::LABEL ||
            bound_cmp.op != icode_op::LT ||
            !operands_equivalent(bound_cmp.left, index_init.result) ||
            bound_cmp.right.kind != operand_kind::INT_CONST ||
            bound_cmp.right.ival <= 0 || bound_cmp.right.ival > 255 ||
            bound_branch.op != icode_op::IFX ||
            !operands_equivalent(bound_branch.left, bound_cmp.result) ||
            body_label.op != icode_op::LABEL ||
            body_label.label_name != bound_branch.true_lbl ||
            load_a.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_a.result) ||
            !operands_equivalent(load_a.left, cursor_a_init.result) ||
            !load_a.right.is_none() ||
            load_b.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_b.result) ||
            !operands_equivalent(load_b.left, cursor_b_init.result) ||
            !load_b.right.is_none() ||
            different.op != icode_op::NE ||
            !((operands_equivalent(different.left, load_a.result) &&
               operands_equivalent(different.right, load_b.result)) ||
              (operands_equivalent(different.right, load_a.result) &&
               operands_equivalent(different.left, load_b.result))) ||
            different_branch.op != icode_op::IFX ||
            !operands_equivalent(different_branch.left, different.result) ||
            different_label.op != icode_op::LABEL ||
            different_label.label_name != different_branch.true_lbl ||
            return_false.op != icode_op::RETURN ||
            !is_exact_int_const(return_false.left, 0) ||
            step_label.op != icode_op::LABEL ||
            step_label.label_name != different_branch.false_lbl ||
            index_step.op != icode_op::ADD ||
            !operands_equivalent(index_step.result, index_init.result) ||
            !operands_equivalent(index_step.left, index_init.result) ||
            !is_exact_int_const(index_step.right, 1) ||
            cursor_b_step.op != icode_op::ADD ||
            !is_word_temp(cursor_b_step.result) ||
            !operands_equivalent(cursor_b_step.left, cursor_b_init.result) ||
            !is_exact_int_const(cursor_b_step.right, 1) ||
            cursor_b_store.op != icode_op::ASSIGN ||
            !operands_equivalent(cursor_b_store.result, cursor_b_init.result) ||
            !operands_equivalent(cursor_b_store.left, cursor_b_step.result) ||
            cursor_a_step.op != icode_op::ADD ||
            !is_word_temp(cursor_a_step.result) ||
            !operands_equivalent(cursor_a_step.left, cursor_a_init.result) ||
            !is_exact_int_const(cursor_a_step.right, 1) ||
            cursor_a_store.op != icode_op::ASSIGN ||
            !operands_equivalent(cursor_a_store.result, cursor_a_init.result) ||
            !operands_equivalent(cursor_a_store.left, cursor_a_step.result) ||
            backedge.op != icode_op::GOTO ||
            backedge.label_name != loop.label_name ||
            done.op != icode_op::LABEL ||
            done.label_name != bound_branch.false_lbl ||
            return_true.op != icode_op::RETURN ||
            !is_exact_int_const(return_true.left, 1)) {
            return false;
        }

        const int count = static_cast<int>(bound_cmp.right.ival);
        emit_helper_header();
        emit_comment("frameless fixed-length byte equality loop");
        emit_line("ld\tb, h");
        emit_line("ld\tc, l");
        emit_line("push\tde");
        emit_line("pop\tiy");
        emit_line("ld\te, %s", asm_.imm(count).c_str());
        emit_label(loop.label_name, false);
        emit_line("ld\ta, (bc)");
        emit_line("cp\t0(iy)");
        emit_line("jr\tnz, %s", different_label.label_name.c_str());
        emit_line("inc\tbc");
        emit_line("inc\tiy");
        emit_line("dec\te");
        emit_line("jr\tnz, %s", loop.label_name.c_str());
        emit_line("ld\tde, %s", asm_.imm(1).c_str());
        emit_line("ret");
        emit_label(different_label.label_name, false);
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    auto match_frameless_fixed_shift_add_byte_fold = [&]() -> bool {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL1 ||
            fn.stack_param_bytes != 0 ||
            (body.size() != 19 && body.size() != 20) ||
            !fn.ret_type || fn.ret_type->size() != 2) {
            return false;
        }

        // The terminal widened-byte forwarding pass consumes the explicit
        // byte-to-word CAST, leaving the same operation with a direct byte
        // operand.  Accept both equivalent IR spellings.
        const bool has_widen = body.size() == 20;

        const auto &recv = *body[0];
        const auto &acc_init = *body[1];
        const auto &index_init = *body[2];
        const auto &cursor_init = *body[3];
        const auto &loop = *body[4];
        const auto &bound_cmp = *body[5];
        const auto &bound_branch = *body[6];
        const auto &body_label = *body[7];
        const auto &shift = *body[8];
        const auto &add_old = *body[9];
        const auto &load = *body[10];
        const icode *widen = has_widen ? body[11] : nullptr;
        const auto &add_byte = *body[has_widen ? 12 : 11];
        const auto &step_label = *body[has_widen ? 13 : 12];
        const auto &index_step = *body[has_widen ? 14 : 13];
        const auto &cursor_step = *body[has_widen ? 15 : 14];
        const auto &cursor_store = *body[has_widen ? 16 : 15];
        const auto &backedge = *body[has_widen ? 17 : 16];
        const auto &done = *body[has_widen ? 18 : 17];
        const auto &ret = *body[has_widen ? 19 : 18];
        const operand &byte_value = has_widen ? widen->result : load.result;

        if (recv.op != icode_op::RECEIVE ||
            recv.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv.result) ||
            acc_init.op != icode_op::ASSIGN ||
            !is_word_temp(acc_init.result) ||
            acc_init.left.kind != operand_kind::INT_CONST ||
            index_init.op != icode_op::ASSIGN ||
            !index_init.result.is_temp() ||
            !is_exact_int_const(index_init.left, 0) ||
            cursor_init.op != icode_op::ASSIGN ||
            !is_word_temp(cursor_init.result) ||
            !operands_equivalent(cursor_init.left, recv.result) ||
            loop.op != icode_op::LABEL ||
            bound_cmp.op != icode_op::LT ||
            !operands_equivalent(bound_cmp.left, index_init.result) ||
            bound_cmp.right.kind != operand_kind::INT_CONST ||
            bound_cmp.right.ival <= 0 || bound_cmp.right.ival > 255 ||
            bound_branch.op != icode_op::IFX ||
            !operands_equivalent(bound_branch.left, bound_cmp.result) ||
            body_label.op != icode_op::LABEL ||
            body_label.label_name != bound_branch.true_lbl ||
            shift.op != icode_op::SHL || !is_word_temp(shift.result) ||
            !operands_equivalent(shift.left, acc_init.result) ||
            shift.right.kind != operand_kind::INT_CONST ||
            shift.right.ival <= 0 || shift.right.ival > 7 ||
            add_old.op != icode_op::ADD || !is_word_temp(add_old.result) ||
            !((operands_equivalent(add_old.left, shift.result) &&
               operands_equivalent(add_old.right, acc_init.result)) ||
              (operands_equivalent(add_old.right, shift.result) &&
               operands_equivalent(add_old.left, acc_init.result))) ||
            load.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load.result) || !load.result.type ||
            !load.result.type->is_unsigned() ||
            !operands_equivalent(load.left, cursor_init.result) ||
            !load.right.is_none() ||
            (has_widen &&
             (widen->op != icode_op::CAST || !is_word_temp(widen->result) ||
              !widen->result.type || !widen->result.type->is_unsigned() ||
              !operands_equivalent(widen->left, load.result))) ||
            add_byte.op != icode_op::ADD ||
            !operands_equivalent(add_byte.result, acc_init.result) ||
            !((operands_equivalent(add_byte.left, add_old.result) &&
               operands_equivalent(add_byte.right, byte_value)) ||
              (operands_equivalent(add_byte.right, add_old.result) &&
               operands_equivalent(add_byte.left, byte_value))) ||
            step_label.op != icode_op::LABEL ||
            index_step.op != icode_op::ADD ||
            !operands_equivalent(index_step.result, index_init.result) ||
            !operands_equivalent(index_step.left, index_init.result) ||
            !is_exact_int_const(index_step.right, 1) ||
            cursor_step.op != icode_op::ADD ||
            !is_word_temp(cursor_step.result) ||
            !operands_equivalent(cursor_step.left, cursor_init.result) ||
            !is_exact_int_const(cursor_step.right, 1) ||
            cursor_store.op != icode_op::ASSIGN ||
            !operands_equivalent(cursor_store.result, cursor_init.result) ||
            !operands_equivalent(cursor_store.left, cursor_step.result) ||
            backedge.op != icode_op::GOTO ||
            backedge.label_name != loop.label_name ||
            done.op != icode_op::LABEL ||
            done.label_name != bound_branch.false_lbl ||
            ret.op != icode_op::RETURN ||
            !operands_equivalent(ret.left, acc_init.result)) {
            return false;
        }

        const int count = static_cast<int>(bound_cmp.right.ival);
        const int shift_count = static_cast<int>(shift.right.ival);
        const int initial = static_cast<int>(acc_init.left.ival & 0xffff);
        emit_helper_header();
        emit_comment("frameless fixed-count shift-add byte fold");
        emit_line("push\thl");
        emit_line("pop\tiy");
        emit_line("ld\tde, %s", asm_.imm(initial).c_str());
        emit_line("ld\tb, %s", asm_.imm(count).c_str());
        emit_label(loop.label_name, false);
        emit_line("ld\th, d");
        emit_line("ld\tl, e");
        for (int i = 0; i < shift_count; ++i)
            emit_line("add\thl, hl");
        emit_line("add\thl, de");
        emit_line("ld\te, 0(iy)");
        emit_line("ld\td, %s", asm_.imm(0).c_str());
        emit_line("add\thl, de");
        emit_line("ex\tde, hl");
        emit_line("inc\tiy");
        emit_line("djnz\t%s", loop.label_name.c_str());
        emit_line("ret");
        emit_helper_footer();
        return true;
    };

    // Size-oriented IR commonly advances one cursor explicitly while spelling
    // the second as base+index.  Prove the index and first cursor advance in
    // lockstep, then allocate the equivalent second cursor to IY for the
    // complete equality region.
    auto match_frameless_indexed_fixed_byte_equality = [&]() -> bool {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL1 ||
            fn.stack_param_bytes != 0 || body.size() != 23 ||
            !fn.ret_type || fn.ret_type->size() != 2) {
            return false;
        }

        const auto &recv_a = *body[0];
        const auto &recv_b = *body[1];
        const auto &index_init = *body[2];
        const auto &cursor_a_init = *body[3];
        const auto &loop = *body[4];
        const auto &bound_cmp = *body[5];
        const auto &bound_branch = *body[6];
        const auto &body_label = *body[7];
        const auto &wide_index = *body[8];
        const auto &load_a = *body[9];
        const auto &address_b = *body[10];
        const auto &load_b = *body[11];
        const auto &different = *body[12];
        const auto &different_branch = *body[13];
        const auto &different_label = *body[14];
        const auto &return_false = *body[15];
        const auto &step_label = *body[16];
        const auto &index_step = *body[17];
        const auto &cursor_a_step = *body[18];
        const auto &cursor_a_store = *body[19];
        const auto &backedge = *body[20];
        const auto &done = *body[21];
        const auto &return_true = *body[22];

        const bool b_address_matches =
            address_b.op == icode_op::ADD &&
            ((operands_equivalent(address_b.left, recv_b.result) &&
              operands_equivalent(address_b.right, wide_index.result)) ||
             (operands_equivalent(address_b.right, recv_b.result) &&
              operands_equivalent(address_b.left, wide_index.result)));
        if (recv_a.op != icode_op::RECEIVE ||
            recv_a.arg_loc != abi_arg_loc::REG_HL ||
            !is_word_temp(recv_a.result) ||
            recv_b.op != icode_op::RECEIVE ||
            recv_b.arg_loc != abi_arg_loc::REG_DE ||
            !is_word_temp(recv_b.result) ||
            index_init.op != icode_op::ASSIGN ||
            !index_init.result.is_temp() ||
            !is_exact_int_const(index_init.left, 0) ||
            cursor_a_init.op != icode_op::ASSIGN ||
            !is_word_temp(cursor_a_init.result) ||
            !operands_equivalent(cursor_a_init.left, recv_a.result) ||
            loop.op != icode_op::LABEL ||
            bound_cmp.op != icode_op::LT ||
            !operands_equivalent(bound_cmp.left, index_init.result) ||
            bound_cmp.right.kind != operand_kind::INT_CONST ||
            bound_cmp.right.ival <= 0 || bound_cmp.right.ival > 255 ||
            bound_branch.op != icode_op::IFX ||
            !operands_equivalent(bound_branch.left, bound_cmp.result) ||
            body_label.op != icode_op::LABEL ||
            body_label.label_name != bound_branch.true_lbl ||
            wide_index.op != icode_op::CAST ||
            !is_word_temp(wide_index.result) ||
            !operands_equivalent(wide_index.left, index_init.result) ||
            load_a.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_a.result) ||
            !operands_equivalent(load_a.left, cursor_a_init.result) ||
            !load_a.right.is_none() || !b_address_matches ||
            !is_word_temp(address_b.result) ||
            load_b.op != icode_op::GET_VALUE_AT ||
            !is_byte_temp(load_b.result) ||
            !operands_equivalent(load_b.left, address_b.result) ||
            !load_b.right.is_none() ||
            different.op != icode_op::NE ||
            !((operands_equivalent(different.left, load_a.result) &&
               operands_equivalent(different.right, load_b.result)) ||
              (operands_equivalent(different.right, load_a.result) &&
               operands_equivalent(different.left, load_b.result))) ||
            different_branch.op != icode_op::IFX ||
            !operands_equivalent(different_branch.left, different.result) ||
            different_label.op != icode_op::LABEL ||
            different_label.label_name != different_branch.true_lbl ||
            return_false.op != icode_op::RETURN ||
            !is_exact_int_const(return_false.left, 0) ||
            step_label.op != icode_op::LABEL ||
            step_label.label_name != different_branch.false_lbl ||
            index_step.op != icode_op::ADD ||
            !operands_equivalent(index_step.result, index_init.result) ||
            !operands_equivalent(index_step.left, index_init.result) ||
            !is_exact_int_const(index_step.right, 1) ||
            cursor_a_step.op != icode_op::ADD ||
            !is_word_temp(cursor_a_step.result) ||
            !operands_equivalent(cursor_a_step.left, cursor_a_init.result) ||
            !is_exact_int_const(cursor_a_step.right, 1) ||
            cursor_a_store.op != icode_op::ASSIGN ||
            !operands_equivalent(cursor_a_store.result, cursor_a_init.result) ||
            !operands_equivalent(cursor_a_store.left, cursor_a_step.result) ||
            backedge.op != icode_op::GOTO ||
            backedge.label_name != loop.label_name ||
            done.op != icode_op::LABEL ||
            done.label_name != bound_branch.false_lbl ||
            return_true.op != icode_op::RETURN ||
            !is_exact_int_const(return_true.left, 1)) {
            return false;
        }

        const int count = static_cast<int>(bound_cmp.right.ival);
        emit_helper_header();
        emit_comment("frameless indexed fixed-length byte equality loop");
        emit_line("ld\tb, h");
        emit_line("ld\tc, l");
        emit_line("push\tde");
        emit_line("pop\tiy");
        emit_line("ld\te, %s", asm_.imm(count).c_str());
        emit_label(loop.label_name, false);
        emit_line("ld\ta, (bc)");
        emit_line("cp\t0(iy)");
        emit_line("jr\tnz, %s", different_label.label_name.c_str());
        emit_line("inc\tbc");
        emit_line("inc\tiy");
        emit_line("dec\te");
        emit_line("jr\tnz, %s", loop.label_name.c_str());
        emit_line("ld\tde, %s", asm_.imm(1).c_str());
        emit_line("ret");
        emit_label(different_label.label_name, false);
        emit_line("ld\tde, %s", asm_.imm(0).c_str());
        emit_line("ret");
        emit_helper_footer();
        return true;
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

    if (match_frameless_u16_mul_shr8() ||
        match_frameless_byte_distance() ||
        match_frameless_byte_compare() ||
        match_frameless_byte_copy_until_zero() ||
        match_frameless_fixed_byte_equality() ||
        match_frameless_fixed_shift_add_byte_fold() ||
        match_frameless_indexed_fixed_byte_equality()) {
        return true;
    }
    if (!sdcc_style_specialization_enabled())
        return false;

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
        emit_line("jp\tnz, %s", mangle(ifx_ic.true_lbl).c_str());
        emit_line("jp\t%s", mangle(ifx_ic.false_lbl).c_str());
    } else if (!ifx_ic.true_lbl.empty()) {
        emit_line("jp\tnz, %s", mangle(ifx_ic.true_lbl).c_str());
    } else if (!ifx_ic.false_lbl.empty()) {
        emit_line("jp\tz, %s", mangle(ifx_ic.false_lbl).c_str());
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
        opt_settings_.level != opt_level::Os)
        return false;
    if (idx + 4 >= fn.icodes.size())
        return false;

    const icode &shift = fn.icodes[idx];
    const icode &sum = fn.icodes[idx + 1];
    const icode &load = fn.icodes[idx + 2];
    const icode &widen = fn.icodes[idx + 3];
    const icode &accumulate = fn.icodes[idx + 4];
    if (shift.op != icode_op::SHL || !shift.result.is_temp() ||
        shift.right.kind != operand_kind::INT_CONST ||
        op_size(shift.left) != 2 || op_size(shift.result) != 2 ||
        sum.op != icode_op::ADD || !sum.result.is_temp() ||
        load.op != icode_op::GET_VALUE_AT || !load.result.is_temp() ||
        op_size(load.result) != 1 || !load.right.is_none() ||
        !load.left.type || !load.left.type->is_ptr() ||
        load.left.type->is_far_ptr() ||
        (load.result.type && load.result.type->is_volatile) ||
        (load.left.type->base && load.left.type->base->is_volatile) ||
        widen.op != icode_op::CAST || !widen.result.is_temp() ||
        !temp_eq(widen.left, load.result.temp_id) ||
        op_size(widen.result) != 2 ||
        !widen.left.type || widen.left.type->size() != 1 ||
        !widen.left.type->is_unsigned() ||
        accumulate.op != icode_op::ADD ||
        op_size(accumulate.result) != 2) {
        return false;
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
    const bool final_sum_left =
        temp_eq(accumulate.left, sum.result.temp_id) &&
        temp_eq(accumulate.right, widen.result.temp_id);
    const bool final_sum_right =
        temp_eq(accumulate.right, sum.result.temp_id) &&
        temp_eq(accumulate.left, widen.result.temp_id);
    if ((!sum_shift_left && !sum_shift_right) ||
        (!final_sum_left && !final_sum_right) ||
        temp_value_used_after(fn, idx + 2, shift.result.temp_id) ||
        temp_value_used_after(fn, idx + 5, sum.result.temp_id) ||
        temp_value_used_after(fn, idx + 4, load.result.temp_id) ||
        temp_value_used_after(fn, idx + 5, widen.result.temp_id)) {
        return false;
    }

    if (debug_)
        debug_->emit_location(accumulate.line ? accumulate.line : shift.line);
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
    store_hl(accumulate.result);
    invalidate_pair_cache();
    invalidate_a_cache();
    idx += 4;
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

    emit_comment("O3 jump-table switch (%zu cases, span=%zu)",
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
