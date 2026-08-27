//
// z80gen.h — Z80 assembly code generator for the xcc compiler.
//
// z80_gen translates an ir_module into Z80 assembly text (sdasz80
// syntax).  One ir_function produces one assembly function; each icode
// instruction dispatches to a dedicated emit helper.
//
// Register usage convention:
//   HL  — primary working register (most loads and arithmetic results)
//   DE  — secondary register (second operand for 16-bit operations)
//   A   — byte operations and comparisons
//   IX  — frame pointer (fixed throughout each function)
//   BC  — scratch for shift loops and mul/div helper calls
//
// Temporaries: all anonymous temporaries (TEMP operands) spill to the
// IX stack frame.  temp_slots_ maps each temp_id to its IX-relative
// byte offset.  alloc_temp() assigns new slots growing downward from
// IX-2 (after the last local variable).
//
// Output uses sdasz80 assembly syntax:
//   immediates  : #value         (e.g., ld hl,#42)
//   IX-relative : N (ix)         (space before the paren is required)
//   globals     : .globl name    (not .global)
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "z80.h"
#include "backend/asm_emitter.h"
#include "backend/z80/convention.h"
#include "backend/z80/debug_info.h"
#include "ir/icode.h"
#include "opt/opt_settings.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iosfwd>

namespace xcc {

// Where a register-allocated temp lives.
// EXX-bank slots (alt_bc/alt_de/alt_hl) are reserved for a future
// basic-block-local EXX region pass; the current allocator only uses
// stack, main_bc, and alt_a.
enum class temp_home {
    stack,    // spilled to IX frame (default)
    main_bc,  // 16-bit temp in main BC register pair
    main_hl,  // next-use-only 16-bit temp still live in HL
    main_de,  // loaded 16-bit temp retained through a comparison chain
    main_iy,  // loop-carried byte pointer in caller-saved IY
    remat_hl, // 16-bit temp rematerialized cheaply into HL on demand
    main_a,   // next-use-only 8-bit temp still live in A
    main_b,   // 8-bit temp kept in B across a wider loop/control window
    main_c,   // 8-bit temp kept in C across a wider straight-line / branchy window
    main_d,   // branch-local loaded byte kept in D across a proven safe window
    main_e,   // loop-carried byte accumulator kept in E across a proven safe window
    alt_a,    // 8-bit temp in A' via ex af,af'
    arg_a,    // incoming 8-bit argument still live in A
    arg_l,    // incoming 8-bit argument still live in L
    arg_hl,   // incoming 16-bit argument still live in HL
    arg_de,   // incoming 16-bit argument still live in DE
    alt_bc,   // (reserved) 16-bit temp in BC' within an EXX region
    alt_de,   // (reserved) 16-bit temp in DE' within an EXX region
    alt_hl,   // (reserved) 16-bit temp in HL' within an EXX region
};

// Forward-declare concrete convention classes so z80_gen can grant them friendship.
struct stack_linkage_convention;
struct cc_sdcccall0;
struct cc_sdcccall1;
struct cc_z88dk_smallc;
struct cc_z88dk_fastcall;
struct cc_z88dk_callee;
struct cc_naked;
struct cc_interrupt;
struct cc_critical;

class z80_gen {
    friend struct abi_convention;
    friend struct stack_linkage_convention;
    friend struct cc_sdcccall0;
    friend struct cc_sdcccall1;
    friend struct cc_z88dk_smallc;
    friend struct cc_z88dk_fastcall;
    friend struct cc_z88dk_callee;
    friend struct cc_naked;
    friend struct cc_interrupt;
    friend struct cc_critical;
public:
    //
    // Construct a code generator that emits assembly via the given emitter.
    //
    explicit z80_gen(asm_emitter &asm_out);

    //
    // Set normalized optimization settings. Must be called before
    // emit_module().
    void set_opt_settings(const optimization_settings &settings) {
        opt_settings_ = settings;
    }

    //
    // Tell the backend whether the driver is stopping after assembly (-S).
    // The value is still useful for diagnostics/formatting choices, but
    // size-optimized SDCC-compatible output may reference public runtime
    // helpers just like SDCC does.
    //
    void set_standalone_assembly_output(bool enabled) {
        standalone_asm_output_ = enabled;
    }

    // Emit z88dk classic's link-time printf/scanf capability definitions.
    // These are assembler metadata and contribute no target bytes.
    void set_z88dk_classic_runtime(bool enabled) {
        z88dk_classic_runtime_ = enabled;
    }

    //
    // Attach a debug info emitter (DWARF or SDCC-style).
    // Must be called before emit_module().  When not called, no debug
    // output is emitted.
    //
    void set_debug(std::unique_ptr<debug_info_emitter> dbg);

    //
    // Generate assembly for the entire module: global variable
    // declarations, string literals, and all functions.
    //
    void emit_module(const ir_module &mod);

private:
    struct pair_cache_state {
        bool valid = false;
        std::string key;
    };
    struct byte_cache_state {
        bool valid = false;
        std::string key;
    };

    asm_emitter      &asm_;
    const ir_function *cur_fn_       = nullptr;
    int               local_bytes_   = 0;
    std::string       fn_end_lbl_;

    // Active calling-convention for the function being compiled.
    abi_convention *cur_convention_ = nullptr;

    std::unique_ptr<debug_info_emitter> debug_; // null unless -g was passed

    optimization_settings opt_settings_ =
        optimization_settings::for_level(opt_level::O0);
    bool standalone_asm_output_ = false;
    bool z88dk_classic_runtime_ = false;
    bool size_shared_ix_helpers_ = false;
    bool compact_codegen_ = false;

    std::unordered_map<int, int>       temp_slots_; // temp_id -> IX offset
    std::unordered_map<int, temp_home> temp_regs_;  // temp_id -> register home (if not stack)
    std::unordered_map<int, temp_home> incoming_symbol_homes_; // local symbol stack_offset -> incoming arg home
    std::unordered_map<int, temp_home> symbol_regs_; // local symbol key -> register home (if not stack)
    // Direct, register-argument calls at which a live IY allocation is
    // caller-saved.  Keeping this per-call (rather than treating IY as
    // globally callee-saved) avoids changing the public ABI.
    std::unordered_set<size_t> iy_preserved_call_indices_;
    // Direct local helpers whose selected machine-code form is known not to
    // touch IY.  This is populated as those helpers are emitted and lets a
    // later caller avoid a redundant caller-save pair.
    std::unordered_set<std::string> iy_preserving_local_callees_;
    // File-scope static functions defined in the current translation unit.
    // Direct references to these symbols must not emit .globl/.global.
    std::unordered_set<std::string> internal_function_names_;
    // Any definition in this translation unit suppresses standard-library
    // builtin substitution, including a public definition.
    std::unordered_set<std::string> defined_function_names_;
    // Likewise, a profitable loop counter may occupy BC across a direct
    // register-only call.  These are the exact sites where it is saved.
    std::unordered_set<size_t> bc_preserved_call_indices_;
    // Canonical zero-based +1 induction values whose controlling upper bound
    // proves that only one byte is needed for their loop comparison.
    std::unordered_map<int, int> bounded_induction_limits_;
    // Local C variables are memory symbols rather than SSA temporaries.  Keep
    // the exact proven compare/update sites for their bounded loops so reuse
    // of the same source variable elsewhere cannot inherit the range fact.
    std::unordered_map<size_t, int> bounded_symbol_induction_comparisons_;
    std::unordered_set<size_t> bounded_symbol_induction_increments_;
    // Byte loads immediately consumed by a selected jump-table dispatch stay
    // in A; materializing their otherwise dead allocator home is unnecessary.
    std::unordered_set<size_t> jump_table_selector_loads_;
    // Single-use 32-bit loads through a proven IY base can be rematerialized
    // word-by-word by their immediately following arithmetic consumer.  This
    // avoids a four-byte spill/reload while preserving the source load's
    // exact address and ordering.
    std::unordered_map<int, int64_t> iy_u32_remat_offsets_;
    // Word loads selected as addends for a DE-resident loop reduction are
    // emitted directly into HL.  A general main_hl home is not sufficient:
    // ordinary one-step forwarding may deliberately keep the value in DE.
    std::unordered_set<int> iy_de_reduction_hl_loads_;
    int next_temp_slot_ = 0;
    int temp_stack_bytes_ = 0;
    int temp_frame_bytes_ = 0;
    bool reserving_prologue_spills_ = false;
    size_t cur_ic_index_ = 0;
    std::unordered_set<size_t> skipped_icodes_;
    size_t local_label_counter_ = 0;
    bool direct_call_return_pending_ = false;
    operand direct_call_return_value_;
    bool sibling_tail_call_pending_ = false;
    operand sibling_tail_call_value_;
    bool automatic_address_materialized_ = false;
    bool last_frameless_return_terminated_ = false;
    bool direct_compare_return_pending_ = false;
    operand direct_compare_return_value_;
    bool direct_call_ifx_pending_ = false;
    operand direct_call_ifx_value_;
    call_abi direct_call_ifx_abi_ = call_abi::DEFAULT;
    int direct_call_ifx_reg_size_ = 0;
    bool direct_call_ifx_keep_word_pending_ = false;
    bool direct_widen_send_pending_ = false;
    operand direct_widen_send_value_;
    operand direct_widen_send_source_;
    bool direct_byte_load_ifx_pending_ = false;
    operand direct_byte_load_ifx_value_;
    bool direct_word_load_ifx_pending_ = false;
    operand direct_word_load_ifx_value_;
    bool direct_word_value_pending_ = false;
    operand direct_word_value_;
    bool direct_mem_copy_pending_ = false;
    operand direct_mem_copy_value_;
    operand direct_mem_copy_src_ptr_;
    operand direct_mem_copy_src_index_;
    bool direct_postinc_load_pending_ = false;
    operand direct_postinc_load_cursor_;
    operand direct_postinc_load_old_ptr_;
    int direct_postinc_load_step_ = 0;
    size_t direct_postinc_load_get_index_ = 0;
    pair_cache_state hl_cache_;
    pair_cache_state de_cache_;
    byte_cache_state a_cache_;
    bool sp_ix_delta_known_ = false;
    int sp_ix_delta_ = 0;

    // TLS: maps mangled global name → byte offset within the TLS block.
    // Built during emit_module() before globals are emitted.
    std::unordered_map<std::string, int> tls_offsets_;
    int tls_size_ = 0;

    // Return true if ic clobbers main BC (CALL, MUL, DIV, MOD, shifts, float).
    static bool clobbers_bc(const icode &ic);

    // Pre-pass: compute temp live intervals and assign register targets.
    //   16-bit temps → "bc"    (main BC, 2 instructions to save/load)
    //    8-bit temps → "a_alt" (A' via ex af,af', 1 instruction to save/load)
    // Populates temp_regs_.  Called at the start of emit_function() for -O2.
    void regalloc_prepass(const ir_function &fn);

    // Compute the total bytes needed for stack-resident TEMP slots and
    // pre-plan reusable IX spill slots for overlapping temp live ranges.
    int compute_temp_frame_bytes(const ir_function &fn);

    // ----- assembly emission -----------------------------------------

    //
    // Emit a formatted assembly line to out_.
    //
    void emit_line(const char *fmt, ...);

    // Declare a direct callee only when it has external linkage.
    void emit_direct_callee_decl(const std::string &name);
    bool is_inline_ctype_call(const icode &ic) const;
    void emit_inline_ctype_call(const icode &ic);

    //
    // Emit a label definition.  If global is true, precede it with
    // .globl to export the symbol to the linker.
    //
    void emit_label(const std::string &name, bool global = false);
    std::string fresh_local_label(const char *prefix);

    void emit_comment(const char *fmt, ...);
    void invalidate_pair_cache();
    void invalidate_hl_cache();
    void invalidate_de_cache();
    void invalidate_a_cache();
    std::string pair_load_cache_key(const operand &op) const;
    std::string pair_word_cache_key(const operand &op, int word_index) const;
    std::string pair_ix_addr_cache_key(int off) const;
    std::string a_load_cache_key(const operand &op) const;
    void track_emitted_instruction(const std::string &line);
    void set_known_sp_ix_delta(int delta) {
        sp_ix_delta_known_ = true;
        sp_ix_delta_ = delta;
    }
    void clear_known_sp_ix_delta() {
        sp_ix_delta_known_ = false;
        sp_ix_delta_ = 0;
    }
    bool has_known_sp_ix_delta() const { return sp_ix_delta_known_; }
    int current_sp_ix_delta() const { return sp_ix_delta_; }

    bool regalloc_enabled() const { return opt_settings_.regalloc; }
    bool compare_ifx_fusion_enabled() const { return opt_settings_.compare_ifx_fusion; }
    bool frame_omit_enabled() const { return opt_settings_.frame_omit; }
    bool temp_frame_prealloc_enabled() const { return opt_settings_.prealloc_temp_frame; }
    bool switch_jump_tables_enabled() const { return opt_settings_.switch_jump_tables; }
    bool size_opt_enabled() const {
        return opt_settings_.level == opt_level::Os || compact_codegen_;
    }
    bool tuned_profile_enabled() const {
        return opt_settings_.level == opt_level::Os ||
               opt_settings_.level == opt_level::Of ||
               opt_settings_.level == opt_level::O3;
    }
    bool shared_ix_helpers_enabled() const {
        return !debug_ &&
               (opt_settings_.level == opt_level::O2 ||
                (opt_settings_.level == opt_level::Os &&
                 size_shared_ix_helpers_));
    }
    bool pair_cache_enabled() const {
        return opt_settings_.level == opt_level::O2 || tuned_profile_enabled();
    }
    bool a_cache_enabled() const { return pair_cache_enabled(); }
    int required_frame_bytes() const { return local_bytes_ + temp_stack_bytes_; }
    int total_frame_bytes() const { return local_bytes_ + temp_frame_bytes_; }
    static bool temp_home_uses_spill_slot(temp_home home);
    static int symbol_reg_key(const operand &op);
    bool symbol_home_in_bc(const operand &op) const;
    bool symbol_home_in_iy(const operand &op) const;
    bool operand_home_in_bc(const operand &op) const;
    bool needs_frame_without_temps(const ir_function &fn) const;
    bool can_omit_frame_pointer(const ir_function &fn) const;
    bool try_finish_direct_hl_return(const operand &result);
    bool structured_loop_fastpaths_enabled() const {
        return opt_settings_.level == opt_level::O2 ||
               opt_settings_.level == opt_level::Of ||
               opt_settings_.level == opt_level::Os ||
               opt_settings_.level == opt_level::O3;
    }
    bool temp_value_used_after(const ir_function &fn, size_t start_idx, int temp_id) const;
    bool symbol_value_used_after(const ir_function &fn, size_t start_idx,
                                 const operand &sym) const;
    const icode *find_temp_def_before(int temp_id, size_t before_idx) const;
    bool get_zero_extended_u8_source(const operand &op, operand &src) const;
    bool emit_rematerialize_hl(const operand &op);
    bool emit_byte_alu_direct_rhs(const char *mnemonic,
                                  const operand &rhs,
                                  bool allow_bc_rhs);
    bool try_emit_u32_frame_add_chain(const icode &ic);
    bool try_emit_u32_frame_add(const icode &ic);
    bool try_emit_u32_frame_alu(const icode &ic, const char *mnemonic);
    bool try_emit_u32_bitwise_add_chain(const icode &ic);
    void maybe_materialize_incoming_arg_temp(
        const operand &op, bool scan_across_branches = false);
    void maybe_materialize_incoming_arg_symbol(const operand &op);
    bool get_sign_extended_i8_source(const operand &op, operand &src) const;
    bool try_emit_byte_mask_walk_loop(const ir_function &fn, size_t &idx);
    bool try_emit_byte_copy_walk_loop(const ir_function &fn, size_t &idx);
    bool try_emit_zero_byte_walk_loop(const ir_function &fn, size_t &idx);
    bool try_emit_inplace_byte_step_ifx(const ir_function &fn, size_t &idx);
    bool try_emit_inplace_pointer_update(const ir_function &fn, size_t &idx);
    bool try_emit_iy_indexed_load(const ir_function &fn, size_t &idx);
    bool try_emit_iy_indexed_store(const ir_function &fn, size_t &idx);
    bool try_emit_scaled_frame_load(const ir_function &fn, size_t &idx);
    bool try_emit_scaled_global_load(const ir_function &fn, size_t &idx);
    bool try_emit_postinc_indexed_load(const ir_function &fn, size_t &idx);
    bool try_emit_postinc_indexed_store(const ir_function &fn, size_t &idx);
    bool try_emit_postdec_truth(const ir_function &fn, size_t &idx);
    bool try_emit_shift_xor_self(const ir_function &fn, size_t &idx);
    bool try_emit_msb_word_shift_xor_diamonds(const ir_function &fn,
                                               size_t &idx);
    bool try_emit_msb_byte_shift_xor_diamonds(const ir_function &fn,
                                               size_t &idx);
    bool try_emit_shift_add_byte_accumulate(const ir_function &fn, size_t &idx);
    bool try_emit_switch_jump_table(const ir_function &fn, size_t &idx);
    bool try_emit_lsb32_shift_xor_diamond(const ir_function &fn, size_t &idx);
    bool try_emit_band_ifx(const ir_function &fn, size_t &idx);
    bool try_emit_byte_load_compare_ifx(const ir_function &fn, size_t &idx);
    bool try_emit_guarded_zero_arg_indirect_call(const ir_function &fn,
                                                 size_t &idx);
    bool try_emit_word_select_send(const ir_function &fn, size_t &idx);
    bool try_emit_compare_ifx(const ir_function &fn, size_t &idx);
    bool find_direct_byte_truth_ifx(const operand &value,
                                    size_t start_idx,
                                    operand &ifx_value) const;
    bool find_direct_word_truth_ifx(const operand &value,
                                    size_t start_idx,
                                    operand &ifx_value) const;
    bool is_flag_preserving_byte_truth_bridge(const icode &ic) const;

    // ----- module-level emission -------------------------------------

    void plan_size_shared_ix_helpers(const ir_module &mod);

    //
    // Emit .area _DATA declarations for all global variables.
    //
    void emit_globals(const ir_module &mod);
    void emit_external_data_refs(const ir_module &mod);

    //
    // Emit string literals into .area _DATA.
    //
    void emit_strings(const ir_module &mod);

    //
    // Emit the data body for one global variable (initializers or .ds reserve).
    // zero_fill: if true, emit zeros regardless of the declared initializer.
    //
    void emit_global_body(const ir_module::global_var &g, bool zero_fill);

    //
    // Emit the complete assembly for one function.
    //
    void emit_function(const ir_function &fn);

    // ----- function prologue/epilogue --------------------------------

    //
    // Emit the standard IX-frame prologue:
    //   push ix / ld ix,#0 / add ix,sp / dec sp / dec sp ...
    //
    void emit_prologue(const ir_function &fn);

    //
    // Emit the standard epilogue: restore IX and return.
    //
    void emit_epilogue(const ir_function &fn);

    // ----- icode dispatch --------------------------------------------

    //
    // Dispatch ic to the appropriate gen_* handler below.
    //
    void gen_icode(const icode &ic);

    void gen_label       (const icode &ic);
    void gen_goto        (const icode &ic);
    void gen_ifx         (const icode &ic);
    void gen_function    (const icode &ic);
    void gen_endfunction (const icode &ic);
    void gen_return      (const icode &ic);
    void gen_send        (const icode &ic);
    void gen_receive     (const icode &ic);

    //
    // Emit a function call.  Direct calls use ic.func_name; indirect
    // calls delegate to the callee ABI's trampoline rules.
    //
    void gen_call        (const icode &ic);

    void gen_assign      (const icode &ic);
    void gen_address_of  (const icode &ic);
    void gen_get_value_at(const icode &ic);
    void gen_set_value_at(const icode &ic);
    void gen_block_fill  (const icode &ic);
    void gen_add         (const icode &ic);
    void gen_sub         (const icode &ic);
    void gen_mul         (const icode &ic);
    void gen_div_mod     (const icode &ic, bool want_mod);
    void gen_neg         (const icode &ic);
    void gen_band        (const icode &ic);
    void gen_bor         (const icode &ic);
    void gen_bxor        (const icode &ic);
    void gen_bnot        (const icode &ic);
    void gen_shift       (const icode &ic, bool right, bool arithmetic);
    void gen_rotate      (const icode &ic, bool right);
    void gen_pack_bytes  (const icode &ic);
    void emit_compare_branch(const icode &ic, icode_op cmp,
                             const std::string &true_lbl,
                             const std::string &false_lbl);
    void gen_compare     (const icode &ic, icode_op cmp);
    void gen_cast        (const icode &ic);
    void gen_float_arith (const icode &ic);
    void gen_alloca      (const icode &ic);
    void gen_inline_asm  (const icode &ic);
    void gen_make_complex(const icode &ic);

    // Consume an immediately following `*iy_derived = result` while the
    // 16-bit result is still in HL.  Returns true when the store was emitted
    // and its IR instruction marked as consumed.
    bool try_finish_direct_hl_iy_store(const operand &result);
    bool try_emit_word_load_add_chain();

    // ----- operand loading -------------------------------------------

    //
    // Descriptor for a Z80 16-bit register pair used by emit_load_rr
    // and emit_store_rr.  via_hl is true for register pairs (e.g. DE)
    // that cannot be loaded directly from a global memory address and
    // must route through HL with an ex instruction.
    //
    struct reg_pair {
        const char *name; // "hl", "de" — used for immediate loads
        char        lo;   // low-byte register: 'l', 'e'
        char        hi;   // high-byte register: 'h', 'd'
        bool        via_hl; // must use HL+ex for global indirect loads
    };

    void set_pair_cache(const reg_pair &r, const std::string &key);
    bool pair_cache_matches(const reg_pair &r, const std::string &key) const;
    void set_a_cache(const std::string &key);
    bool a_cache_matches(const std::string &key) const;

    //
    // Unified 16-bit register-pair load: emit the correct instruction
    // sequence to load op into the register pair described by r.
    // Handles all operand_kind cases in one switch.
    //
    void emit_load_rr (const reg_pair &r, const operand &op);

    //
    // Unified 16-bit register-pair store: emit the correct instruction
    // sequence to store the register pair described by r into op.
    //
    void emit_store_rr(const reg_pair &r, const operand &op);

    static bool fits_ix_disp(int off);
    void load_ix_addr_hl(int off);
    void load_frame_byte(char dst, int off);
    void store_frame_byte(int off, char src);
    void load_frame_word(const reg_pair &r, int off);
    void store_frame_word(const reg_pair &r, int off);

    // ----- far (24-bit banked) pointer support -----------------------
    //
    // Load / store the bank byte (byte 2) of a far pointer operand into A.
    // Unlike load_a/store_a these honour the byte offset for global
    // symbols, where the bank lives at (sym+2).
    //
    void load_far_bank (const operand &ptr);
    void store_far_bank(const operand &dst);

    //
    // Load a far pointer operand into HL (16-bit address) and C (bank).
    // Robust against helper clobbers via push/pop sequencing.
    //
    void emit_load_far_ptr(const operand &ptr);

    // Lower *farptr (load) and *farptr = v (store) via the far-access
    // runtime trampoline (__far_getb / __far_putb).  Return true when the
    // operand is a far pointer and the far path was emitted.
    //
    bool gen_far_get_value_at(const icode &ic);
    bool gen_far_set_value_at(const icode &ic);

    // 24-bit far pointer ± integer arithmetic.  Returns true when the
    // result is a far pointer and the far path was emitted.  is_add
    // selects addition (carry into bank) vs subtraction (borrow).
    bool gen_far_ptr_arith(const icode &ic, bool is_add);

    //
    // Load op (16-bit) into HL.
    //
    void load_hl(const operand &op);

    //
    // Load op (16-bit) into DE.
    //
    void load_de(const operand &op);

    //
    // Load a zero-extended 8-bit source into DE.
    //
    void load_de_zero_extended_u8(const operand &op);

    //
    // Load op (16-bit) into BC (for sdccall(1) 3rd argument).
    //
    void load_bc(const operand &op);

    //
    // Load op (8-bit) into A.
    //
    void load_a(const operand &op);

    //
    // Store HL into the location identified by op.
    //
    void store_hl(const operand &op);

    //
    // Store DE into the location identified by op.
    //
    void store_de(const operand &op);

    //
    // Store A into the location identified by op.
    //
    void store_a(const operand &op);

    // ----- 32-bit and 64-bit operand loading (long arithmetic) -------

    //
    // Load 16-bit word at byte-offset word_index*2 from op into HL.
    // word_index 0 = lowest 16 bits, 1 = next 16, 2 = bits 32-47, 3 = bits 48-63.
    //
    void load_hl_word(const operand &op, int word_index);

    //
    // Load 16-bit word at byte-offset word_index*2 from op into DE.
    //
    void load_de_word(const operand &op, int word_index);

    //
    // Store HL into the 16-bit word at byte-offset word_index*2 in op.
    //
    void store_hl_word(const operand &op, int word_index);

    //
    // Store DE into the 16-bit word at byte-offset word_index*2 in op.
    //
    void store_de_word(const operand &op, int word_index);

    //
    // Load the low 16 bits of a 32-bit operand into HL.
    //
    void load_hl_lo32(const operand &op);

    //
    // Load the high 16 bits of a 32-bit operand into HL.
    //
    void load_hl_hi32(const operand &op);

    //
    // Store HL into the low 16 bits of a 32-bit destination.
    //
    void store_hl_lo32(const operand &op);

    //
    // Store HL into the high 16 bits of a 32-bit destination.
    //
    void store_hl_hi32(const operand &op);

    //
    // Load/store a full 64-bit operand using the runtime ABI register shape:
    //   DE:HL:DE':HL'  (word0..word3, low to high)
    //
    void load_reg64(const operand &op);
    void store_reg64(const operand &op);

    //
    // Return the SDAS addressing string for op's memory location
    // (e.g., "-2 (ix)" for a local, or a global label name).
    //
    std::string addr_of(const operand &op);

    //
    // Allocate a stack slot for a temp of sz bytes.
    // Returns the IX-relative offset of the new slot.
    //
    int alloc_temp(int temp_id, int sz = 2);

    //
    // Return the IX-relative offset of a previously allocated temp.
    //
    int temp_ix_offset(int temp_id) const;

    //
    // Return the byte size of op based on its type.
    //
    int  op_size(const operand &op) const;

    //
    // Return true if op is a 2-byte (16-bit) operand.
    //
    bool op_is_16bit(const operand &op) const;

    // ----- helpers ---------------------------------------------------

    //
    // Return true when the byte value currently in A is being stored to a
    // non-volatile stack slot whose current contents are provably overwritten
    // on every forward path before any read or aliasing escape.
    //
    bool can_elide_current_byte_store(const operand &op) const;
    bool byte_slot_overwritten_before_read(size_t start,
                                           const operand &slot,
                                           size_t budget,
                                           std::unordered_set<size_t> &active) const;

    //
    // Apply platform name-mangling to a C identifier.
    // User-visible C symbols always gain one assembler leading underscore,
    // so "foo" -> "_foo" and "_foo" -> "__foo". Only the compiler-reserved
    // __xcc_/__xopt_ label namespaces already live in assembler space.
    //
    std::string mangle(const std::string &name) const;

    //
    // Resolve a label-reference operand name for assembly emission.
    // Real code labels already arrive mangled (e.g. "__xcc_L1"), but some
    // IR transforms also use LABEL_REF for static data symbols by their C
    // source names. Those need the normal platform mangling before emission.
    //
    std::string asm_label_ref_name(const std::string &name) const;

    //
    // Resolve an operand that names a code/data symbol for assembly emission.
    // SYMBOL operands use the normal C name mangling; LABEL_REF operands use
    // the label-aware helper so compiler-generated data refs stay correct.
    //
    std::string asm_symbol_ref_name(const operand &op) const;


    //
    // Return the IX-relative byte offset for a parameter operand.
    // Parameters live at IX + 4 + stack_offset.
    //
    int param_ix_offset(const operand &op) const;

    //
    // Return the IX-relative byte offset for any stack-allocated operand
    // (TEMP, local SYMBOL, or param SYMBOL).  Centralises the repeated
    // three-way branch that appeared in every load/store helper.
    //
    int ix_offset_of(const operand &op) const;
};

} // namespace xcc
