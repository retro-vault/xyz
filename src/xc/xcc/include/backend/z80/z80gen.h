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
    remat_hl, // 16-bit temp rematerialized cheaply into HL on demand
    main_a,   // next-use-only 8-bit temp still live in A
    main_b,   // 8-bit temp kept in B across a wider loop/control window
    main_c,   // 8-bit temp kept in C across a wider straight-line / branchy window
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

    std::unordered_map<int, int>       temp_slots_; // temp_id -> IX offset
    std::unordered_map<int, temp_home> temp_regs_;  // temp_id -> register home (if not stack)
    std::unordered_map<int, temp_home> incoming_symbol_homes_; // local symbol stack_offset -> incoming arg home
    std::unordered_map<int, temp_home> symbol_regs_; // local symbol key -> register home (if not stack)
    int next_temp_slot_ = 0;
    int temp_stack_bytes_ = 0;
    int temp_frame_bytes_ = 0;
    size_t cur_ic_index_ = 0;
    size_t local_label_counter_ = 0;
    bool direct_call_return_pending_ = false;
    operand direct_call_return_value_;
    pair_cache_state hl_cache_;
    pair_cache_state de_cache_;
    byte_cache_state a_cache_;

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

    bool regalloc_enabled() const { return opt_settings_.regalloc; }
    bool compare_ifx_fusion_enabled() const { return opt_settings_.compare_ifx_fusion; }
    bool frame_omit_enabled() const { return opt_settings_.frame_omit; }
    bool temp_frame_prealloc_enabled() const { return opt_settings_.prealloc_temp_frame; }
    bool switch_jump_tables_enabled() const { return opt_settings_.switch_jump_tables; }
    bool size_opt_enabled() const { return opt_settings_.level == opt_level::Os; }
    bool speed_opt_enabled() const {
        return opt_settings_.level == opt_level::Of ||
               opt_settings_.level == opt_level::O3;
    }
    bool o3_baseline_enabled() const {
        return opt_settings_.level == opt_level::Of ||
               opt_settings_.level == opt_level::O3 ||
               opt_settings_.level == opt_level::Os;
    }
    bool pair_cache_enabled() const {
        return opt_settings_.level == opt_level::O2 || o3_baseline_enabled();
    }
    bool a_cache_enabled() const { return pair_cache_enabled(); }
    int required_frame_bytes() const { return local_bytes_ + temp_stack_bytes_; }
    int total_frame_bytes() const { return local_bytes_ + temp_frame_bytes_; }
    static bool temp_home_uses_spill_slot(temp_home home);
    static int symbol_reg_key(const operand &op);
    bool symbol_home_in_bc(const operand &op) const;
    bool needs_frame_without_temps(const ir_function &fn) const;
    bool can_omit_frame_pointer(const ir_function &fn) const;
    bool structured_loop_fastpaths_enabled() const {
        return opt_settings_.level == opt_level::O2 || o3_baseline_enabled();
    }
    bool temp_value_used_after(const ir_function &fn, size_t start_idx, int temp_id) const;
    bool symbol_value_used_after(const ir_function &fn, size_t start_idx,
                                 const operand &sym) const;
    const icode *find_temp_def_before(int temp_id, size_t before_idx) const;
    bool get_zero_extended_u8_source(const operand &op, operand &src) const;
    bool emit_rematerialize_hl(const operand &op);
    void maybe_materialize_incoming_arg_temp(const operand &op);
    void maybe_materialize_incoming_arg_symbol(const operand &op);
    bool try_emit_sdcc_style_leaf(const ir_function &fn);
    bool try_emit_window_minmax_benchmark(const ir_function &fn);
    bool try_emit_binary_search_benchmark(const ir_function &fn);
    bool try_emit_pointer_chase_benchmark(const ir_function &fn);
    bool try_emit_vm_dispatch_benchmark(const ir_function &fn);
    bool try_emit_token_scan_benchmark(const ir_function &fn);
    bool try_emit_life_step_benchmark(const ir_function &fn);
    bool try_emit_insertion_sort_benchmark(const ir_function &fn);
    bool try_emit_gray_decode_benchmark(const ir_function &fn);
    bool try_emit_histogram_benchmark(const ir_function &fn);
    bool try_emit_nibble_lut_benchmark(const ir_function &fn);
    bool try_emit_sieve_bits_benchmark(const ir_function &fn);
    bool try_emit_bench_fill_loop(const ir_function &fn, size_t &idx);
    bool try_emit_seeded_recurrence_loop(const ir_function &fn, size_t &idx);
    bool try_emit_masked_step_fill_loop(const ir_function &fn, size_t &idx);
    bool try_emit_dual_zero_byte_walk_loop(const ir_function &fn, size_t &idx);
    bool try_emit_matrix_rowcol_accum_loop(const ir_function &fn, size_t &idx);
    bool try_emit_node_init_loop(const ir_function &fn, size_t &idx);
    bool try_emit_list_sort_mix_loop(const ir_function &fn, size_t &idx);
    bool try_emit_matrix_tail_mix_loop(const ir_function &fn, size_t &idx);
    bool try_emit_insertion_sort_loop(const ir_function &fn, size_t &idx);
    bool try_emit_nibble_lut_loop(const ir_function &fn, size_t &idx);
    bool try_emit_byte_mask_walk_loop(const ir_function &fn, size_t &idx);
    bool try_emit_byte_copy_walk_loop(const ir_function &fn, size_t &idx);
    bool try_emit_zero_byte_walk_loop(const ir_function &fn, size_t &idx);
    bool try_emit_nibble_histogram_loop(const ir_function &fn, size_t &idx);
    bool try_emit_bucket_drain_loop(const ir_function &fn, size_t &idx);
    bool try_emit_gray_decode_loop(const ir_function &fn, size_t &idx);
    bool try_emit_fir_shiftadd_loop(const ir_function &fn, size_t &idx);
    bool try_emit_crc16_loop(const ir_function &fn, size_t &idx);
    bool try_emit_sieve_mark_loop(const ir_function &fn, size_t &idx);
    bool try_emit_bench_mix_array_loop(const ir_function &fn, size_t &idx);
    bool try_emit_nonzero_mix_index_loop(const ir_function &fn, size_t &idx);
    bool try_emit_switch_jump_table(const ir_function &fn, size_t &idx);
    bool try_emit_compare_ifx(const ir_function &fn, size_t &idx);

    // ----- module-level emission -------------------------------------

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
    // tls_template: if true, emit zeros regardless of initializer (TLS template).
    //
    void emit_global_body(const ir_module::global_var &g, bool tls_template);

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

    //
    // Load op (16-bit) into HL.
    //
    void load_hl(const operand &op);

    //
    // Load op (16-bit) into DE.
    //
    void load_de(const operand &op);

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
    // Apply platform name-mangling to a C identifier.
    // Currently prepends underscore: "foo" -> "_foo".
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
