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
//   ld hl,#n; ld l,X; ld h,Y                    — dead first load removed
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
                                int normal_passes = 10,
                                bool size_bias = false);

    // Perform only control-flow-size cleanup that is safe after outlining.
    // This deliberately excludes register and flag liveness rewrites because
    // an outlined helper's live-ins and live-outs are not ordinary call ABI.
    static std::string optimize_outlined_layout(const std::string &asm_text);

private:
    std::vector<asm_line> lines_;
    bool speed_bias_ = false;
    bool size_bias_ = false;

    void load(const std::string &text);
    void apply_passes(int passes = 3);
    bool apply_once();
    std::string dump() const;

    // ----- pattern rules ---------------------------------------------

    // Remove ld r, r (self-load no-op).
    bool rule_redundant_ld(size_t i);

    // Remove push hl / pop hl pairs with nothing in between.
    bool rule_push_pop_hl(size_t i);

    // Remove jp label when label: is the immediately following line.
    bool rule_jp_next(size_t i);

    // Remove the second or a,a when two consecutive or a,a appear.
    bool rule_or_a_or_a(size_t i);

    // Fold inclusive byte-threshold branches after cp #N.
    bool rule_cp_threshold_branch_fold(size_t i);

    // cp #0; <flag-preserving instructions>; jr/jp ordinary-cc,L
    //   -> or a,a; ... when parity/overflow is not observed.
    bool rule_cp_zero_branch_to_or(size_t i);

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

    // ld l,X; ld h,Y  → removed when HL is overwritten before any read.
    bool rule_dead_hl_pair_load(size_t i);

    // ld l,X; ld h,#0; ld l,Y; ld h,Z  →  ld l,Y; ld h,Z
    // when the second pair load does not read the old HL value.
    bool rule_dead_hl_zero_extend_before_pair_load(size_t i);

    // ld l,lo; ld h,hi; ld de,#0; or a,a; sbc hl,de; jp p/m,L
    //   → bit 7,hi; jp z/nz,L when HL/DE/flags die on both paths.
    bool rule_signed_zero_branch_from_high_bit(size_t i);

    // [ex de,hl;] ld hl,#K; or a,a; sbc hl,de; jr/jp z/nz,L
    //   → [ex de,hl;] ld a,e; sub #K; or d; jr/jp z/nz,L for byte K.
    bool rule_de_eq_small_const_branch(size_t i);

    // ld l,lo; ld h,hi; or a,a; sbc hl,de; jr nz,L
    //   → ld a,lo; cp e; jr nz,L; ld a,hi; cp d; jr nz,L.
    bool rule_de_word_ne_branch_split(size_t i);

    // ld a,src; xor #128; cp #128; jr/jp c/nc,L
    //   → bit 7,src; jr/jp nz/z,L when A/flags die on both paths.
    bool rule_signed_byte_zero_branch(size_t i);

    // ld l,lo; ld h,hi; ld de,#N00; or a,a; sbc hl,de; jp c/nc/p/m,L
    //   → ld a,hi; cp #N; jp c/nc/p/m,L for page-aligned bounds.
    bool rule_page_aligned_word_bound_branch(size_t i);

    // ld a,l; and #0; ld l,a; ld a,h; and #bit; ld h,a; ...; or a,l; jr/jp z/nz,L
    //   → bit n,h; jr/jp z/nz,L when the masked HL value dies on both paths.
    bool rule_hl_high_byte_mask_branch(size_t i);

    // ld word; and #onehot; store temp; zero high word; or temp; jr/jp z/nz,L
    //   → bit n,source; jr/jp z/nz,L when the temporary word dies.
    bool rule_word_mask_zero_high_branch(size_t i);

    // ld slot,a; bit 7,a; branch diamond around add/xor/store
    //   -> add a,a; jr nc,join; xor #K; join: ld slot,a.
    bool rule_bit7_shift_xor_diamond(size_t i);

    // ld l,a; ld h,#0; bit 7,l; branch diamond around byte shift/xor
    //   -> add a,a; jr nc,join; xor #K; join: ld slot,a.
    bool rule_zero_extended_bit7_shift_xor_diamond(size_t i);

    // Sign-extended byte add/sub/shift-xor sequences whose high byte is
    // immediately discarded -> direct low-byte arithmetic in A.
    bool rule_signed_byte_low_arith(size_t i);

    // ld e,X; ld d,Y; ld a,e; ALU a,d
    //   → ld a,X; ALU a,Y when DE is only a dead shuttle.
    bool rule_de_byte_alu_shuttle(size_t i);

    // ld c,X; ALU a,c
    //   → ALU a,X when C is only a dead byte shuttle.
    bool rule_c_byte_alu_shuttle(size_t i);

    // ld l,X; ld h,#0; (sra h; rr l)+; ld a,l
    //   → ld a,X; srl a... when the widened HL value is dead.
    bool rule_zero_extended_byte_shr_to_a(size_t i);

    // Zero-extended byte shift/add chains that only keep the low byte
    //   → compute directly in A when widened BC/DE/HL results are dead.
    bool rule_low_byte_shift_add_to_a(size_t i);

    // ld slot,a; one or more A shift/xor diamonds; ld same_slot,a
    //   -> remove the first store because A carries the value to the later
    //      same-slot store.
    bool rule_dead_a_store_before_join_store(size_t i);

    // ld byte_tmp,a; small branch/ALU region repeatedly uses byte_tmp
    //   → keep the byte in D when DE is dead after the region.
    bool rule_ix_byte_branch_temp_to_d(size_t i);

    // ld slot,hl; bit 7,h; branch diamond around add hl,hl / xor polynomial
    //   -> add hl,hl; jr nc,join; xor polynomial; join: ld slot,hl.
    bool rule_bit15_shift_xor_diamond(size_t i);

    // ld slot,hl; mask high bit through HL/BC; branch diamond around shift/xor
    //   -> carry-based shift/xor when A and BC die after the diamond.
    bool rule_masked_bit15_shift_xor_diamond(size_t i);

    // bit 0,slot; branch diamond around 32-bit right shift / xor polynomial
    //   -> shift; jr nc,join; xor polynomial; join: store 32-bit result.
    bool rule_lsb32_shift_xor_diamond(size_t i);

    // ld a,r; and #255; ld r,a  →  removed when A/flags are dead afterwards.
    bool rule_redundant_u8_self_mask(size_t i);

    // ld l,X; ld h,#0; ld a,l; and #bit; ...; or a,l; jr/jp z/nz,L
    //   → bit n,X; jr/jp z/nz,L
    // when A, HL, and flags die on both branch paths.
    bool rule_zero_extended_byte_mask_branch(size_t i);

    // ld a,X; add a,a; [xor #K through zero-extended HL]; ld T,l; ld T+1,h
    //   → compute in A and store T=a, T+1=#0 when HL/flags die afterwards.
    bool rule_zero_extended_byte_store_direct(size_t i);

    // ld l,X; ld h,#0; ld a,l; and #mask; ld l,a; inc hl; ld T,l; ld T+1,h
    //   → byte mask/inc in A and direct T=a, T+1=#0 stores when safe.
    bool rule_zero_extended_mask_inc_word_store(size_t i);

    // ld l,T; ld h,T+1; ld a,l  →  ld a,T
    // for indexed word reloads when HL dies after the low-byte extraction.
    bool rule_word_reload_low_byte_to_a(size_t i);

    // ld a,#0  →  xor a  (when not immediately followed by a conditional branch)
    bool rule_ld_a_zero(size_t i);

    // ld a,#N; ld (hl),a  →  ld (hl),#N
    // when A is dead after the store.
    bool rule_hl_immediate_store_direct(size_t i);

    // ld a,N(ix/iy); ld R,a  →  ld R,N(ix/iy)
    // when A is overwritten before any later read.
    bool rule_dead_a_indexed_load_shuttle(size_t i);

    // ld a,#N; ld N(ix/iy),a  →  ld N(ix/iy),#N
    // when A is overwritten before any later read.
    bool rule_dead_a_indexed_immediate_store(size_t i);

    // ld T(ix),a; cp/or...; jr/jp cc,L; [unreferenced labels]; ld a,T(ix)
    //   → keep A live across the preserving compare/branch path.
    bool rule_a_temp_reload_after_preserving_branch(size_t i);

    // ld a,(hl); ld T(ix),a; or a,a; jr z,L; ld l,T(ix); ld h,#0
    //   → keep the loaded byte in A and zero-extend it directly.
    bool rule_byte_temp_zero_extend_after_test(size_t i);

    // ld T(ix),e; ld T+1(ix),d; ld l,T(ix); ld h,T+1(ix); ex de,hl
    //   → keep the existing DE value when HL is dead afterwards.
    bool rule_dead_de_spill_reload_exchange(size_t i);

    // Forward a dead byte call-result spill into a commutative accumulator.
    bool rule_call_result_byte_commute_direct(size_t i);

    // ld d,h; ld e,l; <full HL reload>  →  ex de,hl; <full HL reload>
    // when the exchange's old-DE value in HL is overwritten before any read.
    bool rule_hl_to_de_before_hl_reload_exchange(size_t i);

    // ld h,B; ld l,C; inc/dec hl...  →  ld hl,#offset; add hl,bc
    // (also DE) when flags are dead after the materialized address.
    bool rule_pair_copy_offset_materialize(size_t i);

    // ld rr,#A; ld dd_hi,rr_hi; ld dd_lo,rr_lo; ld rr,#B
    //   → ld dd,#A; ld rr,#B
    // Also handles the common compiler form that routes the copy through BC
    // when BC is dead after the copied value reaches the destination pair.
    bool rule_pair_immediate_copy_reload_elide(size_t i);

    // ld hl,#BASE; spill BASE; ld hl,#LIMIT; ld de,#BASE; sbc; DE=HL; reload BASE
    //   → ld hl,#LIMIT; ld de,#BASE; sbc; ex de,hl
    // when the spill is in the compiler temp frame and dies after the reload.
    bool rule_temp_base_subtract_result_exchange(size_t i);

    // ld bc,#A; <no BC/DD touch>; ld dd_hi,b; ld dd_lo,c
    //   → ld dd,#A; <span>
    // when BC dies after the copy.
    bool rule_bc_immediate_copy_to_pair_direct(size_t i);

    // ld rr,#K; ld M(ix),rr_lo; ld M+1(ix),rr_hi
    //   → ld M(ix),#<K; ld M+1(ix),#>K
    // when rr dies after the store.
    bool rule_pair_immediate_store_direct(size_t i);

    // ld bc,#A; add hl,bc; ld de,#B; add hl,de
    //   → ld bc,#(A+B); add hl,bc
    // when BC, DE, and flags are dead after the folded address calculation.
    bool rule_const_add_bc_de_fold(size_t i);

    // ld N(ix),l; ld N+1(ix),h; ld l,N(ix); ld h,N+1(ix)  →  first 2 only
    // (store HL to IX slot then immediately reload it — HL is unchanged)
    bool rule_ix_store_reload(size_t i);

    // ld N(ix),l/h/e/d; [comments]; ld l,N(ix); ld h,N+1(ix)
    //   → remove the low-word reload because HL survives the stores.
    bool rule_ix_store32_low_reload(size_t i);

    // ld hl,P(ix); save HL to temps; inc P(ix); reload temps; ld a,(hl)
    //   → ld hl,P(ix); ld a,(hl); inc P(ix), preserving increment flags.
    bool rule_ix_postinc_indirect_load_a(size_t i);

    // ld hl,I(ix); save to temps; inc I(ix); base+temps; ld (hl),a
    //   → base+I(ix); ld (hl),a; inc I(ix), when flags are dead afterwards.
    bool rule_ix_postinc_indexed_store_a(size_t i);

    // Two adjacent base+index byte stores with index++ after each store
    //   → compute base+index once, use inc hl, then write index += 2.
    bool rule_adjacent_postinc_indexed_stores_direct(size_t i);

    // push ix; pop hl; ld bc,#N; add hl,bc; [inc hl...]; ld/xor a; ld (hl),a
    //   → ld/xor a; ld N+k(ix),a
    // when the computed address in HL is dead afterwards.
    bool rule_ix_local_byte_store_direct(size_t i);

    // ld a,I(ix); ld T(ix),a; inc I(ix); ld a,T(ix); widen; stack-local base;
    // add index; ld (hl),#N
    //   → keep the old index in A/DE and skip the temporary round-trip.
    bool rule_ix_postinc_local_immediate_store(size_t i);

    // Repeated stack-local immediate stores through the same byte index:
    //   ld a,I(ix); inc I(ix); ... base+I ...; ld (hl),#N
    //   ld a,I(ix); inc I(ix); ... base+I ...; ld (hl),#M
    //   ld a,I(ix);             ... base+I ...; ld (hl),#0
    //     -> compute base+I once and emit sequential stores with inc hl.
    bool rule_ix_indexed_stack_immediate_store_run(size_t i);

    // ld hl,#-N; add hl,sp; ld sp,hl
    //   → push af... [dec sp]
    // for small stack-frame allocations where the add flags are dead.
    bool rule_small_stack_alloc_push_af(size_t i);

    // ld rr,#N; ld (absolute),rr; ...; ld rr,#N
    //   → remove the repeated load while intervening instructions are only
    //     absolute stores from the unchanged pair.
    bool rule_redundant_pair_immediate_across_stores(size_t i);

    // push ix; pop hl; ld bc,#N; add hl,bc; [inc/dec hl]*
    //   → ld hl,#(N-current_sp_ix_delta); add hl,sp
    // when the fixed frame allocation and current stack delta are visible.
    bool rule_ix_addr_materialize_sp_relative(size_t i);

    // ld a,X; push af; inc sp; ld a,Y; push af; inc sp
    //   → ld b,X; ld c,Y; push bc
    // when A/BC are dead while constructing the call frame.
    bool rule_adjacent_byte_arg_push_pair(size_t i);

    // ld l,T(ix); ld h,T+1(ix); ld (hl),#N
    //   → ld K(ix),#N
    // when T is a temp-frame slot holding a known IX-relative address.
    bool rule_ix_temp_ptr_immediate_store_direct(size_t i);

    // ld hl,#K; add hl,sp; [inc/dec hl...] with dead HL
    //   → removed.
    bool rule_dead_hl_sp_frameaddr_calc(size_t i);

    // ld a,S(ix); add a,a*3; ld T(ix),a; ld e,S(ix); ld d,T(ix);
    // ld a,e; xor d; ld S(ix),a
    //   → keep S in E and shifted byte in D when T is dead afterwards.
    bool rule_ix_byte_left_shift_xor_temp_elide(size_t i);

    // ld S(ix),a; srl a*5; ld T(ix),a; ld e,S(ix); ld d,T(ix);
    // ld a,e; xor d; ld S(ix),a
    //   → keep S in E and shifted byte in D when T is dead afterwards.
    bool rule_ix_byte_right_shift_xor_temp_elide(size_t i);

    // sign-extend two IX bytes to 16-bit, xor them, then store only low byte
    //   → xor the bytes directly when flags/HL are dead afterwards.
    bool rule_truncated_promoted_byte_xor_elide(size_t i);

    // ld T,e; ld T+1,d; ld l,T; ld h,T+1; ld a,h; ...; ld a,l
    //   → use D/E directly for switch-key high/low byte tests.
    bool rule_ix_word_temp_switch_key_de_direct(size_t i);

    // ld T,e; ld T+1,d; [DE-preserving address calculation]; ld e,T; ld d,T+1
    //   → keep the word in DE when T/T+1 are temp-frame slots dead afterwards.
    bool rule_de_word_temp_reload_after_address_calc_elide(size_t i);

    // ld T,e; ld T+1,d; [SP-preserving span]; ld l,T; ld h,T+1
    //   → preserve the word with push de / pop hl instead of an IX temp.
    bool rule_de_word_temp_reload_to_hl_stack_preserve(size_t i);

    // ld T,l; ld T+1,h; [address calculation]; ld e,T; ld d,T+1; ld (hl),e...
    //   → keep the word result in DE and calculate the address with BC.
    bool rule_hl_word_temp_reload_after_address_calc_elide(size_t i);

    // ld l,I(ix); ld h,#0; add hl,hl; ld T(ix),l; ld T+1(ix),h;
    // ld l,B(ix); ld h,B+1(ix); ld e,T(ix); ld d,T+1(ix); add hl,de
    //   → keep doubled offset in DE directly when T/T+1 are dead afterwards.
    bool rule_ix_scaled_offset_temp_elide(size_t i);

    // ld l,I(ix); ld h,I+1(ix); [inc/dec hl]; add hl,hl;
    // ld T(ix),l; ld T+1(ix),h; ld hl,#BASE; ld e,T(ix); ld d,T+1(ix); add hl,de
    //   → keep the scaled offset in HL and add the static base directly.
    bool rule_ix_scaled_offset_immediate_base_elide(size_t i);

    // ld b,d; ld c,e; ld h,b; ld l,c; add hl,hl;
    // ld T(ix),l; ld T+1(ix),h; ld hl,#BASE; ld e,T(ix); ld d,T+1(ix); add hl,de
    //   → keep the scaled DE index in HL and add the static base directly.
    bool rule_de_scaled_offset_immediate_base_elide(size_t i);

    // ld hl,S(ix); spill to T(ix); loop: reload T; ld a,(hl); ...; inc T; jr loop
    //   → keep the loop scan pointer in HL when T dies at the forward exit.
    bool rule_ix_pointer_scan_temp_to_hl_loop(size_t i);

    // Collapse the temp-spilled 2*i + 4*i + 8*i expansion into register
    // shift-adds for byte-indexed arrays with 14-byte elements.
    bool rule_ix_index14_scaled_base_temp_elide(size_t i);

    // push hl; ld hl,#0; pop de; or a,a; sbc hl,de; jp z/nz,L
    //   →  ld a,h; or a,l; jp z/nz,L   (zero-compare shortcut)
    bool rule_zero_cmp_optimize(size_t i);

    // [push hl | ld b,h; ld c,l; ex de,hl]; ld hl,#0; or a,a; sbc hl,de;
    // ld hl,#1; jr/jp nz,L; dec hl; L:
    //   →  ld a,h; or a,l; ld hl,#1; jr/jp nz,L; dec hl; L:
    // Shrinks generic 16-bit "value != 0" boolean materialization when the
    // tested value is already in HL.
    bool rule_hl_nonzero_materialize(size_t i);

    // ld l,N(ix); ld h,N+1(ix); ld a,h; or a,l; jr/jp z/nz,L
    //   → ld a,N+1(ix); or N(ix); jr/jp z/nz,L
    // when HL is dead on both branch paths.
    bool rule_ix_word_zero_test_direct(size_t i);

    // ld T(ix),lo; ld T+1(ix),hi; ld a,T+1(ix); or T(ix); jr/jp z/nz,L
    //   → ld T(ix),lo; ld T+1(ix),hi; ld a,hi; or lo; jr/jp z/nz,L
    // for matching BC/DE/HL source pairs.
    bool rule_ix_word_store_zero_test_from_pair(size_t i);

    // ld T(ix),l; ld T+1(ix),h; ...; ld l,T(ix); ld h,T+1(ix);
    // ld a,h; or a,l; jr/jp z/nz,L
    //   → ld T(ix),l; ld T+1(ix),h; ...; ld a,h; or a,l; jr/jp z/nz,L
    // when the short intervening span preserves HL.
    bool rule_ix_hl_store_zero_test_reload_elide(size_t i);

    // jp [cc,] L  →  jr [cc,] L  when the estimated final displacement is
    // within JR range. Only unconditional and z/nz/c/nc branches can be
    // shortened this way.
    bool rule_jp_to_jr(size_t i);

    // push hl; pop bc  →  ld b,h; ld c,l  (same size, faster)
    bool rule_push_hl_pop_bc(size_t i);

    // push de; pop hl  →  ex de,hl
    // when the old DE value is overwritten before any later read.
    bool rule_push_de_pop_hl_to_ex(size_t i);

    // pop bc repeated 6+ times  →  ld hl,#bytes; add hl,sp; ld sp,hl
    // when HL and flags are dead after the stack adjustment.
    bool rule_pop_bc_run_sp_adjust(size_t i);

    // push de; pop hl; ld b,h; ld c,l[; ld a,l]
    //   →  ld b,d; ld c,e[; ld a,e]
    // when HL is dead on every following path.
    bool rule_dead_hl_de_stack_copy_to_bc(size_t i);

    // ld c,a; ld b,#0  → removed when BC is overwritten/clobbered before read.
    bool rule_dead_bc_zero_extend_from_a(size_t i);

    // ld b,h; ld c,l  → removed when BC is overwritten/clobbered before read.
    bool rule_dead_bc_copy_from_hl(size_t i);

    // ld b,h; ld c,l; ld a,(bc)/(bc),a  ->  ld a,(hl)/(hl),a
    // when BC is dead afterwards.
    bool rule_bc_indirect_through_hl(size_t i);

    // ld b,h; ld c,l; ld d,b; ld e,c  →  ld d,h; ld e,l
    // when BC is dead on every following path.
    bool rule_dead_bc_hl_to_de_copy(size_t i);
    bool rule_dead_bc_hl_roundtrip(size_t i);
    bool rule_bc_base_add_direct(size_t i);
    bool rule_bc_offset_base_add_de_direct(size_t i);
    bool rule_bc_index_add_hl_word_load_direct(size_t i);
    bool rule_bc_index_add_reloaded_hl_to_de(size_t i);
    bool rule_bc_saved_hl_push_word_to_de_direct(size_t i);

    // push de/bc; pop hl/de/bc  →  register-register copy
    // Same size as the stack shuttle, but much faster. Only enabled for
    // speed-biased presets so -Os output remains byte-for-byte conservative.
    bool rule_push_pair_copy_adjacent_speed(size_t i);

    // push de; pop hl; <HL/DE-preserving stores>; ex de,hl; ld hl,#X; ex de,hl
    //   → ld de,#X because the copy made HL == DE.
    bool rule_de_hl_equal_load_exchange(size_t i);

    // push de; pop hl; <stores/tests that only read H/L>
    //   → rewrite those reads to D/E and remove the stack copy when HL is dead.
    bool rule_de_result_hl_forward(size_t i);

    // Adjacent byte stores through base+index and base+index+1 followed by
    // index += 2  → compute the address once and use inc hl between stores.
    bool rule_adjacent_indexed_byte_stores_postinc(size_t i);

    // ld l,A(ix); ld h,A+1(ix); ex de,hl; ld l,B(ix); ld h,B+1(ix); or a,a; sbc hl,de
    //   → ld e,A(ix); ld d,A+1(ix); ld l,B(ix); ld h,B+1(ix); or a,a; sbc hl,de
    bool rule_ix_pair_compare_load_de_direct(size_t i);

    // Experimental superoptimizer-inspired length-2 accumulator rewrites.
    bool rule_superopt_accumulator_sequences(size_t i);

    // add a,#1 -> inc a when flags die before use.
    bool rule_add_a_one_to_inc(size_t i);

    // ld l,a; ld h,#0; ld a,l -> ld l,a; ld h,#0.
    bool rule_redundant_a_reload_after_zero_extend(size_t i);

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

    // Experimental temp-frame constant word forwarding:
    //   ld hl,#K; ld T(ix),l; ld T+1(ix),h; ...; ld rr_low,T(ix); ld rr_high,T+1(ix)
    //     -> ld rr,#K
    // for compiler temp-frame slots whose last direct definition is a
    // constant word store.
    bool rule_superopt_temp_const_word_load_direct(size_t i);

    // Experimental direct call-result store:
    //   call f; ex de,hl; [ld b,h; ld c,l;] ld M,l; ld N,h
    //     -> call f; [ld b,d; ld c,e;] ld M,e; ld N,d
    // when HL and DE are dead after the store, and BC is dead if its copy is
    // removed entirely.
    bool rule_superopt_call_result_store_de_direct(size_t i);

    // Experimental dead-BC store-copy cleanup:
    //   ld b,h; ld c,l; ld M,l; ld N,h  ->  ld M,l; ld N,h
    // when BC is overwritten before any later read.
    bool rule_superopt_dead_bc_store_copy(size_t i);

    // Experimental low-byte zero extension to DE:
    //   ld a,h; and #0; ld h,a; ld b,h; ld c,l; ld d,b; ld e,c
    //     -> and #0; ld d,a; ld e,l
    // when HL and BC are dead after the sequence.
    bool rule_superopt_lowbyte_zero_extend_to_de(size_t i);

    // Experimental direct IX word increment:
    //   ld l,N(ix); ld h,N+1(ix); inc hl; ld N(ix),l; ld N+1(ix),h
    //     -> inc N(ix); jr nz,L; inc N+1(ix); L:
    // when the following path overwrites HL and flags before any observer.
    bool rule_superopt_ix_word_inc_direct(size_t i);

    // Experimental direct IX word add-one:
    //   ld l,N(ix); ld h,N+1(ix); ld de,#1; add hl,de; store back
    //     -> inc N(ix); jr nz,L; inc N+1(ix); L:
    // when the following path overwrites HL, DE, and flags before any observer.
    bool rule_superopt_ix_word_add1_direct(size_t i);

    // ld rr,#small; add hl,rr  ->  inc/dec hl repeated
    // when the following path overwrites rr and flags before any observer.
    bool rule_add_hl_de_one_to_inc(size_t i);

    // Experimental direct IX byte increment:
    //   ld a,N(ix); add a,#1; ld N(ix),a  ->  inc N(ix)
    // when both A and flags are proven dead along the following path.
    bool rule_superopt_ix_byte_inc_direct(size_t i);

    // Direct IX byte increment with immediate zero test:
    //   ld a,N(ix); add a,#1; ld N(ix),a; or a,a; jr/jp z/nz,L
    //     -> inc N(ix); ld a,N(ix); or a,a; jr/jp z/nz,L
    bool rule_ix_byte_inc_test_direct(size_t i);

    // Direct register byte increment/decrement:
    //   ld a,r; add/sub a,#1; ld r,a  ->  inc/dec r
    // when A and flags are dead afterwards.
    bool rule_reg_byte_inc_dec_direct(size_t i);

    // __divuint remainder preservation cleanup:
    //   call __divuint; ld b,h; ld c,l; store DE; ld h,b; ld l,c; ld a,l; ld r,a
    //     -> call __divuint; [preserve needed B/C side effects]; store DE; ld a,l; ld r,a
    bool rule_divuint_remainder_bc_restore_elide(size_t i);

    // Direct IX byte decrement:
    //   ld a,N(ix); sub #1; ld N(ix),a  ->  dec N(ix)
    // when both A and flags are proven dead along the following path.
    bool rule_ix_byte_dec_direct(size_t i);

    // Fold A/E/D byte add shuttle:
    //   ld a,r; ld e,a; ld d,X; ld a,e; add a,d; ld r,a
    //     -> ld a,r; add a,X; ld r,a
    // when DE is dead afterwards.
    bool rule_reg_byte_add_shuttle_elide(size_t i);

    // Fold E/D byte add/sub shuttle back into a byte destination:
    //   ld e,X; ld d,Y; ld a,e; add/sub a,d; ld X,a
    //     -> ld a,X; add/sub a,Y; ld X,a
    // when DE is dead afterwards.
    bool rule_byte_addsub_shuttle_elide(size_t i);

    // Fold A-mediated E/D byte add/sub shuttle:
    //   ld e,X; ld a,Y; ld d,a; ld a,e; add/sub a,d; ld X,a
    //     -> ld a,X; add/sub a,Y; ld X,a
    // when DE is dead afterwards.
    bool rule_byte_addsub_a_to_d_shuttle_elide(size_t i);

    // Remove dead D copy in byte x*10 expansion:
    //   ld e,a; add a,a; add a,a; ld d,a; add a,e
    //     -> ld e,a; add a,a; add a,a; add a,e
    bool rule_dead_d_in_byte_mul10(size_t i);

    // Direct IX byte bit set:
    //   ld a,N(ix); or #bitmask; ld N(ix),a  ->  set bit,N(ix)
    // when the bitmask has one bit and both A and flags are dead afterwards.
    bool rule_ix_byte_or_mask_set_direct(size_t i);

    // Fold spilled 8-bit x*10:
    //   x2=temp(x+x); x8=temp(x<<3); x=x2+x8
    // into straight-line A/E/D arithmetic while preserving final A/D/E/flags.
    bool rule_ix_byte_mul10_spill_elide(size_t i);

    // Fold spilled 8-bit addition:
    //   ld a,X(ix); ld e,a; ld a,Y(ix); ld d,a; ld a,e; add a,d; ld Z(ix),a
    //     -> ld a,X(ix); add a,Y(ix); ld Z(ix),a
    // when DE is dead afterwards.
    bool rule_ix_byte_add_spill_elide(size_t i);

    // Fold spilled 8-bit addition feeding a zero/nonzero branch while keeping
    // the explicit or a,a flag normalization.
    bool rule_ix_byte_add_spill_branch_elide(size_t i);

    // cp #K; jr z,L; jr c,L; jr out  ->  cp #(K+1); jr c,L; jr out
    // when flags are dead at both branch targets.
    bool rule_unsigned_le_branch_fold(size_t i);

    // Experimental IX byte load forwarding:
    //   ld a,N(ix); ld r,a  ->  ld r,N(ix)
    // when the accumulator value is proven dead along the following path.
    bool rule_superopt_ix_byte_load_forward(size_t i);

    // Experimental direct IX byte ALU forwarding:
    //   ld e,X(ix); ld d,Y(ix); ld a,e; op a,d  ->  ld a,X(ix); op a,Y(ix)
    // when DE is overwritten before any later read.
    bool rule_superopt_ix_byte_alu_forward(size_t i);

    // Experimental zero-clear cleanup:
    //   ld a,r; and #0; ld r,a  ->  ld r,#0
    // when A and flags are overwritten before any observer.
    bool rule_superopt_dead_a_zero_reg(size_t i);

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

    // Experimental return cleanup:
    //   ld b,h; ld c,l; ld d,b; ld e,c; <epilogue>
    //     -> ld d,h; ld e,l; <epilogue>
    // `BC` is caller-saved and not part of the modern return value.
    bool rule_superopt_hl_via_bc_return_de_copy(size_t i);

    // Experimental modern-ABI direct return load:
    //   ld hl,#imm-or-(abs); ld d,h; ld e,l; <epilogue>
    //     -> ld de,#imm-or-(abs); <epilogue>
    // because the modern ABI returns 16-bit values in DE.
    bool rule_superopt_hl_load_return_de_direct(size_t i);

    // Experimental modern-ABI direct IX return load:
    //   ld l,N(ix); ld h,M(ix); ld d,h; ld e,l; <epilogue>
    //     -> ld e,N(ix); ld d,M(ix); <epilogue>
    // because the modern ABI returns 16-bit values in DE.
    bool rule_superopt_ix_word_return_de_direct(size_t i);

    // ld d,h; ld e,l; <modern return tail> -> ex de,hl; <tail>.
    bool rule_superopt_hl_return_de_exchange(size_t i);

    // Low-byte return synthesis:
    //   ld b,#0; ld hl,#K; add hl,bc; add hl,de; ld a,l; ret
    //     -> ld a,e; add a,c; add a,#K; ret
    // when only the byte return value in A escapes.
    bool rule_superopt_lowbyte_sum_return_direct(size_t i);

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

    // ld N(ix),a; ld r,N(ix)  →  ld N(ix),a; ld r,a
    // (immediate byte store forwarding for one-shot temporaries)
    bool rule_ix_byte_store_forward(size_t i);

    // ld T(ix),r where T is in the compiler temp frame and T is never
    // referenced again in the function  -> removed.
    bool rule_dead_temp_ix_store(size_t i);

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
