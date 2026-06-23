// cpu.h — Z80 CPU emulator.
//
// Design principles
// -----------------
//   Dependency Injection : IMemory and IPorts are injected at construction
//                          time and held by reference.  The caller owns them
//                          and is responsible for their lifetime.
//
//   Pimpl              : The vendor Z80 header is an implementation detail
//                        hidden inside cpu::impl.  Clients see only this
//                        interface header.
//
//   Single Responsibility : this class executes Z80 instructions and
//                           maintains CPU state only.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once
#include <cstdint>
#include <memory>

#include <xz80/cpu_state.h>
#include <xz80/memory.h>
#include <xz80/ports.h>
#include <xz80/types.h>

namespace xz80 {

class cpu {
public:
    // Construct with injected memory and port strategies.
    // Both references must remain valid for the lifetime of this object.
    explicit cpu(IMemory& mem, IPorts& ports);
    ~cpu();

    // Non-copyable: owns emulator context.
    cpu(const cpu&)            = delete;
    cpu& operator=(const cpu&) = delete;

    // -------------------------------------------------------------------
    // Execution
    // -------------------------------------------------------------------

    void reset()    noexcept;

    // Execute exactly one instruction.  Returns the number of T-states
    // consumed (4–23 for documented opcodes).
    int  step()     noexcept;

    // Raise a non-maskable interrupt.
    void nmi()      noexcept;

    // Raise a maskable interrupt with the given data-bus vector.
    // Returns false if IFF1 is clear (interrupt is masked).
    bool interrupt(uint8_t vector) noexcept;

    // -------------------------------------------------------------------
    // State inspection
    // -------------------------------------------------------------------

    bool     halted() const noexcept;
    uint16_t pc()     const noexcept;

    // Full register snapshot / restore.
    cpu_state snapshot()                  const noexcept;
    void      restore(const cpu_state& s)       noexcept;

    // Individual 16-bit register access.
    uint16_t reg    (reg16 r)           const noexcept;
    void     set_reg(reg16 r, uint16_t v)     noexcept;

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace xz80
