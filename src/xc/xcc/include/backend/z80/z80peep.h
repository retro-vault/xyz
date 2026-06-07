//
// z80peep.h — Z80 peephole optimizer for the xcc compiler.
//
// z80_peep takes the assembly text produced by z80_gen and applies a
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

namespace xcc {

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
    static std::string optimize(const std::string &asm_text);

private:
    std::vector<asm_line> lines_;

    void load(const std::string &text);
    void apply_passes(int passes = 3);
    bool apply_once();
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

} // namespace xcc
