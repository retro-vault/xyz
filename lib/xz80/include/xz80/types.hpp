// types.hpp — Z80 register and condition-code enumerations.
//
// Provides the canonical type-safe enumerations for all Z80 register
// operands and condition codes, plus constexpr encode helpers that map
// them to the opcode bit-fields defined in the Zilog Z80 CPU User Manual.
//
// These types are shared by the assembler (xas), compiler back-end (xcc),
// and the disassembler / emulator in this library.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once
#include <cstdint>

namespace xz80 {

// ---------------------------------------------------------------------------
// 8-bit register encoding  (bits 0..2 of a Z80 opcode byte)
// ---------------------------------------------------------------------------
enum class reg8 : uint8_t {
    B  = 0,
    C  = 1,
    D  = 2,
    E  = 3,
    H  = 4,
    L  = 5,
    HL_IND = 6, // (HL) — not a register, but uses slot 6 in encodings
    A  = 7
};

// ---------------------------------------------------------------------------
// 16-bit register pairs
// ---------------------------------------------------------------------------
enum class reg16 : uint8_t {
    BC = 0,
    DE = 1,
    HL = 2,
    SP = 3,
    AF = 4,   // only valid in PUSH/POP context
    IX = 5,
    IY = 6,
    PC = 7
};

// ---------------------------------------------------------------------------
// Condition codes  (bits 3..5 of JP/CALL/RET/JR opcodes)
// ---------------------------------------------------------------------------
enum class condition : uint8_t {
    NZ = 0,
    Z  = 1,
    NC = 2,
    C  = 3,
    PO = 4,
    PE = 5,
    P  = 6,
    M  = 7
};

// ---------------------------------------------------------------------------
// Encoding helpers — all constexpr so they can be used in constant contexts
// ---------------------------------------------------------------------------

// reg8 → opcode bit-field (0..7)
constexpr int encode_reg8(reg8 r) noexcept
{
    return static_cast<int>(r);
}

// reg16 → opcode field for most arithmetic (BC=0, DE=1, HL=2, SP=3)
// Returns -1 for AF, IX, IY, PC which are not valid in this encoding.
constexpr int encode_reg16_sp(reg16 r) noexcept
{
    switch (r) {
        case reg16::BC: return 0;
        case reg16::DE: return 1;
        case reg16::HL: return 2;
        case reg16::SP: return 3;
        default:        return -1;
    }
}

// reg16 → PUSH/POP encoding (BC=0, DE=1, HL=2, AF=3)
// Returns -1 for SP, IX, IY, PC.
constexpr int encode_reg16_af(reg16 r) noexcept
{
    switch (r) {
        case reg16::BC: return 0;
        case reg16::DE: return 1;
        case reg16::HL: return 2;
        case reg16::AF: return 3;
        default:        return -1;
    }
}

// condition → opcode bits (NZ=0 .. M=7)
constexpr int encode_condition(condition cc) noexcept
{
    return static_cast<int>(cc);
}

// Condition → JR-subset encoding (NZ=0, Z=1, NC=2, C=3).
// Returns -1 for PO, PE, P, M which are not valid for JR.
constexpr int encode_jr_condition(condition cc) noexcept
{
    switch (cc) {
        case condition::NZ: return 0;
        case condition::Z:  return 1;
        case condition::NC: return 2;
        case condition::C:  return 3;
        default:            return -1;
    }
}

} // namespace xz80
