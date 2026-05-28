// disassembler.cpp — table-driven Z80 disassembler.
//
// Opcode tables cover the full documented Z80 instruction set plus the
// most common undocumented instructions (IXH/IXL, SLL, DDCB/FDCB register
// variants).  Truly undefined slots are displayed as "NOP*".
//
// Format-string conventions (single-character substitution markers):
//   @   read two bytes (LE) → display as #NNNN
//   ^   read one byte       → display as #NN
//   %   read signed byte    → display as absolute address #NNNN
//   $   use/read signed displacement → display as +N or -N
//       (for DDCB/FDCB the displacement is read before the opcode byte
//        and re-used when the formatter encounters $)
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include <xz80/disassembler.hpp>
#include <xz80/memory.hpp>

namespace xz80 {

namespace {

// ---------------------------------------------------------------------------
// Table entry
// ---------------------------------------------------------------------------

struct opc {
    const char* fmt; // nullptr = undefined (print "NOP*") or mirror base (DD/FD)
    uint8_t     t;   // nominal T-states
    uint8_t     t2;  // alternate T-states (taken branch); 0 = same as t
};

#define O(f,t)     opc{f, t, 0}
#define OC(f,t,t2) opc{f, t, t2}
#define NU         opc{nullptr,  8, 0} // undefined opcode → "NOP*"
#define NX         opc{nullptr,  0, 0} // DD/FD mirror: fall back to base table

// ---------------------------------------------------------------------------
// Base opcode table  (0x00 – 0xFF)
// ---------------------------------------------------------------------------

static constexpr opc base_tbl[256] = {
    // 00-0F
    O("NOP",4),         O("LD BC,@",10),    O("LD (BC),A",7),   O("INC BC",6),
    O("INC B",4),       O("DEC B",4),       O("LD B,^",7),      O("RLCA",4),
    O("EX AF,AF'",4),   O("ADD HL,BC",11),  O("LD A,(BC)",7),   O("DEC BC",6),
    O("INC C",4),       O("DEC C",4),       O("LD C,^",7),      O("RRCA",4),
    // 10-1F
    OC("DJNZ %",13,8),  O("LD DE,@",10),    O("LD (DE),A",7),   O("INC DE",6),
    O("INC D",4),       O("DEC D",4),       O("LD D,^",7),      O("RLA",4),
    O("JR %",12),       O("ADD HL,DE",11),  O("LD A,(DE)",7),   O("DEC DE",6),
    O("INC E",4),       O("DEC E",4),       O("LD E,^",7),      O("RRA",4),
    // 20-2F
    OC("JR NZ,%",12,7), O("LD HL,@",10),    O("LD (@),HL",16),  O("INC HL",6),
    O("INC H",4),       O("DEC H",4),       O("LD H,^",7),      O("DAA",4),
    OC("JR Z,%",12,7),  O("ADD HL,HL",11),  O("LD HL,(@)",16),  O("DEC HL",6),
    O("INC L",4),       O("DEC L",4),       O("LD L,^",7),      O("CPL",4),
    // 30-3F
    OC("JR NC,%",12,7), O("LD SP,@",10),    O("LD (@),A",13),   O("INC SP",6),
    O("INC (HL)",11),   O("DEC (HL)",11),   O("LD (HL),^",10),  O("SCF",4),
    OC("JR C,%",12,7),  O("ADD HL,SP",11),  O("LD A,(@)",13),   O("DEC SP",6),
    O("INC A",4),       O("DEC A",4),       O("LD A,^",7),      O("CCF",4),
    // 40-4F  LD B,r  /  LD C,r
    O("LD B,B",4),      O("LD B,C",4),      O("LD B,D",4),      O("LD B,E",4),
    O("LD B,H",4),      O("LD B,L",4),      O("LD B,(HL)",7),   O("LD B,A",4),
    O("LD C,B",4),      O("LD C,C",4),      O("LD C,D",4),      O("LD C,E",4),
    O("LD C,H",4),      O("LD C,L",4),      O("LD C,(HL)",7),   O("LD C,A",4),
    // 50-5F  LD D,r  /  LD E,r
    O("LD D,B",4),      O("LD D,C",4),      O("LD D,D",4),      O("LD D,E",4),
    O("LD D,H",4),      O("LD D,L",4),      O("LD D,(HL)",7),   O("LD D,A",4),
    O("LD E,B",4),      O("LD E,C",4),      O("LD E,D",4),      O("LD E,E",4),
    O("LD E,H",4),      O("LD E,L",4),      O("LD E,(HL)",7),   O("LD E,A",4),
    // 60-6F  LD H,r  /  LD L,r
    O("LD H,B",4),      O("LD H,C",4),      O("LD H,D",4),      O("LD H,E",4),
    O("LD H,H",4),      O("LD H,L",4),      O("LD H,(HL)",7),   O("LD H,A",4),
    O("LD L,B",4),      O("LD L,C",4),      O("LD L,D",4),      O("LD L,E",4),
    O("LD L,H",4),      O("LD L,L",4),      O("LD L,(HL)",7),   O("LD L,A",4),
    // 70-7F  LD (HL),r  /  HALT  /  LD A,r
    O("LD (HL),B",7),   O("LD (HL),C",7),   O("LD (HL),D",7),   O("LD (HL),E",7),
    O("LD (HL),H",7),   O("LD (HL),L",7),   O("HALT",4),        O("LD (HL),A",7),
    O("LD A,B",4),      O("LD A,C",4),      O("LD A,D",4),      O("LD A,E",4),
    O("LD A,H",4),      O("LD A,L",4),      O("LD A,(HL)",7),   O("LD A,A",4),
    // 80-8F  ADD A,r  /  ADC A,r
    O("ADD A,B",4),     O("ADD A,C",4),     O("ADD A,D",4),     O("ADD A,E",4),
    O("ADD A,H",4),     O("ADD A,L",4),     O("ADD A,(HL)",7),  O("ADD A,A",4),
    O("ADC A,B",4),     O("ADC A,C",4),     O("ADC A,D",4),     O("ADC A,E",4),
    O("ADC A,H",4),     O("ADC A,L",4),     O("ADC A,(HL)",7),  O("ADC A,A",4),
    // 90-9F  SUB r  /  SBC A,r
    O("SUB B",4),       O("SUB C",4),       O("SUB D",4),       O("SUB E",4),
    O("SUB H",4),       O("SUB L",4),       O("SUB (HL)",7),    O("SUB A",4),
    O("SBC A,B",4),     O("SBC A,C",4),     O("SBC A,D",4),     O("SBC A,E",4),
    O("SBC A,H",4),     O("SBC A,L",4),     O("SBC A,(HL)",7),  O("SBC A,A",4),
    // A0-AF  AND r  /  XOR r
    O("AND B",4),       O("AND C",4),       O("AND D",4),       O("AND E",4),
    O("AND H",4),       O("AND L",4),       O("AND (HL)",7),    O("AND A",4),
    O("XOR B",4),       O("XOR C",4),       O("XOR D",4),       O("XOR E",4),
    O("XOR H",4),       O("XOR L",4),       O("XOR (HL)",7),    O("XOR A",4),
    // B0-BF  OR r  /  CP r
    O("OR B",4),        O("OR C",4),        O("OR D",4),        O("OR E",4),
    O("OR H",4),        O("OR L",4),        O("OR (HL)",7),     O("OR A",4),
    O("CP B",4),        O("CP C",4),        O("CP D",4),        O("CP E",4),
    O("CP H",4),        O("CP L",4),        O("CP (HL)",7),     O("CP A",4),
    // C0-CF
    OC("RET NZ",11,5),  O("POP BC",10),     O("JP NZ,@",10),    O("JP @",10),
    OC("CALL NZ,@",17,10), O("PUSH BC",11), O("ADD A,^",7),     O("RST 00H",11),
    OC("RET Z",11,5),   O("RET",10),        O("JP Z,@",10),     NX/*CB*/,
    OC("CALL Z,@",17,10),  O("CALL @",17),  O("ADC A,^",7),     O("RST 08H",11),
    // D0-DF
    OC("RET NC",11,5),  O("POP DE",10),     O("JP NC,@",10),    O("OUT (^),A",11),
    OC("CALL NC,@",17,10), O("PUSH DE",11), O("SUB ^",7),       O("RST 10H",11),
    OC("RET C",11,5),   O("EXX",4),         O("JP C,@",10),     O("IN A,(^)",11),
    OC("CALL C,@",17,10),  NX/*DD*/,        O("SBC A,^",7),     O("RST 18H",11),
    // E0-EF
    OC("RET PO",11,5),  O("POP HL",10),     O("JP PO,@",10),    O("EX (SP),HL",19),
    OC("CALL PO,@",17,10), O("PUSH HL",11), O("AND ^",7),       O("RST 20H",11),
    OC("RET PE",11,5),  O("JP (HL)",4),     O("JP PE,@",10),    O("EX DE,HL",4),
    OC("CALL PE,@",17,10), NX/*ED*/,        O("XOR ^",7),       O("RST 28H",11),
    // F0-FF
    OC("RET P",11,5),   O("POP AF",10),     O("JP P,@",10),     O("DI",4),
    OC("CALL P,@",17,10),  O("PUSH AF",11), O("OR ^",7),        O("RST 30H",11),
    OC("RET M",11,5),   O("LD SP,HL",6),    O("JP M,@",10),     O("EI",4),
    OC("CALL M,@",17,10),  NX/*FD*/,        O("CP ^",7),        O("RST 38H",11),
};

// ---------------------------------------------------------------------------
// CB prefix table  (built once, lazily, from a regular pattern)
// ---------------------------------------------------------------------------

static std::array<opc, 256> build_cb()
{
    static const char* rot_name[8] = {
        "RLC","RRC","RL","RR","SLA","SRA","SLL","SRL"
    };
    static const char* r8[8] = {
        "B","C","D","E","H","L","(HL)","A"
    };
    static char bufs[256][18];
    std::array<opc, 256> t{};
    for (int i = 0; i < 64; ++i) {
        uint8_t ts = (i & 7) == 6 ? 15u : 8u;
        std::snprintf(bufs[i], sizeof bufs[i], "%s %s",
            rot_name[i >> 3], r8[i & 7]);
        t[i] = { bufs[i], ts, 0 };
    }
    for (int i = 64; i < 128; ++i) {
        uint8_t ts = (i & 7) == 6 ? 12u : 8u;
        std::snprintf(bufs[i], sizeof bufs[i], "BIT %d,%s",
            (i >> 3) & 7, r8[i & 7]);
        t[i] = { bufs[i], ts, 0 };
    }
    for (int i = 128; i < 192; ++i) {
        uint8_t ts = (i & 7) == 6 ? 15u : 8u;
        std::snprintf(bufs[i], sizeof bufs[i], "RES %d,%s",
            (i >> 3) & 7, r8[i & 7]);
        t[i] = { bufs[i], ts, 0 };
    }
    for (int i = 192; i < 256; ++i) {
        uint8_t ts = (i & 7) == 6 ? 15u : 8u;
        std::snprintf(bufs[i], sizeof bufs[i], "SET %d,%s",
            (i >> 3) & 7, r8[i & 7]);
        t[i] = { bufs[i], ts, 0 };
    }
    return t;
}

static const std::array<opc, 256>& cb_tbl()
{
    static const auto tbl = build_cb();
    return tbl;
}

// ---------------------------------------------------------------------------
// ED prefix table  (0xED 0x00 – 0xFF)
// ---------------------------------------------------------------------------

static constexpr opc ed_tbl[256] = {
    // 00-3F: undefined
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // 00-0F
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // 10-1F
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // 20-2F
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // 30-3F
    // 40-4F
    O("IN B,(C)",12),   O("OUT (C),B",12),  O("SBC HL,BC",15),  O("LD (@),BC",20),
    O("NEG",8),         O("RETN",14),       O("IM 0",8),        O("LD I,A",9),
    O("IN C,(C)",12),   O("OUT (C),C",12),  O("ADC HL,BC",15),  O("LD BC,(@)",20),
    O("NEG*",8),        O("RETI",14),       O("IM 0*",8),       O("LD R,A",9),
    // 50-5F
    O("IN D,(C)",12),   O("OUT (C),D",12),  O("SBC HL,DE",15),  O("LD (@),DE",20),
    O("NEG*",8),        O("RETN*",14),      O("IM 1",8),        O("LD A,I",9),
    O("IN E,(C)",12),   O("OUT (C),E",12),  O("ADC HL,DE",15),  O("LD DE,(@)",20),
    O("NEG*",8),        O("RETI*",14),      O("IM 2",8),        O("LD A,R",9),
    // 60-6F
    O("IN H,(C)",12),   O("OUT (C),H",12),  O("SBC HL,HL",15),  O("LD (@),HL*",20),
    O("NEG*",8),        O("RETN*",14),      O("IM 0*",8),       O("RRD",18),
    O("IN L,(C)",12),   O("OUT (C),L",12),  O("ADC HL,HL",15),  O("LD HL,(@)*",20),
    O("NEG*",8),        O("RETI*",14),      O("IM 0*",8),       O("RLD",18),
    // 70-7F
    O("IN F,(C)*",12),  O("OUT (C),0*",12), O("SBC HL,SP",15),  O("LD (@),SP",20),
    O("NEG*",8),        O("RETN*",14),      O("IM 1*",8),       NU,
    O("IN A,(C)",12),   O("OUT (C),A",12),  O("ADC HL,SP",15),  O("LD SP,(@)",20),
    O("NEG*",8),        O("RETN*",14),      O("IM 2*",8),       NU,
    // 80-9F: undefined
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // 80-8F
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // 90-9F
    // A0-AF
    O("LDI",16),  O("CPI",16),  O("INI",16),  O("OUTI",16),
    NU,NU,NU,NU,
    O("LDD",16),  O("CPD",16),  O("IND",16),  O("OUTD",16),
    NU,NU,NU,NU,
    // B0-BF
    OC("LDIR",21,16), OC("CPIR",21,16), OC("INIR",21,16), OC("OTIR",21,16),
    NU,NU,NU,NU,
    OC("LDDR",21,16), OC("CPDR",21,16), OC("INDR",21,16), OC("OTDR",21,16),
    NU,NU,NU,NU,
    // C0-FF: undefined
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // C0-CF
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // D0-DF
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // E0-EF
    NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU, NU,NU,NU,NU,  // F0-FF
};

// ---------------------------------------------------------------------------
// DD / FD prefix tables — built lazily.
// Entries with fmt==nullptr fall back to the base table.
// The ix parameter selects IX (true) vs IY (false).
// ---------------------------------------------------------------------------

static std::array<opc, 256> build_ixiy(bool ix)
{
    const std::string R  = ix ? "IX"  : "IY";
    const std::string RH = ix ? "IXH" : "IYH";
    const std::string RL = ix ? "IXL" : "IYL";
    const std::string Pi = "(" + R + "$)"; // (IX$) or (IY$)

    static char bufs_ix[256][24];
    static char bufs_iy[256][24];
    char (*bufs)[24] = ix ? bufs_ix : bufs_iy;

    std::array<opc, 256> t{};

    auto set = [&](int i, std::string s, uint8_t ts, uint8_t ts2 = 0) {
        std::snprintf(bufs[i], 24, "%s", s.c_str());
        t[i] = { bufs[i], ts, ts2 };
    };

    // ADD IX/IY, rr
    set(0x09, "ADD "+R+",BC",  15); set(0x19, "ADD "+R+",DE",  15);
    set(0x29, "ADD "+R+","+R,  15); set(0x39, "ADD "+R+",SP",  15);

    // LD IX/IY, nn / (nn) / INC / DEC
    set(0x21, "LD "+R+",@",    14); set(0x22, "LD (@),"+R,     20);
    set(0x23, "INC "+R,        10); set(0x2A, "LD "+R+",(@)",  20);
    set(0x2B, "DEC "+R,        10);

    // Undocumented high/low halves
    set(0x24, "INC "+RH,        8); set(0x25, "DEC "+RH,        8);
    set(0x26, "LD "+RH+",^",   11); set(0x2C, "INC "+RL,        8);
    set(0x2D, "DEC "+RL,        8); set(0x2E, "LD "+RL+",^",   11);

    // INC/DEC/LD (IX+d)
    set(0x34, "INC "+Pi,       23); set(0x35, "DEC "+Pi,       23);
    set(0x36, "LD "+Pi+",^",   19);

    // LD B/C/D/E, IXH/IXL/(IX+d)
    static const char* const lr[] = { "B","C","D","E" };
    static const int         lb[] = { 0x44, 0x4C, 0x54, 0x5C };
    for (int i = 0; i < 4; ++i) {
        set(lb[i]+0, "LD "+std::string(lr[i])+","+RH, 8);
        set(lb[i]+1, "LD "+std::string(lr[i])+","+RL, 8);
        set(lb[i]+2, "LD "+std::string(lr[i])+","+Pi, 19);
    }

    // LD IXH/IXL, r
    for (int i = 0; i < 4; ++i) {
        set(0x60+i, "LD "+RH+","+lr[i], 8);
        set(0x68+i, "LD "+RL+","+lr[i], 8);
    }
    set(0x64, "LD "+RH+","+RH, 8); set(0x65, "LD "+RH+","+RL, 8);
    set(0x66, "LD H,"+Pi,     19); set(0x67, "LD "+RH+",A",   8);
    set(0x6C, "LD "+RL+","+RH, 8); set(0x6D, "LD "+RL+","+RL, 8);
    set(0x6E, "LD L,"+Pi,     19); set(0x6F, "LD "+RL+",A",   8);

    // LD (IX+d), r
    for (int i = 0; i < 4; ++i) set(0x70+i, "LD "+Pi+","+lr[i], 19);
    set(0x74, "LD "+Pi+",H*", 19); set(0x75, "LD "+Pi+",L*", 19);
    // 0x76 = HALT, stays as null (mirrors base)
    set(0x77, "LD "+Pi+",A",  19);

    set(0x7C, "LD A,"+RH,     8); set(0x7D, "LD A,"+RL,      8);
    set(0x7E, "LD A,"+Pi,     19);

    // Undocumented ALU: IXH / IXL / (IX+d)
    static const char* const ap[] = {
        "ADD A,","ADC A,","SUB ","SBC A,","AND ","XOR ","OR ","CP "
    };
    static const int ab[] = { 0x80,0x88,0x90,0x98,0xA0,0xA8,0xB0,0xB8 };
    for (int i = 0; i < 8; ++i) {
        set(ab[i]+4, std::string(ap[i])+RH, 8);
        set(ab[i]+5, std::string(ap[i])+RL, 8);
        set(ab[i]+6, std::string(ap[i])+Pi, 19);
    }

    // Stack and JP
    set(0xE1, "POP "+R,        14); set(0xE3, "EX (SP),"+R,   23);
    set(0xE5, "PUSH "+R,       15); set(0xE9, "JP ("+R+")",    8);
    set(0xF9, "LD SP,"+R,      10);

    return t;
}

static const std::array<opc, 256>& dd_tbl()
{
    static const auto tbl = build_ixiy(true);
    return tbl;
}

static const std::array<opc, 256>& fd_tbl()
{
    static const auto tbl = build_ixiy(false);
    return tbl;
}

// ---------------------------------------------------------------------------
// DDCB / FDCB prefix tables  (displacement already read before opcode byte)
// ---------------------------------------------------------------------------

static std::array<opc, 256> build_xcb(bool ix)
{
    const char* const R = ix ? "IX" : "IY";
    static char bufs_ix[256][24];
    static char bufs_iy[256][24];
    char (*bufs)[24] = ix ? bufs_ix : bufs_iy;

    static const char* rot_name[8] = {
        "RLC","RRC","RL","RR","SLA","SRA","SLL","SRL"
    };
    static const char* r8[8] = { "B","C","D","E","H","L","","A" };

    std::array<opc, 256> t{};
    for (int i = 0; i < 64; ++i) {
        int reg = i & 7;
        if (reg == 6) {
            // Documented: operate on (IX+d)
            std::snprintf(bufs[i], sizeof bufs[i],
                "%s (%s$)", rot_name[i >> 3], R);
        } else {
            // Undocumented: operate on (IX+d) AND copy result to r
            std::snprintf(bufs[i], sizeof bufs[i],
                "%s (%s$),%s*", rot_name[i >> 3], R, r8[reg]);
        }
        t[i] = { bufs[i], 23, 0 };
    }
    for (int i = 64; i < 128; ++i) {
        int b = (i >> 3) & 7;
        std::snprintf(bufs[i], sizeof bufs[i], "BIT %d,(%s$)", b, R);
        t[i] = { bufs[i], 20, 0 };
    }
    for (int i = 128; i < 192; ++i) {
        int b   = (i >> 3) & 7;
        int reg = i & 7;
        if (reg == 6)
            std::snprintf(bufs[i], sizeof bufs[i], "RES %d,(%s$)", b, R);
        else
            std::snprintf(bufs[i], sizeof bufs[i], "RES %d,(%s$),%s*", b, R, r8[reg]);
        t[i] = { bufs[i], 23, 0 };
    }
    for (int i = 192; i < 256; ++i) {
        int b   = (i >> 3) & 7;
        int reg = i & 7;
        if (reg == 6)
            std::snprintf(bufs[i], sizeof bufs[i], "SET %d,(%s$)", b, R);
        else
            std::snprintf(bufs[i], sizeof bufs[i], "SET %d,(%s$),%s*", b, R, r8[reg]);
        t[i] = { bufs[i], 23, 0 };
    }
    return t;
}

static const std::array<opc, 256>& ddcb_tbl()
{
    static const auto tbl = build_xcb(true);
    return tbl;
}

static const std::array<opc, 256>& fdcb_tbl()
{
    static const auto tbl = build_xcb(false);
    return tbl;
}

// ---------------------------------------------------------------------------
// Format a single mnemonic using the substitution markers
// ---------------------------------------------------------------------------

static std::string format_mnemonic(
    const char*  fmt,
    uint16_t&    pc,          // advanced as bytes are read
    const IMemory& mem,
    bool         have_disp,   // true if displacement was pre-read (DDCB/FDCB)
    int8_t       disp,        // pre-read displacement (if have_disp)
    std::array<uint8_t,4>& raw_bytes,
    uint8_t&     raw_len)
{
    auto read_byte = [&]() -> uint8_t {
        uint8_t b = mem.read(pc++);
        if (raw_len < 4) raw_bytes[raw_len++] = b;
        return b;
    };

    std::string out;
    out.reserve(24);
    char tmp[12];

    for (const char* p = fmt; *p; ++p) {
        switch (*p) {
            case '@': { // 16-bit little-endian immediate
                uint8_t lo = read_byte();
                uint8_t hi = read_byte();
                uint16_t v = static_cast<uint16_t>(lo | (hi << 8));
                std::snprintf(tmp, sizeof tmp, "#%04X", v);
                out += tmp;
                break;
            }
            case '^': { // 8-bit immediate
                uint8_t v = read_byte();
                std::snprintf(tmp, sizeof tmp, "#%02X", v);
                out += tmp;
                break;
            }
            case '%': { // signed relative offset → absolute address
                int8_t off;
                if (have_disp) {
                    off = disp;
                } else {
                    off = static_cast<int8_t>(read_byte());
                    have_disp = true; disp = off;
                }
                // pc now points past the displacement byte
                uint16_t target = static_cast<uint16_t>(pc + off);
                std::snprintf(tmp, sizeof tmp, "#%04X", target);
                out += tmp;
                break;
            }
            case '$': { // signed displacement (+N or -N)
                int8_t d;
                if (have_disp) {
                    d = disp;
                } else {
                    d = static_cast<int8_t>(read_byte());
                    have_disp = true; disp = d;
                }
                if (d >= 0)
                    std::snprintf(tmp, sizeof tmp, "+%d", static_cast<int>(d));
                else
                    std::snprintf(tmp, sizeof tmp, "%d", static_cast<int>(d));
                out += tmp;
                break;
            }
            default:
                out += *p;
                break;
        }
    }
    return out;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// disassembler::disassemble
// ---------------------------------------------------------------------------

disasm_result disassembler::disassemble(uint16_t addr, const IMemory& mem) const
{
    disasm_result result{};
    result.address = addr;

    uint16_t pc = addr;
    bool     have_disp = false;
    int8_t   disp      = 0;

    auto read_byte = [&]() -> uint8_t {
        uint8_t b = mem.read(pc++);
        if (result.length < 4) result.bytes[result.length++] = b;
        return b;
    };

    uint8_t opcode = read_byte();

    const opc* entry = nullptr;
    bool       undefined = false;

    switch (opcode) {
        case 0xCB: {
            uint8_t sub = read_byte();
            entry = &cb_tbl()[sub];
            break;
        }
        case 0xED: {
            uint8_t sub = read_byte();
            entry = &ed_tbl[sub];
            if (!entry->fmt) { undefined = true; entry = nullptr; }
            break;
        }
        case 0xDD:
        case 0xFD: {
            bool is_ix = (opcode == 0xDD);
            uint8_t sub = read_byte();

            if (sub == 0xCB) {
                // DDCB / FDCB: displacement before opcode
                disp = static_cast<int8_t>(read_byte());
                have_disp = true;
                uint8_t op2 = read_byte();
                entry = is_ix ? &ddcb_tbl()[op2] : &fdcb_tbl()[op2];
            } else {
                const opc* ix_e = is_ix ? &dd_tbl()[sub] : &fd_tbl()[sub];
                if (ix_e->fmt) {
                    entry = ix_e;
                } else {
                    // Mirror base table
                    entry = &base_tbl[sub];
                }
            }
            break;
        }
        default:
            entry = &base_tbl[opcode];
            if (!entry->fmt) undefined = true;
            break;
    }

    if (undefined || !entry || !entry->fmt) {
        result.text      = "NOP*";
        result.t_states  = 4;
        result.t_states2 = 0;
        return result;
    }

    result.text = format_mnemonic(
        entry->fmt, pc, mem,
        have_disp, disp,
        result.bytes, result.length);

    result.t_states  = entry->t;
    result.t_states2 = entry->t2;
    return result;
}

} // namespace xz80
