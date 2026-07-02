// ports.h — Z80 port I/O interface and Null Object implementation.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once
#include <cstdint>

namespace xz80 {

// ---------------------------------------------------------------------------
// Strategy interface
// ---------------------------------------------------------------------------

class IPorts {
public:
    virtual ~IPorts() = default;

    virtual uint8_t in (uint16_t port)             noexcept = 0;
    virtual void    out(uint16_t port, uint8_t val) noexcept = 0;
};

// ---------------------------------------------------------------------------
// Null Object: IN returns 0xFF, OUT is silently discarded.
// Use when no I/O hardware is modelled.
// ---------------------------------------------------------------------------

class null_ports final : public IPorts {
public:
    uint8_t in (uint16_t)             noexcept override { return 0xFF; }
    void    out(uint16_t, uint8_t)    noexcept override {}
};

} // namespace xz80
