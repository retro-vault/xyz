// memory.h — Z80 memory access interface and flat-RAM implementation.
//
// IMemory is the Strategy interface used by both the CPU emulator and the
// disassembler.  Callers supply a concrete implementation, keeping the core
// library free from any particular memory-map policy.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace xz80 {

// ---------------------------------------------------------------------------
// Strategy interface
// ---------------------------------------------------------------------------

class IMemory {
public:
    virtual ~IMemory() = default;

    virtual uint8_t read (uint16_t addr)             const noexcept = 0;
    virtual void    write(uint16_t addr, uint8_t val)      noexcept = 0;
};

// ---------------------------------------------------------------------------
// Concrete implementation: flat 64 KiB RAM
// ---------------------------------------------------------------------------

// Models a plain 64 K address space with no banking, ROM, or I/O holes.
// Suitable for unit tests, the debugger target, and the xcc test runner.
class flat_memory final : public IMemory {
public:
    explicit flat_memory(uint8_t fill = 0x00) noexcept;

    uint8_t  read (uint16_t addr)             const noexcept override;
    void     write(uint16_t addr, uint8_t val)      noexcept override;

    // Bulk-load a contiguous block starting at origin.
    // Bytes past 0xFFFF are silently ignored.
    void load(uint16_t origin, std::span<const uint8_t> data) noexcept;

    // Direct read-only view of the whole address space.
    const std::array<uint8_t, 0x10000>& data() const noexcept;

private:
    std::array<uint8_t, 0x10000> data_;
};

} // namespace xz80
