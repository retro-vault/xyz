// test_cpu.cpp — Z80 CPU emulator unit tests.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#include <xz80/cpu.hpp>
#include <xz80/memory.hpp>
#include <xz80/ports.hpp>
#include <xz80/cpu_state.hpp>

// ---------------------------------------------------------------------------
// Helper: small machine with flat RAM and null ports
// ---------------------------------------------------------------------------

struct machine {
    xz80::flat_memory mem;
    xz80::null_ports  ports;
    xz80::cpu         cpu;

    machine() : cpu(mem, ports) { cpu.reset(); }

    // Load bytes at address 0 and reset PC to 0.
    void load(std::initializer_list<uint8_t> bytes)
    {
        uint16_t pc = 0;
        for (uint8_t b : bytes) mem.write(pc++, b);
        cpu.set_reg(xz80::reg16::PC, 0x0000);
    }

    int step() { return cpu.step(); }
};

// ---------------------------------------------------------------------------
// Basic execution tests
// ---------------------------------------------------------------------------

TEST(cpu_nop_advances_pc)
{
    machine m;
    m.load({ 0x00 }); // NOP
    int t = m.step();
    REQUIRE_EQ(t, 4);
    REQUIRE_EQ(m.cpu.pc(), 1);
}

TEST(cpu_ld_bc_nn)
{
    machine m;
    m.load({ 0x01, 0x34, 0x12 }); // LD BC, #1234
    m.step();
    REQUIRE_EQ(m.cpu.reg(xz80::reg16::BC), 0x1234);
    REQUIRE_EQ(m.cpu.pc(), 3);
}

TEST(cpu_ld_a_n)
{
    machine m;
    m.load({ 0x3E, 0x42 }); // LD A, #42
    m.step();
    auto s = m.cpu.snapshot();
    REQUIRE_EQ(s.af >> 8, 0x42u);
}

TEST(cpu_inc_bc)
{
    machine m;
    m.load({ 0x01, 0xFF, 0x00, // LD BC, #00FF
             0x03 });           // INC BC
    m.step(); m.step();
    REQUIRE_EQ(m.cpu.reg(xz80::reg16::BC), 0x0100);
}

TEST(cpu_add_hl_de)
{
    machine m;
    m.load({ 0x21, 0x00, 0x10, // LD HL, #1000
             0x11, 0x00, 0x01, // LD DE, #0100
             0x19 });           // ADD HL,DE
    m.step(); m.step(); m.step();
    REQUIRE_EQ(m.cpu.reg(xz80::reg16::HL), 0x1100);
}

TEST(cpu_jr_forward)
{
    machine m;
    // JR +2 → skip next two bytes
    m.load({ 0x18, 0x02,  // JR +2 (target = PC+2+2 = 0x0004)
             0x00, 0x00,  // NOP NOP (skipped)
             0x3C });     // INC A  (executed)
    m.step(); // JR
    REQUIRE_EQ(m.cpu.pc(), 0x0004);
    m.step(); // INC A
    auto s = m.cpu.snapshot();
    REQUIRE_EQ(s.af >> 8, 0x01u);
}

TEST(cpu_halt_sets_halted_flag)
{
    machine m;
    m.load({ 0x76 }); // HALT
    m.step();
    REQUIRE(m.cpu.halted());
}

// ---------------------------------------------------------------------------
// Register snapshot / restore
// ---------------------------------------------------------------------------

TEST(cpu_snapshot_restore_round_trip)
{
    machine m;
    m.load({ 0x01, 0x34, 0x12,  // LD BC, #1234
             0x11, 0x78, 0x56,  // LD DE, #5678
             0x21, 0xBC, 0x9A,  // LD HL, #9ABC
             0x3E, 0xFF });      // LD A, #FF
    m.step(); m.step(); m.step(); m.step();

    xz80::cpu_state snap = m.cpu.snapshot();
    REQUIRE_EQ(snap.bc, 0x1234u);
    REQUIRE_EQ(snap.de, 0x5678u);
    REQUIRE_EQ(snap.hl, 0x9ABCu);
    REQUIRE_EQ(snap.af >> 8, 0xFFu);

    // Modify BC and restore
    snap.bc = 0xBEEF;
    m.cpu.restore(snap);
    REQUIRE_EQ(m.cpu.reg(xz80::reg16::BC), 0xBEEFu);
}

TEST(cpu_set_reg_pc)
{
    machine m;
    m.load({ 0x00 }); // NOP at 0
    m.mem.write(0x1000, 0x3C); // INC A at 0x1000
    m.cpu.set_reg(xz80::reg16::PC, 0x1000);
    m.step();
    auto s = m.cpu.snapshot();
    REQUIRE_EQ(s.af >> 8, 0x01u);
    REQUIRE_EQ(m.cpu.pc(), 0x1001u);
}

// ---------------------------------------------------------------------------
// Memory write-back test
// ---------------------------------------------------------------------------

TEST(cpu_ld_indirect_hl_writes_memory)
{
    machine m;
    m.load({ 0x21, 0x00, 0x80,  // LD HL, #8000
             0x36, 0xAB });      // LD (HL), #AB
    m.step(); m.step();
    REQUIRE_EQ(m.mem.read(0x8000), 0xABu);
}

// ---------------------------------------------------------------------------
// T-state count tests
// ---------------------------------------------------------------------------

TEST(cpu_t_states_nop)       { machine m; m.load({0x00});          REQUIRE_EQ(m.step(), 4);  }
TEST(cpu_t_states_ld_bc_nn)  { machine m; m.load({0x01,0,0});      REQUIRE_EQ(m.step(), 10); }
TEST(cpu_t_states_add_hl_bc) { machine m; m.load({0x09});          REQUIRE_EQ(m.step(), 11); }
