// cpu.cpp — Z80 CPU implementation (SUZUKI PLAN back-end).
//
// The SUZUKI PLAN emulator is a MIT-licensed, single-header C++ Z80
// implementation vendored at src/vendor/z80.hpp.  It is an implementation
// detail: clients depend only on cpu.hpp.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih

// The SUZUKI PLAN header triggers some pedantic warnings we don't own.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "vendor/z80.hpp"
#pragma GCC diagnostic pop

#include <xz80/cpu.hpp>

namespace xz80 {

// ---------------------------------------------------------------------------
// Pimpl body
// ---------------------------------------------------------------------------

struct cpu::impl {
    IMemory& mem;
    IPorts&  ports;
    Z80      z80;

    impl(IMemory& m, IPorts& p) : mem(m), ports(p) {
        z80.setupCallback(
            [](void* arg, unsigned short addr) -> unsigned char {
                return static_cast<impl*>(arg)->mem.read(
                    static_cast<uint16_t>(addr));
            },
            [](void* arg, unsigned short addr, unsigned char val) {
                static_cast<impl*>(arg)->mem.write(
                    static_cast<uint16_t>(addr),
                    static_cast<uint8_t>(val));
            },
            [](void* arg, unsigned short port) -> unsigned char {
                return static_cast<impl*>(arg)->ports.in(
                    static_cast<uint16_t>(port));
            },
            [](void* arg, unsigned short port, unsigned char val) {
                static_cast<impl*>(arg)->ports.out(
                    static_cast<uint16_t>(port),
                    static_cast<uint8_t>(val));
            },
            this);
    }
};

// ---------------------------------------------------------------------------
// cpu public interface
// ---------------------------------------------------------------------------

cpu::cpu(IMemory& mem, IPorts& ports)
    : impl_(std::make_unique<impl>(mem, ports))
{}

cpu::~cpu() = default;

static constexpr uint8_t k_iff_halt = 0x80; // Z80::IFF_HALT() bit

void cpu::reset() noexcept
{
    impl_->z80.reg = {};
}

int cpu::step() noexcept
{
    // Passing 1 causes execute() to run exactly one instruction:
    // after one opcode (minimum 4 T-states) the clock goes negative
    // and the loop exits.
    return impl_->z80.execute(1);
}

void cpu::nmi() noexcept
{
    impl_->z80.generateNMI(0x0066); // standard NMI vector
}

bool cpu::interrupt(uint8_t vector) noexcept
{
    if (!(impl_->z80.reg.IFF & 0x01)) // IFF1 — maskable-interrupt enable
        return false;
    impl_->z80.generateIRQ(vector);
    return true;
}

bool cpu::halted() const noexcept
{
    return (impl_->z80.reg.IFF & k_iff_halt) != 0;
}

uint16_t cpu::pc() const noexcept
{
    return impl_->z80.reg.PC;
}

// ---------------------------------------------------------------------------
// Snapshot / restore helpers
// ---------------------------------------------------------------------------

static uint16_t make16(uint8_t hi, uint8_t lo) noexcept
{
    return static_cast<uint16_t>((hi << 8) | lo);
}

cpu_state cpu::snapshot() const noexcept
{
    const auto& r = impl_->z80.reg;
    cpu_state s;
    s.af  = make16(r.pair.A, r.pair.F);
    s.bc  = make16(r.pair.B, r.pair.C);
    s.de  = make16(r.pair.D, r.pair.E);
    s.hl  = make16(r.pair.H, r.pair.L);
    s.af2 = make16(r.back.A, r.back.F);
    s.bc2 = make16(r.back.B, r.back.C);
    s.de2 = make16(r.back.D, r.back.E);
    s.hl2 = make16(r.back.H, r.back.L);
    s.ix  = r.IX;
    s.iy  = r.IY;
    s.sp  = r.SP;
    s.pc  = r.PC;
    s.i   = r.I;
    s.r   = r.R;
    s.im  = r.interrupt & 0x03;
    s.iff1   = (r.IFF & 0x01) != 0;
    s.iff2   = (r.IFF & 0x02) != 0;
    s.halted = (r.IFF & k_iff_halt) != 0;
    return s;
}

void cpu::restore(const cpu_state& s) noexcept
{
    auto& r = impl_->z80.reg;
    r.pair.A = static_cast<uint8_t>(s.af >> 8);
    r.pair.F = static_cast<uint8_t>(s.af);
    r.pair.B = static_cast<uint8_t>(s.bc >> 8);
    r.pair.C = static_cast<uint8_t>(s.bc);
    r.pair.D = static_cast<uint8_t>(s.de >> 8);
    r.pair.E = static_cast<uint8_t>(s.de);
    r.pair.H = static_cast<uint8_t>(s.hl >> 8);
    r.pair.L = static_cast<uint8_t>(s.hl);
    r.back.A = static_cast<uint8_t>(s.af2 >> 8);
    r.back.F = static_cast<uint8_t>(s.af2);
    r.back.B = static_cast<uint8_t>(s.bc2 >> 8);
    r.back.C = static_cast<uint8_t>(s.bc2);
    r.back.D = static_cast<uint8_t>(s.de2 >> 8);
    r.back.E = static_cast<uint8_t>(s.de2);
    r.back.H = static_cast<uint8_t>(s.hl2 >> 8);
    r.back.L = static_cast<uint8_t>(s.hl2);
    r.IX = s.ix;
    r.IY = s.iy;
    r.SP = s.sp;
    r.PC = s.pc;
    r.I  = s.i;
    r.R  = s.r;
    r.interrupt = (r.interrupt & 0xFC) | (s.im & 0x03);
    uint8_t iff = 0;
    if (s.iff1)   iff |= 0x01;
    if (s.iff2)   iff |= 0x02;
    if (s.halted) iff |= k_iff_halt;
    r.IFF = iff;
}

uint16_t cpu::reg(reg16 r) const noexcept
{
    const auto& regs = impl_->z80.reg;
    switch (r) {
        case reg16::BC: return make16(regs.pair.B, regs.pair.C);
        case reg16::DE: return make16(regs.pair.D, regs.pair.E);
        case reg16::HL: return make16(regs.pair.H, regs.pair.L);
        case reg16::SP: return regs.SP;
        case reg16::AF: return make16(regs.pair.A, regs.pair.F);
        case reg16::IX: return regs.IX;
        case reg16::IY: return regs.IY;
        case reg16::PC: return regs.PC;
    }
    return 0;
}

void cpu::set_reg(reg16 r, uint16_t v) noexcept
{
    auto& regs = impl_->z80.reg;
    switch (r) {
        case reg16::BC:
            regs.pair.B = static_cast<uint8_t>(v >> 8);
            regs.pair.C = static_cast<uint8_t>(v);
            break;
        case reg16::DE:
            regs.pair.D = static_cast<uint8_t>(v >> 8);
            regs.pair.E = static_cast<uint8_t>(v);
            break;
        case reg16::HL:
            regs.pair.H = static_cast<uint8_t>(v >> 8);
            regs.pair.L = static_cast<uint8_t>(v);
            break;
        case reg16::SP: regs.SP = v; break;
        case reg16::AF:
            regs.pair.A = static_cast<uint8_t>(v >> 8);
            regs.pair.F = static_cast<uint8_t>(v);
            break;
        case reg16::IX: regs.IX = v; break;
        case reg16::IY: regs.IY = v; break;
        case reg16::PC: regs.PC = v; break;
    }
}

} // namespace xz80
