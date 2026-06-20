//
// z80peep.h — Z80 peephole optimizer for libxopt.
//
// z80_peep takes Z80 assembly text and applies a
// set of pattern-replacement rules to reduce code size and eliminate
// redundant instructions.  The optimization runs in multiple passes
// until no more rules fire.
//
// The simplest fixed-window peepholes are described in a small
// structural rule table, while the more context-sensitive patterns stay
// as custom matchers.
//
// Rules target patterns that xcc's code generator commonly produces:
//
//   ld r, r                                     — no-op self-load removed
//   push hl; pop hl                             — redundant push/pop removed
//   jp label; label:                            — jump to next line removed
//   jp [cc,] L  (final displacement in range)   — replaced with jr (saves 1 byte)
//   or a,a; or a,a                              — duplicate flag test removed
//   dec sp;dec sp;ld N(ix),l/h;ld l/h,N(ix)    — full temp store+reload elided
//   ld N(ix),l;ld N+1(ix),h;ld l,N(ix);ld h,…  — redundant 16-bit reload removed
//   ld N(ix),a;ld a,N(ix)                       — redundant byte reload removed
//   ld l,N(ix);ld h,N+1(ix);ld l,M(ix);ld h,…  — dead 16-bit IX load pair removed
//   push hl; ld hl,#n; pop de                   — replaced with ex de,hl; ld hl,#n
//   push hl; ld de,#n; pop hl                   — push/pop around neutral DE load removed
//   push hl; pop bc                             — replaced with ld b,h; ld c,l
//   ld a,(ix+N); ld (ix+N),a                   — self-store removed
//   ld hl,#n; ld hl,X                           — dead first load removed
//   push hl;ld hl,#0;pop de;or a,a;sbc hl,de   — zero-compare → ld a,h; or a,l
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include <string>
#include <vector>

namespace xopt {

// ----- asm_line ------------------------------------------------------
//
// A parsed assembly source line.  The peephole window operates over a
// vector of asm_line structs so rules can match on mnemonic and operand
// strings without re-parsing each time.

struct asm_line {
    std::string label;
    std::string mnemonic;
    std::string operands;
    std::string comment;
    bool        is_comment = false;
    bool        is_label   = false;
    bool        is_global_label = false;
    bool        label_has_colon = true;

    //
    // Parse a raw assembly text line into an asm_line.
    //
    static asm_line parse(const std::string &raw);

    //
    // Reconstruct the canonical text form of this line.
    //
    std::string to_string() const;
};

// ----- z80_peep ------------------------------------------------------

class z80_peep {
public:
    //
    // Apply all peephole rules to asm_text and return the optimised
    // assembly text.  Runs multiple passes until a fixed point is reached.
    //
    static std::string optimize(const std::string &asm_text,
                                bool speed_bias = false,
                                bool enable_spaghetti = false);

private:
    std::vector<asm_line> lines_;
    bool speed_bias_ = false;

    void load(const std::string &text);
    void apply_passes(int passes = 3);
    void apply_spaghetti_passes(int passes = 2);
    void apply_spaghetti_tail_passes(int passes = 2);
    bool apply_once();
    bool apply_spaghetti_once();
    bool apply_spaghetti_tail_once();
    std::string dump() const;

    // ----- pattern rules ---------------------------------------------

    // Remove ld r, r (self-load no-op).
    bool rule_redundant_ld(size_t i);

    // Remove push hl / pop hl pairs with nothing in between.
    bool rule_push_pop_hl(size_t i);

    // call target; ret  →  jp target
    // call target; label: ret  →  jp target; label: ret
    bool rule_call_ret_to_jp(size_t i);

    // push hl; pop de  →  ex de,hl  (no instructions between)
    bool rule_push_hl_pop_de(size_t i);

    // Remove jp label when label: is the immediately following line.
    bool rule_jp_next(size_t i);

    // Remove the second or a,a when two consecutive or a,a appear.
    bool rule_or_a_or_a(size_t i);

    // Elide dec sp;dec sp;ld N(ix),l;ld N+1(ix),h;ld l,N(ix);ld h,N+1(ix)
    // when the temp slot is not referenced anywhere after the reload.
    bool rule_temp_store_reload(size_t i);

    // push hl; ld hl,X; pop de  →  ex de,hl; ld hl,X  (any single ld hl)
    bool rule_push_hl_load_pop_de(size_t i);

    // push hl; ld l,N(ix); ld h,N+1(ix); pop de  →  ex de,hl; ld l,N(ix); ld h,N+1(ix)
    bool rule_push_hl_ix_pop_de(size_t i);

    // push hl; ld de,#imm; pop hl  →  ld de,#imm
    bool rule_push_hl_de_load(size_t i);

    // ld a,(ix+N); ld (ix+N),a  →  nothing (self-store no-op)
    bool rule_self_store(size_t i);

    // ld hl,#imm; ld hl,X  →  ld hl,X  (dead first immediate load)
    bool rule_dead_hl_load(size_t i);

    // ld a,#0  →  xor a  (when not immediately followed by a conditional branch)
    bool rule_ld_a_zero(size_t i);

    // ld N(ix),l; ld N+1(ix),h; ld l,N(ix); ld h,N+1(ix)  →  first 2 only
    // (store HL to IX slot then immediately reload it — HL is unchanged)
    bool rule_ix_store_reload(size_t i);

    // push hl; ld hl,#0; pop de; or a,a; sbc hl,de; jp z/nz,L
    //   →  ld a,h; or a,l; jp z/nz,L   (zero-compare shortcut)
    bool rule_zero_cmp_optimize(size_t i);

    // [push hl | ld b,h; ld c,l; ex de,hl]; ld hl,#0; or a,a; sbc hl,de;
    // ld hl,#1; jr/jp nz,L; dec hl; L:
    //   →  ld a,h; or a,l; ld hl,#1; jr/jp nz,L; dec hl; L:
    // Shrinks generic 16-bit "value != 0" boolean materialization when the
    // tested value is already in HL.
    bool rule_hl_nonzero_materialize(size_t i);

    // jp [cc,] L  →  jr [cc,] L  when the estimated final displacement is
    // within JR range. Only unconditional and z/nz/c/nc branches can be
    // shortened this way.
    bool rule_jp_to_jr(size_t i);

    // push hl; pop bc  →  ld b,h; ld c,l  (same size, faster)
    bool rule_push_hl_pop_bc(size_t i);

    // push de/bc; pop hl/de/bc  →  register-register copy
    // Same size as the stack shuttle, but much faster. Only enabled for
    // speed-biased presets so -Os output remains byte-for-byte conservative.
    bool rule_push_pair_copy_adjacent_speed(size_t i);

    // Experimental superoptimizer-inspired length-2 accumulator rewrites.
    // These are exact Z80 flag/value equivalents, but live only in the
    // speed-biased "here be dragons" lane.
    bool rule_superopt_accumulator_sequences(size_t i);

    // Experimental low-bit pair shortcuts, e.g. set 0,l; dec hl → res 0,l.
    bool rule_superopt_lowbit_pair_sequences(size_t i);

    // Experimental constant logical A shift:
    //   srl a repeated N times
    //     -> rrca/rlca rotates plus an AND mask, when flags cannot leak.
    bool rule_superopt_srl_a_const_shift(size_t i);

    // Experimental speed-biased 16-bit logical-right shift loop unroll:
    //   ld b,#5; L: srl l; rr a; djnz L
    //     -> srl l; rr a repeated 5 times
    // when the private loop label has no other users and B is dead until an
    // overwrite. This intentionally spends bytes for hot-loop speed.
    bool rule_superopt_shift5_pair_loop_unroll(size_t i);

    // Experimental loop-invariant high-byte zero hoist:
    //   L: ... ld hl,#base; ld d,#0; add hl,de; ... jr cc,L
    //     -> ld d,#0; L: ... ld hl,#base; add hl,de; ... jr cc,L
    // for simple single-backedge loops that use E as an 8-bit index.
    bool rule_superopt_hoist_d_zero_index_loop(size_t i);

    // Experimental short-span dead-flag-setter removal.  Only removes a
    // flag-only instruction when a nearby instruction overwrites all relevant
    // flags before any label, branch, carry read, or flags escape.
    bool rule_superopt_dead_flag_setter(size_t i);

    // Experimental register move cleanup: removes register round-trips and
    // pure dead register loads exposed by earlier speed-biased rewrites.
    bool rule_superopt_register_move_sequences(size_t i);

    // Experimental separated byte-immediate pair fold:
    //   ld b,#hi; <no C/BC touch>; ld c,#lo  ->  ld bc,#hilo; <gap>
    // and equivalent C/B, D/E, E/D, H/L, and L/H forms.
    bool rule_superopt_separated_pair_immediate_load(size_t i);

    // Experimental direct IX word increment:
    //   ld l,N(ix); ld h,N+1(ix); inc hl; ld N(ix),l; ld N+1(ix),h
    //     -> inc N(ix); jr nz,L; inc N+1(ix); L:
    // when the following path overwrites flags before any observer.
    bool rule_superopt_ix_word_inc_direct(size_t i);

    // Experimental direct IX byte increment:
    //   ld a,N(ix); add a,#1; ld N(ix),a  ->  inc N(ix)
    // when both A and flags are proven dead along the following path.
    bool rule_superopt_ix_byte_inc_direct(size_t i);

    // Experimental IX byte load forwarding:
    //   ld a,N(ix); ld r,a  ->  ld r,N(ix)
    // when the accumulator value is proven dead along the following path.
    bool rule_superopt_ix_byte_load_forward(size_t i);

    // Experimental HL byte load forwarding:
    //   ld a,(hl); ld r,a  ->  ld r,(hl)
    // when the accumulator value is proven dead along the following path.
    bool rule_superopt_hl_byte_load_forward(size_t i);

    // Experimental compare fallthrough reload cleanup:
    //   ld a,X; cp #n; jr cc,T; [unreferenced labels]; ld a,X
    //     -> ld a,X; cp #n; jr cc,T; [unreferenced labels]
    // because CP and the conditional branch preserve A on the fallthrough path.
    bool rule_superopt_compare_fallthrough_reload(size_t i);

    // Experimental transformed-compare fallthrough reload cleanup:
    //   ld a,X; xor #k; cp #n; jr cc,T; [unreferenced labels]; ld a,X; xor #k
    //     -> ld a,X; xor #k; cp #n; jr cc,T; [unreferenced labels]
    // because CP and the conditional branch preserve the transformed A value.
    bool rule_superopt_xor_compare_fallthrough_reload(size_t i);

    // Experimental zero-store chain cleanup:
    //   xor a; ld dst,a; xor a  ->  xor a; ld dst,a
    // because the store preserves both A and the flags produced by xor a.
    bool rule_superopt_redundant_zero_store_chain(size_t i);

    // Experimental zero-extend pair test shortcut:
    //   ld l,X; ld h,#0; ld b,h; ld c,l; ld h,b; ld l,c; ld a,h; or a,l
    //     -> ld l,X; ld h,#0; ld b,h; ld c,l; ld a,l; or a,a
    // preserving both HL and BC while testing the same zero-extended byte.
    bool rule_superopt_zero_extend_pair_test_shortcut(size_t i);

    // Experimental short accumulator-source forwarding:
    //   ld l,c; ld a,X; xor l; ld l,a
    //     -> ld a,X; xor c; ld l,a
    // when X does not read L/HL.
    bool rule_superopt_low_byte_xor_forward(size_t i);

    // Experimental dead-BC XOR forwarding:
    //   ld a,c; xor l; ld c,a; ld a,b; xor h; ld b,a; ld l,c; ld h,b
    //     -> ld a,l; xor c; ld l,a; ld a,h; xor b; ld h,a
    // when BC is overwritten before any later read.
    bool rule_superopt_dead_bc_xor_hl_forward(size_t i);

    // Experimental cold-C XOR forwarding:
    //   ld a,c; xor l; ld c,a; ld a,b; xor h; ld b,a; ld l,c; ld h,b
    //     -> ld a,c; xor l; ld l,a; ld a,b; xor h; ld b,a; ld h,a
    // when C is overwritten before any later read, but B may remain live.
    bool rule_superopt_dead_c_xor_hl_forward(size_t i);

    // Experimental DE xorshift shortcut:
    //   ld a,e; ld l,d; (srl l; rr a) * 5; xor e; ld e,a; ld a,l; xor d; ld d,a
    //     -> rotate/mask equivalent preserving final A, DE, L, and flags.
    bool rule_superopt_de_xor_right5(size_t i);

    // Experimental stack-backed HL xorshift shortcut:
    //   spill HL; (srl h; rr l) * 5; copy shifted to BC; reload HL; HL ^= BC
    //     -> spill HL; synthesize BC = HL >> 5 with rotate/mask; HL ^= BC.
    bool rule_superopt_hl_xor_right5_stack(size_t i);

    // Experimental zero-extend cleanup:
    //   ld l,a; ld h,#0; ld b,h; ld c,l; <HL overwrite>
    //     -> ld c,a; ld b,#0; <HL overwrite>
    bool rule_superopt_zero_extend_a_to_bc(size_t i);

    // Experimental generalized zero-extend cleanup:
    //   ld l,X; ld h,#0; ld b,h; ld c,l; <HL overwrite>
    //     -> ld c,X; ld b,#0; <HL overwrite>
    // when X can be loaded directly into C and HL dies before any read.
    bool rule_superopt_zero_extend_src_to_bc(size_t i);

    // Experimental zero-extend truth-test cleanup:
    //   ld c,a; ld b,#0; ld h,b; ld l,c; ld a,h; or a,l
    //     -> ld c,a; ld b,#0; ld h,b; ld l,c; or a,a
    bool rule_superopt_zero_extend_truth_test(size_t i);

    // Experimental dead-HL zero-test cleanup:
    //   ld c,a; ld b,#0; ld h,b; ld l,c; or a,a; jr cc,L
    //     -> ld c,a; ld b,#0; or a,a; jr cc,L
    // when both branch paths overwrite HL before reading it.
    bool rule_superopt_zero_extend_dead_hl_truth_test(size_t i);

    // Experimental dead-BC zero-test cleanup:
    //   ld c,a; ld b,#0; or a,a; jr cc,L
    //     -> or a,a; jr cc,L
    // when both branch paths overwrite BC before reading it.
    bool rule_superopt_zero_extend_dead_bc_truth_test(size_t i);

    // Experimental dead-DE pair-copy cleanup:
    //   ld h,d; ld l,e
    //     -> ex de,hl
    // when all following paths overwrite DE before reading it.
    bool rule_superopt_de_to_hl_dead_de_copy(size_t i);

    // Experimental return cleanup:
    //   ld b,h; ld c,l; ex de,hl; <epilogue>
    //     -> ex de,hl; <epilogue>
    // `BC` is caller-saved and not part of the modern return value.
    bool rule_superopt_dead_bc_return_copy(size_t i);

    // Experimental pair-copy forwarding:
    //   ld d,b; ld e,c; [or a,a;] sbc hl,de
    //     -> [or a,a;] sbc hl,bc
    // when DE is overwritten before any later read.
    bool rule_superopt_bc_to_de_alu_forward(size_t i);

    // Experimental dead-HL return-copy forwarding:
    //   ld h,d; ld l,e; ld b,h; ld c,l; ld M,l; ld N,h
    //     -> ld b,d; ld c,e; ld M,e; ld N,d
    // when HL is overwritten before any later read.
    bool rule_superopt_dead_hl_de_return_store_forward(size_t i);

    // Experimental modern-ABI constant return synthesis:
    //   ld hl,#imm; ex de,hl; <sdcccall(1) return tail>
    //     -> ld de,#imm; <return tail>
    // because the modern ABI returns 16-bit values in DE.
    bool rule_superopt_modern_const_return_direct(size_t i);

    // Experimental alternate-bank cancellation:
    //   exx; exx  ->  <removed>
    // when neither instruction carries a label.
    bool rule_superopt_cancel_exx_pair(size_t i);

    // Experimental call-argument DE direct load:
    //   ld hl,A; push hl; ld hl,B; ex de,hl; ld hl,C
    //     -> ld hl,A; push hl; ld de,B; ld hl,C
    // and, when A == B:
    //     -> ld de,A; push de; ld hl,C
    bool rule_superopt_call_arg_de_direct(size_t i);

    // Experimental dead-HL exchange cleanup:
    //   ld hl,A; ex de,hl  ->  ld de,A
    // when A is an immediate/symbol constant and all following paths
    // overwrite HL before reading it.
    bool rule_superopt_dead_hl_exchange_to_de_load(size_t i);

    // Experimental equal-pair exchange cleanup:
    //   <prove HL == DE>; ex de,hl  ->  <prove HL == DE>
    // for exact copy/reload shapes where the exchange is a no-op.
    bool rule_superopt_equal_de_hl_exchange(size_t i);

    // Experimental exchange-sandwich direct load:
    //   ex de,hl; ld hl,K; ex de,hl  ->  ld de,K
    // because the sandwich preserves the original HL and only loads DE.
    bool rule_superopt_exchange_sandwich_de_load(size_t i);

    // Experimental dead-pair stack discard:
    //   inc sp; inc sp  ->  pop rr
    // when every following path overwrites rr before reading it.
    bool rule_superopt_dead_pair_stack_discard_pop(size_t i);

    // Experimental dead restored-pair cleanup:
    //   pop rr; push rr  ->  <removed>
    // when every following path overwrites rr before reading it.
    bool rule_superopt_dead_pair_pop_push(size_t i);

    // Experimental long stack-discard cleanup:
    //   inc sp; inc sp; ...  ->  ld hl,#N; add hl,sp; ld sp,hl
    // for N >= 6 when HL and the changed flags die before observation.
    bool rule_superopt_long_inc_sp_run(size_t i);

    // Experimental Spaghetti helper ABI rewrite:
    //   push af; push bc; setup; call load-helper; pop bc; pop af
    //     -> setup; call preserving-load-helper
    // when every call to the helper has the exact wrapped shape.
    bool rule_superopt_spaghetti_load_helper_preserve_af_bc(size_t i);

    // Experimental Spaghetti helper ABI rewrite:
    //   push af; push bc; push de; setup; call store-helper; pop de; pop bc; pop af
    //     -> call preserving-store-helper
    // when every call to the helper has the exact wrapped shape.
    bool rule_superopt_spaghetti_store_helper_preserve_regs(size_t i);

    // Experimental Spaghetti flag-only inlining:
    //   call zero-test-helper; jr cc,L
    //     -> inline tiny zero test; jr cc,L
    // when every call only observes flags and the helper's result registers die
    // on both branch paths.
    bool rule_superopt_spaghetti_flag_helper_inline(size_t i);

    // push rr; <small span preserving rr>; pop rr  →  <span>
    // for rr in {hl,de,bc}. This is a conservative liveness-aware
    // extension of the adjacent push/pop removal.
    bool rule_push_pop_same_reg_span(size_t i);

    // push hl/de; <small span preserving both pairs>; pop de/hl
    //   →  ex de,hl; <span>
    // Generalizes the adjacent/existing load-specific rules to tiny
    // register-neutral spans.
    bool rule_push_pair_exchange_span(size_t i);

    // push hl/de; <small span preserving source and destination>; pop bc/hl
    //   →  ld hi,src_hi; ld lo,src_lo; <span>
    // Used for tiny save/restore shuffles that do not need the stack.
    bool rule_push_pair_copy_span(size_t i);

    // ld l,N(ix); ld h,N+1(ix); ld l,M(ix); ld h,M+1(ix)  →  last 2 only
    // (first 16-bit IX load is dead — immediately overwritten by second)
    bool rule_dead_hl_ix_load(size_t i);

    // ld N(ix),a; ld a,N(ix)  →  ld N(ix),a
    // (byte store to IX slot then immediate reload — A is unchanged)
    bool rule_ix_byte_store_reload(size_t i);

    // jr/jp cc1,L; ld hl,#A; jr/jp L_end; L: ld hl,#B; L_end: ld a,h; or a,l; jr/jp cc2,tgt
    // →  jr/jp combined_cc, tgt
    // Replaces the boolean generation + IFX test with a direct branch.
    bool rule_bool_ifx_shortcircuit(size_t i);

    // ld l,A(ix); ld h,A+1(ix); ex de,hl; ld l,B(ix); ld h,B+1(ix); ex de,hl
    // →  ld e,B(ix); ld d,B+1(ix); ld l,A(ix); ld h,A+1(ix)
    // Eliminates a double ex de,hl when IX loads are between them.
    bool rule_ex_de_hl_load_double(size_t i);

    // jr/jp cc,L; jr/jp L_end; L:  →  jr/jp !cc,L_end; L:
    // Inverts the condition so the unconditional skip is eliminated.
    // Only fires when L is not referenced from anywhere else.
    bool rule_invert_branch_skip(size_t i);

    // ----- helpers ---------------------------------------------------

    // Return true if the mnemonic + operands form a conditional branch.
    static bool is_conditional_branch(const asm_line &l);
};

} // namespace xopt
