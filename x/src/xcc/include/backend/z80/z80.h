//
// z80.h — Z80 register definitions and ABI constants.
//
// Defines the z80_reg enumeration (register indices used by the xcc
// Z80 backend) and the z80_cc enumeration (Z80 condition codes used
// when generating conditional jumps).
//
// xcc Z80 calling convention:
//
//   Parameters:
//     All arguments are pushed right-to-left onto the stack before
//     the call instruction.  The first parameter lives at IX+4,
//     subsequent parameters at IX+4+sizeof(prev), etc.
//
//   Return values:
//     1-byte result  -> L
//     2-byte result  -> HL
//     4-byte result  -> DE:HL  (DE = high word)
//
//   Callee saves:  IX only (push ix / pop ix in prologue/epilogue).
//   Stack cleanup: ABI-sensitive. sdcccall(0) is caller-clean; sdcccall(1)
//   also keeps stack-spilled arguments caller-clean.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include <string>
#include <cstdint>

namespace xcc {

// ----- z80_reg -------------------------------------------------------
//
// Register indices for Z80 registers.
// Used internally for future register allocation; current
// code generator spills all temporaries to the IX stack frame.

enum z80_reg : int {
    REG_A   = 0,
    REG_C   = 1,
    REG_B   = 2,
    REG_E   = 3,
    REG_D   = 4,
    REG_L   = 5,
    REG_H   = 6,
    REG_IYL = 7,
    REG_IYH = 8,
    REG_BC  = 9,
    REG_DE  = 10,
    REG_HL  = 11,
    REG_IY  = 12,
    REG_SP  = 13,
    REG_NONE = -1,
};

// ----- z80_cc --------------------------------------------------------
//
// Z80 condition codes for conditional jump emission.
// ALWAYS / NEVER are pseudo-conditions used by the peephole optimizer.

enum class z80_cc {
    Z,      // zero flag set     (equal)
    NZ,     // zero flag clear   (not equal)
    C,      // carry flag set
    NC,     // carry flag clear
    PE,     // parity even       (signed overflow)
    PO,     // parity odd
    M,      // sign minus
    P,      // sign plus
    ALWAYS,
    NEVER,
};

//
// Return the SDAS assembly mnemonic string for cc (e.g., "z", "nz").
// The returned pointer is static; do not free it.
//
const char *z80cc_name(z80_cc cc);

//
// Return the logical inverse of cc.
// e.g., z80cc_negate(z80_cc::Z) == z80_cc::NZ.
//
z80_cc z80cc_negate(z80_cc cc);

} // namespace xcc
