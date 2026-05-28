// cpu_state.hpp — plain value object capturing all Z80 register state.
//
// cpu_state has no behaviour, no invariants, and no identity.  It is
// freely copyable and usable as a plain-old-data snapshot.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once
#include <cstdint>

namespace xz80 {

struct cpu_state {
    // Main register bank
    uint16_t af{}, bc{}, de{}, hl{};

    // Alternate (shadow) register bank
    uint16_t af2{}, bc2{}, de2{}, hl2{};

    // Index and control registers
    uint16_t ix{}, iy{};
    uint16_t sp{}, pc{};

    // Special-purpose registers
    uint8_t i{};   // interrupt vector high byte
    uint8_t r{};   // memory refresh counter

    // Interrupt state
    uint8_t im{};   // interrupt mode: 0, 1, or 2
    bool    iff1{}; // interrupt flip-flop 1 (enables maskable interrupts)
    bool    iff2{}; // interrupt flip-flop 2 (NMI save of IFF1)

    // Execution state
    bool    halted{};
};

} // namespace xz80
