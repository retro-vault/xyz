// runtime_machine.hpp — Z80 runtime function test harness.
//
// Loads the compiled runtime binary into a flat xz80 memory image, then
// provides typed call helpers that mirror each SDCC sdcccall(1) ABI variant
// used by the runtime:
//
//   call16          HL=a, DE=b  →  result in HL or DE
//   call16_sret     HL=a, DE=b, hidden result pointer on stack
//   call8           A=a, L=b   →  result in DE (quotient) and HL (remainder)
//   call_shift      HL=val, B=count  →  result in HL
//   call32          DE:HL=a(lo:hi), b on stack  →  result DE:HL(lo:hi)
//   call_float1     float a in HL:DE (HL=high), float b on stack
//   call_float_stackonly  entire float arg on stack (fsneg)
//
// 64-bit / double ABI used by the active long-long and double suites:
//
// The Z80 has four 16-bit alternate register pairs (DE', HL', BC', AF').
// Together with the main DE and HL they provide 64 bits of register
// storage — enough for the first argument and the return value:
//
//   First arg / return value layout:
//     DE  = bits[15: 0]  (lsb word)
//     HL  = bits[31:16]
//     DE' = bits[47:32]  (cpu_state::de2)
//     HL' = bits[63:48]  (cpu_state::hl2)
//
//   Second arg:  on stack (8 bytes).
//   Stack frame after fn's "push ix; ld ix,#0; add ix,sp":
//     ix+4 ..ix+11  = arg2 (b0=lsb … b7=msb)
//
//   call64           a in DE:HL:DE':HL', b on stack → result DE:HL:DE':HL'
//   call64_1arg      a in DE:HL:DE':HL', no stack arg → result same regs
//                    (used for unary ops: ___ll2sint, __dbneg, etc.)
//   call64_from_int  int16 in HL → 64-bit result (for ___sint2ll, ___uint2ll)
//   call64_from_long int32 in DE:HL → 64-bit result (for ___slong2ll)
//   call64_from_float float32 in HL:DE → 64-bit result (for ___fs2db)
//   result64_regs    reads 64-bit from DE:HL:DE':HL'
//   result_double_regs  same, reinterprets as double
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <span>

#include <xz80/xz80.h>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint16_t HALT_ADDR  = 0xFF00;  // HALT sentinel address
static constexpr uint16_t STACK_BASE = 0xFE00;  // initial SP (grows down)
static constexpr int      MAX_STEPS  = 300000;  // step budget per call

// ---------------------------------------------------------------------------
// runtime_machine
// ---------------------------------------------------------------------------

struct runtime_machine {
    xz80::flat_memory mem;
    xz80::null_ports  ports;
    xz80::cpu         cpu;

    // Load binary at address 0; place HALT sentinel at HALT_ADDR.
    explicit runtime_machine(std::span<const uint8_t> code)
        : cpu(mem, ports)
    {
        cpu.reset();
        mem.load(0x0000, code);
        mem.write(HALT_ADDR, 0x76); // HALT
    }

    // -------------------------------------------------------------------
    // Low-level helpers
    // -------------------------------------------------------------------

    // Push 2 bytes (lo, hi) onto a stack pointer value; return new SP.
    uint16_t push16(uint16_t sp, uint16_t val)
    {
        sp -= 2;
        mem.write(sp,   val & 0xFF);
        mem.write(sp+1, (val >> 8) & 0xFF);
        return sp;
    }

    // Push a 32-bit arg per the sdcccall(1) ABI:
    //   PUSH high_word (bytes b3,b2), then PUSH low_word (bytes b1,b0).
    // After the function's "push ix; ld ix,#0; add ix,sp":
    //   ix+4=b0, ix+5=b1, ix+6=b2, ix+7=b3
    uint16_t push32_arg(uint16_t sp, uint32_t val)
    {
        uint16_t hi = (val >> 16) & 0xFFFF;
        uint16_t lo = val & 0xFFFF;
        sp = push16(sp, hi);  // push high word first
        sp = push16(sp, lo);  // push low word second
        return sp;
    }

    // Execute from PC=fn until HALT or MAX_STEPS.  Returns true on halt.
    bool run_to_halt(uint16_t fn_addr, uint16_t sp)
    {
        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn_addr;
        cpu.restore(s);
        for (int i = 0; i < MAX_STEPS; ++i) {
            cpu.step();
            if (cpu.halted()) return true;
        }
        return false; // timed out
    }

    // Snapshot helper
    xz80::cpu_state snap() { return cpu.snapshot(); }

    // -------------------------------------------------------------------
    // Typed call helpers
    // -------------------------------------------------------------------

    // 16-bit ops: HL=a, DE=b.
    // Returns true on success; caller reads result from snapshot().
    bool call16(uint16_t fn, uint16_t hl, uint16_t de)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR); // return address

        xz80::cpu_state s{};
        s.hl = hl;
        s.de = de;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // 16-bit register arguments with a hidden aggregate-result pointer.
    // After the callee saves IX, ix+2 is the return address and ix+4 is
    // result_ptr, matching the sdcccall(0/1) small-aggregate convention.
    bool call16_sret(uint16_t fn, uint16_t hl, uint16_t de,
                     uint16_t result_ptr)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, result_ptr);
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = hl;
        s.de = de;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // 8-bit ops: A=a (high byte of AF), L=b (low byte of HL).
    bool call8(uint16_t fn, uint8_t a, uint8_t b)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.af = (uint16_t)a << 8;   // A in high byte
        s.hl = (uint16_t)b;        // b in L (H=0)
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Shift ops: HL=value, B=count (B is high byte of BC).
    bool call_shift(uint16_t fn, uint16_t hl, uint8_t count)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = hl;
        s.bc = (uint16_t)count << 8; // B = count, C = 0
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // 32-bit ops with stack arg: DE:HL=a (DE=low16, HL=high16), b on stack.
    bool call32(uint16_t fn, uint32_t a, uint32_t b)
    {
        uint16_t a_lo = a & 0xFFFF;
        uint16_t a_hi = (a >> 16) & 0xFFFF;

        uint16_t sp = STACK_BASE;
        sp = push32_arg(sp, b);     // push b (high word first, then low)
        sp = push16(sp, HALT_ADDR); // return address

        xz80::cpu_state s{};
        s.de = a_lo;
        s.hl = a_hi;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // 16×16→32 helpers (mulsint2slong/muluint2ulong): HL=a, DE=b.
    // Result: DE:HL (DE=low, HL=high).
    bool call16x16to32(uint16_t fn, uint16_t hl, uint16_t de)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = hl;
        s.de = de;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Float 2-arg: a in HL:DE (HL=high, DE=low), b on stack.
    // Used by fsadd, fssub, fsmul, fsdiv.
    bool call_float2(uint16_t fn, float a, float b)
    {
        uint32_t a_bits, b_bits;
        std::memcpy(&a_bits, &a, sizeof a);
        std::memcpy(&b_bits, &b, sizeof b);

        uint16_t sp = STACK_BASE;
        sp = push32_arg(sp, b_bits);
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = (a_bits >> 16) & 0xFFFF; // H=a3, L=a2
        s.de = a_bits & 0xFFFF;          // D=a1, E=a0
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Float 3-arg: a in HL:DE, then c and b on the stack so the callee sees
    // b at ix+4..7 and c at ix+8..11.
    bool call_float3(uint16_t fn, float a, float b, float c)
    {
        uint32_t a_bits, b_bits, c_bits;
        std::memcpy(&a_bits, &a, sizeof a);
        std::memcpy(&b_bits, &b, sizeof b);
        std::memcpy(&c_bits, &c, sizeof c);

        uint16_t sp = STACK_BASE;
        sp = push32_arg(sp, c_bits);
        sp = push32_arg(sp, b_bits);
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = (a_bits >> 16) & 0xFFFF;
        s.de = a_bits & 0xFFFF;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Float 2-arg + pointer: a in HL:DE, b on the stack, pointer after that.
    bool call_float2_ptr(uint16_t fn, float a, float b, uint16_t ptr)
    {
        uint32_t a_bits, b_bits;
        std::memcpy(&a_bits, &a, sizeof a);
        std::memcpy(&b_bits, &b, sizeof b);

        uint16_t sp = STACK_BASE;
        sp = push16(sp, ptr);
        sp = push32_arg(sp, b_bits);
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = (a_bits >> 16) & 0xFFFF;
        s.de = a_bits & 0xFFFF;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Float compare/eq/lt: a in HL:DE, b on stack.
    // Used by fscmp, fseq, fslt (callee-cleans stack).
    bool call_float_cmp(uint16_t fn, float a, float b)
    {
        return call_float2(fn, a, b); // same stack layout
    }

    // Float 1-arg in regs: a in HL:DE.
    // Used by fs2sint, fs2uint, fs2slong, fs2ulong, fs2schar, fs2uchar.
    bool call_float1(uint16_t fn, float a)
    {
        uint32_t bits;
        std::memcpy(&bits, &a, sizeof bits);

        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = (bits >> 16) & 0xFFFF;
        s.de = bits & 0xFFFF;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Float 1-arg entirely on stack (fsneg).
    bool call_float_stack(uint16_t fn, float a)
    {
        uint32_t bits;
        std::memcpy(&bits, &a, sizeof bits);

        uint16_t sp = STACK_BASE;
        sp = push32_arg(sp, bits);
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // float _Complex 1-arg entirely on stack: real first, imag second.
    // Public complex helpers like cabsf/cargf/conjf receive:
    //   ix+4..7   = real float
    //   ix+8..11  = imag float
    bool call_complex1(uint16_t fn, float real, float imag)
    {
        uint32_t real_bits, imag_bits;
        std::memcpy(&real_bits, &real, sizeof real_bits);
        std::memcpy(&imag_bits, &imag, sizeof imag_bits);

        uint16_t sp = STACK_BASE;
        sp = push32_arg(sp, imag_bits);
        sp = push32_arg(sp, real_bits);
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Int-to-float (uint2fs, sint2fs, schar2fs, uchar2fs): HL=int.
    // Result in HL:DE (HL=high, DE=low).
    bool call_int_to_float(uint16_t fn, uint16_t hl_val)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = hl_val;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Char-to-float (schar2fs, uchar2fs): A=char value.
    bool call_char_to_float(uint16_t fn, uint8_t a_val)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.af = (uint16_t)a_val << 8;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Long-to-float (slong2fs, ulong2fs): HL:DE = 32-bit (HL=high, DE=low).
    // Result in HL:DE (HL=high, DE=low).
    bool call_long_to_float(uint16_t fn, uint32_t val)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = (val >> 16) & 0xFFFF;
        s.de = val & 0xFFFF;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // -------------------------------------------------------------------
    // Result readers
    // -------------------------------------------------------------------

    // Standard 32-bit result: value = (HL<<16)|DE where HL=high, DE=low.
    uint32_t result32() const
    {
        auto s = cpu.snapshot();
        return ((uint32_t)s.hl << 16) | (uint32_t)s.de;
    }

    // Float from standard register layout: HL=high, DE=low.
    float result_float_hlde() const
    {
        uint32_t bits = result32();
        float f;
        std::memcpy(&f, &bits, sizeof f);
        return f;
    }

    // Float from inverted register layout (fsneg): DE=high, HL=low.
    float result_float_dehl() const
    {
        auto s = cpu.snapshot();
        uint32_t bits = ((uint32_t)s.de << 16) | (uint32_t)s.hl;
        float f;
        std::memcpy(&f, &bits, sizeof f);
        return f;
    }

    // Signed 16-bit from DE (used by fscmp).
    int16_t result_de_s16() const
    {
        auto s = cpu.snapshot();
        return (int16_t)s.de;
    }

    // Unsigned 8-bit from A register (fseq, fslt, fs2schar, fs2uchar).
    uint8_t result_a() const
    {
        auto s = cpu.snapshot();
        return (uint8_t)(s.af >> 8);
    }

    // Signed 8-bit from A register.
    int8_t result_a_s8() const
    {
        return (int8_t)result_a();
    }

    // -------------------------------------------------------------------
    // 64-bit call helpers (proposed ABI for long long / double)
    //
    // Register layout for first arg AND result:
    //   DE  = bits[15: 0]  (lsb)
    //   HL  = bits[31:16]
    //   DE' = bits[47:32]  (cpu_state::de2)
    //   HL' = bits[63:48]  (cpu_state::hl2)
    //
    // Second arg (if any) on stack via push64_arg.
    // Stack frame after fn's push-ix: ix+4..ix+11 = arg2 (b0=lsb..b7=msb)
    // -------------------------------------------------------------------

    // Pack a 64-bit value into cpu_state registers.
    static void set_regs64(xz80::cpu_state& s, uint64_t v)
    {
        s.de  = (uint16_t)(v & 0xFFFFu);
        s.hl  = (uint16_t)((v >> 16) & 0xFFFFu);
        s.de2 = (uint16_t)((v >> 32) & 0xFFFFu);
        s.hl2 = (uint16_t)((v >> 48) & 0xFFFFu);
    }

    // Push arg2 on stack: high32 first, then low32.
    uint16_t push64_arg(uint16_t sp, uint64_t val)
    {
        sp = push32_arg(sp, (uint32_t)(val >> 32));
        sp = push32_arg(sp, (uint32_t)(val & 0xFFFFFFFFu));
        return sp;
    }

    // First arg in DE:HL:DE':HL', second on stack.
    // Result in DE:HL:DE':HL'.
    bool call64(uint16_t fn, uint64_t a, uint64_t b)
    {
        uint16_t sp = STACK_BASE;
        sp = push64_arg(sp, b);
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        set_regs64(s, a);
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // Unary: single 64-bit arg in DE:HL:DE':HL', no stack arg.
    // Result in DE:HL:DE':HL'.
    // Used by: __dbneg, ___ll2sint, ___ll2slong, ___db2sint, ___db2slong,
    //          ___db2fs, ___ll2uint, ___db2sll, ___db2ull, etc.
    bool call64_1arg(uint16_t fn, uint64_t a)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        set_regs64(s, a);
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // int16 in HL, no alternates → 64-bit result in DE:HL:DE':HL'.
    // Used by: ___sint2ll, ___uint2ll, ___sint2db, ___uint2db.
    bool call64_from_int(uint16_t fn, uint16_t hl_val)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = hl_val;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // int32 in DE:HL (DE=low16, HL=high16) → 64-bit result.
    // Used by: ___slong2ll, ___ulong2ll, ___slong2db, ___ulong2db.
    bool call64_from_long(uint16_t fn, uint32_t val)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.de = val & 0xFFFF;
        s.hl = (val >> 16) & 0xFFFF;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // float32 in HL:DE (HL=high word, DE=low word) → 64-bit result.
    // Used by: ___fs2db.
    bool call64_from_float(uint16_t fn, float f)
    {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof bits);
        uint16_t sp = STACK_BASE;
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.hl = (bits >> 16) & 0xFFFF;
        s.de = bits & 0xFFFF;
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    // -------------------------------------------------------------------
    // 64-bit result readers
    // -------------------------------------------------------------------

    // Read 64-bit result from DE:HL:DE':HL'.
    uint64_t result64_regs() const
    {
        auto s = cpu.snapshot();
        return (uint64_t)s.de
             | ((uint64_t)s.hl  << 16)
             | ((uint64_t)s.de2 << 32)
             | ((uint64_t)s.hl2 << 48);
    }

    // Read double result from registers.
    double result_double_regs() const
    {
        uint64_t bits = result64_regs();
        double d;
        std::memcpy(&d, &bits, sizeof d);
        return d;
    }

    // Encode a double as a uint64_t bit pattern.
    static uint64_t double_bits(double d)
    {
        uint64_t b;
        std::memcpy(&b, &d, sizeof b);
        return b;
    }

    // Convenience wrappers for double tests.
    bool call_double2(uint16_t fn, double a, double b)
    {
        return call64(fn, double_bits(a), double_bits(b));
    }

    bool call_double1(uint16_t fn, double a)
    {
        return call64_1arg(fn, double_bits(a));
    }

    // C-library double ABI: regular functions currently receive double args on
    // the stack, not in the runtime-helper DE:HL:DE':HL' fast path.
    bool call_c_double1(uint16_t fn, double a)
    {
        uint16_t sp = STACK_BASE;
        sp = push64_arg(sp, double_bits(a));
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    bool call_c_double1_int(uint16_t fn, double a, int16_t b)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, (uint16_t)b);
        sp = push64_arg(sp, double_bits(a));
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    bool call_c_double2(uint16_t fn, double a, double b)
    {
        uint16_t sp = STACK_BASE;
        sp = push64_arg(sp, double_bits(b));
        sp = push64_arg(sp, double_bits(a));
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    bool call_c_double1_ptr(uint16_t fn, double a, uint16_t ptr)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, ptr);
        sp = push64_arg(sp, double_bits(a));
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    bool call_c_double1_long(uint16_t fn, double a, int32_t b)
    {
        uint16_t sp = STACK_BASE;
        sp = push32_arg(sp, (uint32_t)b);
        sp = push64_arg(sp, double_bits(a));
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    bool call_c_double3(uint16_t fn, double a, double b, double c)
    {
        uint16_t sp = STACK_BASE;
        sp = push64_arg(sp, double_bits(c));
        sp = push64_arg(sp, double_bits(b));
        sp = push64_arg(sp, double_bits(a));
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

    bool call_c_double2_ptr(uint16_t fn, double a, double b, uint16_t ptr)
    {
        uint16_t sp = STACK_BASE;
        sp = push16(sp, ptr);
        sp = push64_arg(sp, double_bits(b));
        sp = push64_arg(sp, double_bits(a));
        sp = push16(sp, HALT_ADDR);

        xz80::cpu_state s{};
        s.sp = sp;
        s.pc = fn;
        cpu.restore(s);
        return run_to_halt_already_set();
    }

private:
    // Run from already-restored cpu state until halt.
    bool run_to_halt_already_set()
    {
        for (int i = 0; i < MAX_STEPS; ++i) {
            cpu.step();
            if (cpu.halted()) return true;
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// Global runtime machine (initialized once, shared across all tests)
// ---------------------------------------------------------------------------
static runtime_machine* g_rt = nullptr;
