// test_disassembler.cpp — Z80 disassembler unit tests.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#include <xz80/disassembler.hpp>
#include <xz80/memory.hpp>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static xz80::disasm_result dasm(std::initializer_list<uint8_t> bytes)
{
    xz80::flat_memory mem;
    uint16_t addr = 0x0000;
    uint16_t pc   = addr;
    for (uint8_t b : bytes) mem.write(pc++, b);
    xz80::disassembler d;
    return d.disassemble(addr, mem);
}

// ---------------------------------------------------------------------------
// Base opcode tests
// ---------------------------------------------------------------------------

TEST(dasm_nop)
{
    auto r = dasm({ 0x00 });
    REQUIRE_EQ(r.text, std::string("NOP"));
    REQUIRE_EQ(r.length, 1);
    REQUIRE_EQ(r.t_states, 4);
}

TEST(dasm_halt)
{
    auto r = dasm({ 0x76 });
    REQUIRE_EQ(r.text, std::string("HALT"));
    REQUIRE_EQ(r.length, 1);
}

TEST(dasm_ld_bc_nn)
{
    auto r = dasm({ 0x01, 0x34, 0x12 });
    REQUIRE_EQ(r.text, std::string("LD BC,#1234"));
    REQUIRE_EQ(r.length, 3);
    REQUIRE_EQ(r.t_states, 10);
}

TEST(dasm_ld_a_n)
{
    auto r = dasm({ 0x3E, 0x42 });
    REQUIRE_EQ(r.text, std::string("LD A,#42"));
    REQUIRE_EQ(r.length, 2);
    REQUIRE_EQ(r.t_states, 7);
}

TEST(dasm_jr_e)
{
    // JR at 0x0000 with offset +5 → target = 0x0002 + 5 = 0x0007
    auto r = dasm({ 0x18, 0x05 });
    REQUIRE_EQ(r.text, std::string("JR #0007"));
    REQUIRE_EQ(r.length, 2);
}

TEST(dasm_jr_negative_offset)
{
    // JR with offset -4 (0xFC) → target = 0x0002 - 4 = 0xFFFE
    auto r = dasm({ 0x18, 0xFC });
    REQUIRE_EQ(r.text, std::string("JR #FFFE"));
    REQUIRE_EQ(r.length, 2);
}

TEST(dasm_djnz)
{
    auto r = dasm({ 0x10, 0x03 });
    REQUIRE_EQ(r.text, std::string("DJNZ #0005"));
    REQUIRE_EQ(r.t_states,  13);
    REQUIRE_EQ(r.t_states2, 8);
}

TEST(dasm_call_nn)
{
    auto r = dasm({ 0xCD, 0x00, 0x80 });
    REQUIRE_EQ(r.text, std::string("CALL #8000"));
    REQUIRE_EQ(r.length, 3);
    REQUIRE_EQ(r.t_states, 17);
}

TEST(dasm_ret)
{
    auto r = dasm({ 0xC9 });
    REQUIRE_EQ(r.text, std::string("RET"));
    REQUIRE_EQ(r.length, 1);
    REQUIRE_EQ(r.t_states, 10);
}

TEST(dasm_ret_nz)
{
    auto r = dasm({ 0xC0 });
    REQUIRE_EQ(r.text, std::string("RET NZ"));
    REQUIRE_EQ(r.t_states,  11);
    REQUIRE_EQ(r.t_states2, 5);
}

TEST(dasm_rst)
{
    auto r = dasm({ 0xFF }); // RST 38H
    REQUIRE_EQ(r.text, std::string("RST 38H"));
}

TEST(dasm_add_hl_de)
{
    auto r = dasm({ 0x19 });
    REQUIRE_EQ(r.text, std::string("ADD HL,DE"));
    REQUIRE_EQ(r.t_states, 11);
}

TEST(dasm_out_n_a)
{
    auto r = dasm({ 0xD3, 0xFE });
    REQUIRE_EQ(r.text, std::string("OUT (#FE),A"));
    REQUIRE_EQ(r.length, 2);
}

TEST(dasm_ld_indirect_hl_stored)
{
    auto r = dasm({ 0x22, 0x00, 0x40 });
    REQUIRE_EQ(r.text, std::string("LD (#4000),HL"));
    REQUIRE_EQ(r.length, 3);
    REQUIRE_EQ(r.t_states, 16);
}

// ---------------------------------------------------------------------------
// CB prefix
// ---------------------------------------------------------------------------

TEST(dasm_cb_rlc_b)
{
    auto r = dasm({ 0xCB, 0x00 });
    REQUIRE_EQ(r.text, std::string("RLC B"));
    REQUIRE_EQ(r.length, 2);
    REQUIRE_EQ(r.t_states, 8);
}

TEST(dasm_cb_rlc_hl_ind)
{
    auto r = dasm({ 0xCB, 0x06 });
    REQUIRE_EQ(r.text, std::string("RLC (HL)"));
    REQUIRE_EQ(r.t_states, 15);
}

TEST(dasm_cb_bit_3_c)
{
    auto r = dasm({ 0xCB, 0x59 }); // BIT 3,C
    REQUIRE_EQ(r.text, std::string("BIT 3,C"));
    REQUIRE_EQ(r.t_states, 8);
}

TEST(dasm_cb_set_7_a)
{
    auto r = dasm({ 0xCB, 0xFF }); // SET 7,A
    REQUIRE_EQ(r.text, std::string("SET 7,A"));
    REQUIRE_EQ(r.t_states, 8);
}

TEST(dasm_cb_res_0_hl)
{
    auto r = dasm({ 0xCB, 0x86 }); // RES 0,(HL)
    REQUIRE_EQ(r.text, std::string("RES 0,(HL)"));
    REQUIRE_EQ(r.t_states, 15);
}

TEST(dasm_cb_srl_d)
{
    auto r = dasm({ 0xCB, 0x3A }); // SRL D
    REQUIRE_EQ(r.text, std::string("SRL D"));
}

// ---------------------------------------------------------------------------
// ED prefix
// ---------------------------------------------------------------------------

TEST(dasm_ed_ldir)
{
    auto r = dasm({ 0xED, 0xB0 });
    REQUIRE_EQ(r.text, std::string("LDIR"));
    REQUIRE_EQ(r.t_states,  21);
    REQUIRE_EQ(r.t_states2, 16);
}

TEST(dasm_ed_in_b_c)
{
    auto r = dasm({ 0xED, 0x40 });
    REQUIRE_EQ(r.text, std::string("IN B,(C)"));
    REQUIRE_EQ(r.t_states, 12);
}

TEST(dasm_ed_ld_i_a)
{
    auto r = dasm({ 0xED, 0x47 });
    REQUIRE_EQ(r.text, std::string("LD I,A"));
}

TEST(dasm_ed_undefined)
{
    auto r = dasm({ 0xED, 0x00 }); // undefined → NOP*
    REQUIRE_EQ(r.text, std::string("NOP*"));
}

TEST(dasm_ed_rld)
{
    auto r = dasm({ 0xED, 0x6F });
    REQUIRE_EQ(r.text, std::string("RLD"));
    REQUIRE_EQ(r.t_states, 18);
}

// ---------------------------------------------------------------------------
// DD (IX) prefix
// ---------------------------------------------------------------------------

TEST(dasm_dd_ld_ix_nn)
{
    auto r = dasm({ 0xDD, 0x21, 0x00, 0x10 });
    REQUIRE_EQ(r.text, std::string("LD IX,#1000"));
    REQUIRE_EQ(r.length, 4);
    REQUIRE_EQ(r.t_states, 14);
}

TEST(dasm_dd_ld_b_ix_plus_d)
{
    auto r = dasm({ 0xDD, 0x46, 0x05 }); // LD B,(IX+5)
    REQUIRE_EQ(r.text, std::string("LD B,(IX+5)"));
    REQUIRE_EQ(r.length, 3);
    REQUIRE_EQ(r.t_states, 19);
}

TEST(dasm_dd_ld_ix_plus_d_a)
{
    auto r = dasm({ 0xDD, 0x77, 0xFB }); // LD (IX-5),A
    REQUIRE_EQ(r.text, std::string("LD (IX-5),A"));
    REQUIRE_EQ(r.length, 3);
}

TEST(dasm_dd_jp_ix)
{
    auto r = dasm({ 0xDD, 0xE9 });
    REQUIRE_EQ(r.text, std::string("JP (IX)"));
    REQUIRE_EQ(r.length, 2);
    REQUIRE_EQ(r.t_states, 8);
}

TEST(dasm_dd_add_ix_bc)
{
    auto r = dasm({ 0xDD, 0x09 });
    REQUIRE_EQ(r.text, std::string("ADD IX,BC"));
    REQUIRE_EQ(r.t_states, 15);
}

TEST(dasm_dd_mirror_base)
{
    // DD 0x01 = undefined DD opcode → mirrors LD BC,nn
    auto r = dasm({ 0xDD, 0x01, 0xFF, 0x00 });
    REQUIRE_EQ(r.text, std::string("LD BC,#00FF"));
}

// ---------------------------------------------------------------------------
// FD (IY) prefix
// ---------------------------------------------------------------------------

TEST(dasm_fd_ld_iy_nn)
{
    auto r = dasm({ 0xFD, 0x21, 0x00, 0x20 });
    REQUIRE_EQ(r.text, std::string("LD IY,#2000"));
    REQUIRE_EQ(r.length, 4);
}

TEST(dasm_fd_jp_iy)
{
    auto r = dasm({ 0xFD, 0xE9 });
    REQUIRE_EQ(r.text, std::string("JP (IY)"));
}

// ---------------------------------------------------------------------------
// DDCB / FDCB prefix
// ---------------------------------------------------------------------------

TEST(dasm_ddcb_rlc_ix_d)
{
    auto r = dasm({ 0xDD, 0xCB, 0x04, 0x06 }); // RLC (IX+4)
    REQUIRE_EQ(r.text, std::string("RLC (IX+4)"));
    REQUIRE_EQ(r.length, 4);
    REQUIRE_EQ(r.t_states, 23);
}

TEST(dasm_ddcb_bit_2_ix_neg)
{
    auto r = dasm({ 0xDD, 0xCB, 0xFC, 0x56 }); // BIT 2,(IX-4)
    REQUIRE_EQ(r.text, std::string("BIT 2,(IX-4)"));
    REQUIRE_EQ(r.length, 4);
}

TEST(dasm_ddcb_set_7_ix)
{
    auto r = dasm({ 0xDD, 0xCB, 0x00, 0xFE }); // SET 7,(IX+0)
    REQUIRE_EQ(r.text, std::string("SET 7,(IX+0)"));
}

TEST(dasm_fdcb_res_5_iy)
{
    auto r = dasm({ 0xFD, 0xCB, 0x02, 0xAE }); // RES 5,(IY+2)
    REQUIRE_EQ(r.text, std::string("RES 5,(IY+2)"));
    REQUIRE_EQ(r.length, 4);
}

// ---------------------------------------------------------------------------
// length field correctness
// ---------------------------------------------------------------------------

TEST(dasm_length_1byte)   { REQUIRE_EQ(dasm({0x00}).length, 1); }
TEST(dasm_length_2byte)   { REQUIRE_EQ(dasm({0x3E,0x00}).length, 2); }
TEST(dasm_length_3byte)   { REQUIRE_EQ(dasm({0x01,0x00,0x00}).length, 3); }
TEST(dasm_length_4byte)   { REQUIRE_EQ(dasm({0xDD,0x21,0x00,0x00}).length, 4); }
