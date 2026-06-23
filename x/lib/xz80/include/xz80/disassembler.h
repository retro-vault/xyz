// disassembler.h — stateless Z80 instruction disassembler.
//
// Design principles
// -----------------
//   Strategy  : instruction bytes are read through an IMemory reference
//               supplied by the caller, so the disassembler works with any
//               memory model — flat RAM, banked ROM, live CPU memory, etc.
//
//   Stateless : disassembler holds no mutable state; every call is
//               self-contained and thread-safe.
//
//   Value result : disasm_result is a plain value type containing both the
//                  structured fields and a formatted text representation.
//
// Output format
// -------------
//   Immediates  : #NN  (bytes) / #NNNN (words) — uppercase hex
//   Displacements : +N / -N   (signed decimal, e.g. IX+5, IX-3)
//   Relative targets : #NNNN (absolute address in hex)
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once
#include <array>
#include <cstdint>
#include <string>

#include <xz80/memory.h>

namespace xz80 {

// ---------------------------------------------------------------------------
// Result of disassembling one instruction
// ---------------------------------------------------------------------------

struct disasm_result {
    uint16_t             address{};         // address of the first byte
    std::array<uint8_t,4> bytes{};          // raw bytes (up to 4)
    uint8_t              length{};          // number of bytes consumed (1–4)
    std::string          text{};            // formatted mnemonic + operands
    uint8_t              t_states{};        // nominal T-state count
    uint8_t              t_states2{};       // alternate T-states (branch taken);
                                            // 0 means same as t_states
};

// ---------------------------------------------------------------------------
// Disassembler
// ---------------------------------------------------------------------------

class disassembler {
public:
    // Decode one instruction at addr from mem.
    // Advances an internal read cursor; the caller must inspect result.length
    // to find the address of the next instruction.
    disasm_result disassemble(uint16_t addr, const IMemory& mem) const;
};

} // namespace xz80
