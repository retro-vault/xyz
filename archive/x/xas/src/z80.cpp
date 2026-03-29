//
// Implements structures for describing instruction opcodes, flag effects,
// and functions for building an instruction decoder.
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#include <cctype>
#include <algorithm>

#include "z80.h"

namespace xas
{
    static const std::vector<reg_info> register_info_map = {
        // 8‑bit registers (idx as per Z80 opcode DDD/SSS fields)
        {"B", 0, token_type::REGISTER8_B},
        {"C", 1, token_type::REGISTER8_C},
        {"D", 2, token_type::REGISTER8_D},
        {"E", 3, token_type::REGISTER8_E},
        {"H", 4, token_type::REGISTER8_H},
        {"L", 5, token_type::REGISTER8_L},
        {"A", 7, token_type::REGISTER8_A},

        // 16‑bit register pairs (for PUSH/POP, etc.).
        {"BC", 0, token_type::REGISTER16_BC},
        {"DE", 1, token_type::REGISTER16_DE},
        {"HL", 2, token_type::REGISTER16_HL},
        {"SP", 3, token_type::REGISTER16_SP},
        {"AF", 4, token_type::REGISTER16_AF},
        {"IX", 5, token_type::REGISTER16_IX},
        {"IY", 6, token_type::REGISTER16_IY}};

    static const std::vector<condition_info> condition_info_map = {
        {"C", 0, token_type::CONDITION_C},
        {"M", 1, token_type::CONDITION_M},
        {"NC", 2, token_type::CONDITION_NC},
        {"NZ", 3, token_type::CONDITION_NZ},
        {"P", 4, token_type::CONDITION_P},
        {"PE", 5, token_type::CONDITION_PE},
        {"PO", 6, token_type::CONDITION_PO},
        {"Z", 7, token_type::CONDITION_Z}};

    static const std::unordered_map<std::string, instruction_info> instruction_table = {
        // ADC instructions
        {"ADC A,(HL)", {{0x8E}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_ADC, 7, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"ADC A,(IX+n)", {{0xDD, 0x8E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_ADC, 19, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"ADC A,(IY+n)", {{0xFD, 0x8E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_ADC, 19, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"ADC A,r", {{0x88}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_ADC, 4, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"ADC A,N", {{0xCE, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_ADC, 7, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"ADC HL,BC", {{0xED, 0x4A}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADC, 15, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"ADC HL,DE", {{0xED, 0x5A}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADC, 15, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"ADC HL,HL", {{0xED, 0x6A}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADC, 15, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"ADC HL,SP", {{0xED, 0x7A}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADC, 15, {false, false, true, true, false, false, false, true, false, false, true}}},

        // ADD instructions
        {"ADD A,(HL)", {{0x86}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 7, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"ADD A,(IX+n)", {{0xDD, 0x86, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_ADD, 19, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"ADD A,(IY+n)", {{0xFD, 0x86, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_ADD, 19, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"ADD A,r", {{0x80}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 4, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"ADD A,N", {{0xC6, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_ADD, 7, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"ADD HL,BC", {{0x09}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 11, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD HL,DE", {{0x19}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 11, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD HL,HL", {{0x29}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 11, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD HL,SP", {{0x39}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 11, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD IX,BC", {{0xDD, 0x09}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 15, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD IX,DE", {{0xDD, 0x19}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 15, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD IX,IX", {{0xDD, 0x29}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 15, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD IX,SP", {{0xDD, 0x39}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 15, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD IY,BC", {{0xFD, 0x09}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 15, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD IY,DE", {{0xFD, 0x19}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 15, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD IY,IY", {{0xFD, 0x29}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 15, {false, false, false, false, true, false, false, true, false, false, true}}},
        {"ADD IY,SP", {{0xFD, 0x39}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_ADD, 15, {false, false, false, false, true, false, false, true, false, false, true}}},

        // AND instructions
        {"AND (HL)", {{0xA6}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_AND, 7, {false, false, true, false, true, true, false, false, false, false, true}}},
        {"AND (IX+n)", {{0xDD, 0xA6, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_AND, 19, {false, false, true, false, true, true, false, false, false, false, true}}},
        {"AND (IY+n)", {{0xFD, 0xA6, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_AND, 19, {false, false, true, false, true, true, false, false, false, false, true}}},
        {"AND r", {{0xA0}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_AND, 4, {false, false, true, false, true, true, false, false, false, false, true}}},
        {"AND N", {{0xE6, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_AND, 7, {false, false, true, false, true, true, false, false, false, false, true}}},

        // BIT instructions
        {"BIT b,(HL)", {{0xCB, 0x46}, 2, false, 0, true, false, false, 0, token_type::MNEMONIC_BIT, 12, {false, false, true, false, false, true, false, false, false, false, false}}},
        {"BIT b,(IX+n)", {{0xDD, 0xCB, 0x00, 0x46}, 4, false, 0, true, true, false, 0, token_type::MNEMONIC_BIT, 20, {false, false, true, false, false, true, false, false, false, false, false}}},
        {"BIT b,(IY+n)", {{0xFD, 0xCB, 0x00, 0x46}, 4, false, 0, true, true, false, 0, token_type::MNEMONIC_BIT, 20, {false, false, true, false, false, true, false, false, false, false, false}}},
        {"BIT b,r", {{0xCB, 0x40}, 2, true, 1, true, false, false, 0, token_type::MNEMONIC_BIT, 8, {false, false, true, false, false, true, false, false, false, false, false}}},

        // CALL instructions
        {"CALL C,NN", {{0xDC, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}}, // 17 if taken, 10 if not
        {"CALL M,NN", {{0xFC, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"CALL NC,NN", {{0xD4, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"CALL NN", {{0xCD, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"CALL NZ,NN", {{0xC4, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"CALL P,NN", {{0xF4, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"CALL PE,NN", {{0xEC, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"CALL PO,NN", {{0xE4, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"CALL Z,NN", {{0xCC, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_CALL, 17, {false, false, false, false, false, false, false, false, false, false, false}}},

        // CCF
        {"CCF", {{0x3F}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_CCF, 4, {false, false, false, false, true, false, false, true, false, false, true}}},

        // CP instructions
        {"CP (HL)", {{0xBE}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_CP, 7, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"CP (IX+n)", {{0xDD, 0xBE, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_CP, 19, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"CP (IY+n)", {{0xFD, 0xBE, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_CP, 19, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"CP r", {{0xB8}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_CP, 4, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"CP N", {{0xFE, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_CP, 7, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"CPD", {{0xED, 0xA9}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_CPD, 16, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"CPDR", {{0xED, 0xB9}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_CPDR, 21, {false, false, true, true, false, false, false, true, false, false, false}}}, // 21 if loop, 16 if no loop
        {"CPI", {{0xED, 0xA1}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_CPI, 16, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"CPIR", {{0xED, 0xB1}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_CPIR, 21, {false, false, true, true, false, false, false, true, false, false, false}}},

        // CPL
        {"CPL", {{0x2F}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_CPL, 4, {false, false, false, true, false, true, false, false, false, false, false}}},

        // DAA
        {"DAA", {{0x27}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DAA, 4, {false, false, true, false, false, false, false, true, false, false, true}}},

        // DEC instructions
        {"DEC (HL)", {{0x35}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 11, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC (IX+n)", {{0xDD, 0x35, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_DEC, 23, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC (IY+n)", {{0xFD, 0x35, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_DEC, 23, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC A", {{0x3D}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC B", {{0x05}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC BC", {{0x0B}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 6, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"DEC C", {{0x0D}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC D", {{0x15}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC DE", {{0x1B}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 6, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"DEC E", {{0x1D}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC H", {{0x25}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC HL", {{0x2B}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 6, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"DEC IX", {{0xDD, 0x2B}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"DEC IY", {{0xFD, 0x2B}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"DEC L", {{0x2D}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"DEC SP", {{0x3B}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DEC, 6, {false, false, false, false, false, false, false, false, false, false, false}}},

        // DI
        {"DI", {{0xF3}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_DI, 4, {false, false, false, false, false, false, false, false, false, false, false}}},

        // DJNZ
        {"DJNZ n", {{0x10, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_DJNZ, 13, {false, false, false, false, false, false, false, false, false, false, false}}}, // 13 if taken, 8 if not

        // EI
        {"EI", {{0xFB}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_EI, 4, {false, false, false, false, false, false, false, false, false, false, false}}},

        // EX instructions
        {"EX (SP),HL", {{0xE3}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_EX, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"EX (SP),IX", {{0xDD, 0xE3}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_EX, 23, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"EX (SP),IY", {{0xFD, 0xE3}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_EX, 23, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"EX AF,AF'", {{0x08}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_EX, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"EX DE,HL", {{0xEB}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_EX, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"EXX", {{0xD9}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_EXX, 4, {false, false, false, false, false, false, false, false, false, false, false}}},

        // HALT
        {"HALT", {{0x76}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_HALT, 4, {false, false, false, false, false, false, false, false, false, false, false}}},

        // IM instructions
        {"IM 0", {{0xED, 0x46}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IM, 8, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"IM 1", {{0xED, 0x56}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IM, 8, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"IM 2", {{0xED, 0x5E}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IM, 8, {false, false, false, false, false, false, false, false, false, false, false}}},

        // IN instructions
        {"IN A,(C)", {{0xED, 0x78}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IN, 12, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"IN A,(N)", {{0xDB, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_IN, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"IN B,(C)", {{0xED, 0x40}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IN, 12, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"IN C,(C)", {{0xED, 0x48}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IN, 12, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"IN D,(C)", {{0xED, 0x50}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IN, 12, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"IN E,(C)", {{0xED, 0x58}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IN, 12, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"IN H,(C)", {{0xED, 0x60}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IN, 12, {false, false, true, false, true, false, false, true, false, false, true}}},
        {"IN L,(C)", {{0xED, 0x68}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IN, 12, {false, false, true, false, true, false, false, true, false, false, true}}},

        // INC instructions
        {"INC (HL)", {{0x34}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 11, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC (IX+n)", {{0xDD, 0x34, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_INC, 23, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC (IY+n)", {{0xFD, 0x34, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_INC, 23, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC A", {{0x3C}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC B", {{0x04}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC BC", {{0x03}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 6, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"INC C", {{0x0C}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC D", {{0x14}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC DE", {{0x13}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 6, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"INC E", {{0x1C}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC H", {{0x24}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC HL", {{0x23}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 6, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"INC IX", {{0xDD, 0x23}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"INC IY", {{0xFD, 0x23}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"INC L", {{0x2C}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 4, {false, false, true, true, false, false, false, true, false, false, false}}},
        {"INC SP", {{0x33}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_INC, 6, {false, false, false, false, false, false, false, false, false, false, false}}},

        // IND, INDR, INI, INIR
        {"IND", {{0xED, 0xAA}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_IND, 16, {false, false, true, false, false, false, false, false, false, false, false}}},
        {"INDR", {{0xED, 0xBA}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_INDR, 21, {false, false, true, false, false, false, false, false, false, false, false}}}, // 21 if loop, 16 if no loop
        {"INI", {{0xED, 0xA2}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_INI, 16, {false, false, true, false, false, false, false, false, false, false, false}}},
        {"INIR", {{0xED, 0xB2}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_INIR, 21, {false, false, true, false, false, false, false, false, false, false, false}}},

        // JP instructions
        {"JP NN", {{0xC3, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP (HL)", {{0xE9}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_JP, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP (IX)", {{0xDD, 0xE9}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_JP, 8, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP (IY)", {{0xFD, 0xE9}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_JP, 8, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP C,NN", {{0xDA, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP M,NN", {{0xFA, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP NC,NN", {{0xD2, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP NZ,NN", {{0xC2, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP P,NN", {{0xF2, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP PE,NN", {{0xEA, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP PO,NN", {{0xE2, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JP Z,NN", {{0xCA, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_JP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},

        // JR instructions
        {"JR n", {{0x18, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_JR, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JR C,n", {{0x38, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_JR, 12, {false, false, false, false, false, false, false, false, false, false, false}}}, // 12 if taken, 7 if not
        {"JR NC,n", {{0x30, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_JR, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JR NZ,n", {{0x20, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_JR, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"JR Z,n", {{0x28, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_JR, 12, {false, false, false, false, false, false, false, false, false, false, false}}},

        // LD instructions
        {"LD (IX+n),r", {{0xDD, 0x70, 0x00}, 3, true, 1, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (IX+n),N", {{0xDD, 0x36, 0x00, 0x00}, 4, false, 0, false, true, true, 1, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (IY+n),r", {{0xFD, 0x70, 0x00}, 3, true, 1, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (IY+n),N", {{0xFD, 0x36, 0x00, 0x00}, 4, false, 0, false, true, true, 1, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (NN),A", {{0x32, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 13, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (NN),BC", {{0xED, 0x43, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (NN),DE", {{0xED, 0x53, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (NN),HL", {{0x22, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 16, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (NN),IX", {{0xDD, 0x22, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (NN),IY", {{0xFD, 0x22, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD (NN),SP", {{0xED, 0x73, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,(BC)", {{0x0A}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,(DE)", {{0x1A}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,(HL)", {{0x7E}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,(IX+n)", {{0xDD, 0x7E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,(IY+n)", {{0xFD, 0x7E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,(NN)", {{0x3A, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 13, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,r", {{0x78}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_LD, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,I", {{0xED, 0x57}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 9, {false, false, true, false, true, false, false, false, false, false, false}}},
        {"LD A,N", {{0x3E, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD A,R", {{0xED, 0x5F}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 9, {false, false, true, false, true, false, false, false, false, false, false}}},
        {"LD B,(HL)", {{0x46}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD B,(IX+n)", {{0xDD, 0x46, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD B,(IY+n)", {{0xFD, 0x46, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD B,r", {{0x40}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_LD, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD B,N", {{0x06, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD BC,(NN)", {{0xED, 0x4B, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD BC,NN", {{0x01, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD C,(HL)", {{0x4E}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD C,(IX+n)", {{0xDD, 0x4E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD C,(IY+n)", {{0xFD, 0x4E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD C,r", {{0x48}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_LD, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD C,N", {{0x0E, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD D,(HL)", {{0x56}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD D,(IX+n)", {{0xDD, 0x56, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD D,(IY+n)", {{0xFD, 0x56, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD D,r", {{0x50}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_LD, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD D,N", {{0x16, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD DE,(NN)", {{0xED, 0x5B, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD DE,NN", {{0x11, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD E,(HL)", {{0x5E}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD E,(IX+n)", {{0xDD, 0x5E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD E,(IY+n)", {{0xFD, 0x5E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD E,r", {{0x58}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_LD, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD E,N", {{0x1E, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD H,(HL)", {{0x66}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD H,(IX+n)", {{0xDD, 0x66, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD H,(IY+n)", {{0xFD, 0x66, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD H,r", {{0x60}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_LD, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD H,N", {{0x26, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD HL,(NN)", {{0x2A, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 16, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD HL,NN", {{0x21, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD I,A", {{0xED, 0x47}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 9, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD IX,(NN)", {{0xDD, 0x2A, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD IX,NN", {{0xDD, 0x21, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 14, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD IY,(NN)", {{0xFD, 0x2A, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD IY,NN", {{0xFD, 0x21, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 14, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD L,(HL)", {{0x6E}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD L,(IX+n)", {{0xDD, 0x6E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD L,(IY+n)", {{0xFD, 0x6E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_LD, 19, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD L,r", {{0x68}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_LD, 4, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD L,N", {{0x2E, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_LD, 7, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD R,A", {{0xED, 0x4F}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 9, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD SP,(NN)", {{0xED, 0x7B, 0x00, 0x00}, 4, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 20, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD SP,HL", {{0xF9}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 6, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD SP,IX", {{0xDD, 0xF9}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD SP,IY", {{0xFD, 0xF9}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LD, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"LD SP,NN", {{0x31, 0x00, 0x00}, 3, false, 0, false, false, true, 2, token_type::MNEMONIC_LD, 10, {false, false, false, false, false, false, false, false, false, false, false}}},

        // LDD, LDDR, LDI, LDIR
        {"LDD", {{0xED, 0xA8}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LDD, 16, {false, false, true, false, false, false, false, false, false, false, false}}},
        {"LDDR", {{0xED, 0xB8}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LDDR, 21, {false, false, true, false, false, false, false, false, false, false, false}}}, // 21 if loop, 16 if no loop
        {"LDI", {{0xED, 0xA0}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LDI, 16, {false, false, true, false, false, false, false, false, false, false, false}}},
        {"LDIR", {{0xED, 0xB0}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_LDIR, 21, {false, false, true, false, false, false, false, false, false, false, false}}},

        // NEG
        {"NEG", {{0xED, 0x44}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_NEG, 8, {false, false, true, true, false, false, false, true, false, false, true}}},

        // NOP
        {"NOP", {{0x00}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_NOP, 4, {false, false, false, false, false, false, false, false, false, false, false}}},

        // OR instructions
        {"OR (HL)", {{0xB6}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_OR, 7, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"OR (IX+n)", {{0xDD, 0xB6, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_OR, 19, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"OR (IY+n)", {{0xFD, 0xB6, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_OR, 19, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"OR r", {{0xB0}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_OR, 4, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"OR N", {{0xF6, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_OR, 7, {false, false, true, false, true, false, false, false, false, false, true}}},

        // OTDR, OTIR, OUTD, OUTI
        {"OTDR", {{0xED, 0xBB}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OTDR, 21, {false, false, true, false, false, false, false, false, false, false, false}}}, // 21 if loop, 16 if no loop
        {"OTIR", {{0xED, 0xB3}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OTIR, 21, {false, false, true, false, false, false, false, false, false, false, false}}}, // 21 if loop, 16 if no loop
        {"OUTD", {{0xED, 0xAB}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUTD, 16, {false, false, true, false, false, false, false, false, false, false, false}}},
        {"OUTI", {{0xED, 0xA3}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUTI, 16, {false, false, true, false, false, false, false, false, false, false, false}}},

        // OUT instructions
        {"OUT (C),A", {{0xED, 0x79}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUT, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"OUT (C),B", {{0xED, 0x41}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUT, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"OUT (C),C", {{0xED, 0x49}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUT, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"OUT (C),D", {{0xED, 0x51}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUT, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"OUT (C),E", {{0xED, 0x59}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUT, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"OUT (C),H", {{0xED, 0x61}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUT, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"OUT (C),L", {{0xED, 0x69}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_OUT, 12, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"OUT (N),A", {{0xD3, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_OUT, 11, {false, false, false, false, false, false, false, false, false, false, false}}},

        // POP instructions
        {"POP AF", {{0xF1}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_POP, 10, {false, false, true, false, false, false, false, true, false, false, true}}},
        {"POP BC", {{0xC1}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_POP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"POP DE", {{0xD1}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_POP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"POP HL", {{0xE1}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_POP, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"POP IX", {{0xDD, 0xE1}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_POP, 14, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"POP IY", {{0xFD, 0xE1}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_POP, 14, {false, false, false, false, false, false, false, false, false, false, false}}},

        // PUSH instructions
        {"PUSH AF", {{0xF5}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_PUSH, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"PUSH BC", {{0xC5}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_PUSH, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"PUSH DE", {{0xD5}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_PUSH, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"PUSH HL", {{0xE5}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_PUSH, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"PUSH IX", {{0xDD, 0xE5}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_PUSH, 15, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"PUSH IY", {{0xFD, 0xE5}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_PUSH, 15, {false, false, false, false, false, false, false, false, false, false, false}}},

        // RES instructions
        {"RES b,(HL)", {{0xCB, 0x86}, 2, false, 0, true, false, false, 0, token_type::MNEMONIC_RES, 15, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RES b,(IX+n)", {{0xDD, 0xCB, 0x00, 0x86}, 4, false, 0, true, true, false, 0, token_type::MNEMONIC_RES, 23, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RES b,(IY+n)", {{0xFD, 0xCB, 0x00, 0x86}, 4, false, 0, true, true, false, 0, token_type::MNEMONIC_RES, 23, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RES b,r", {{0xCB, 0x80}, 2, true, 1, true, false, false, 0, token_type::MNEMONIC_RES, 8, {false, false, false, false, false, false, false, false, false, false, false}}},

        // RET instructions
        {"RET", {{0xC9}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 10, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RET C", {{0xD8}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 11, {false, false, false, false, false, false, false, false, false, false, false}}}, // 11 if taken, 5 if not
        {"RET M", {{0xF8}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RET NC", {{0xD0}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RET NZ", {{0xC0}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RET P", {{0xF0}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RET PE", {{0xE8}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RET PO", {{0xE0}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RET Z", {{0xC8}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RET, 11, {false, false, false, false, false, false, false, false, false, false, false}}},

        // RETI
        {"RETI", {{0xED, 0x4D}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_RETI, 14, {false, false, false, false, false, false, false, false, false, false, false}}},

        // RETN
        {"RETN", {{0xED, 0x45}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_RETN, 14, {false, false, false, false, false, false, false, false, false, false, false}}},

        // RL instructions
        {"RL (HL)", {{0xCB, 0x16}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_RL, 15, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RL (IX+n)", {{0xDD, 0xCB, 0x00, 0x16}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_RL, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RL (IY+n)", {{0xFD, 0xCB, 0x00, 0x16}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_RL, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RL r", {{0xCB, 0x10}, 2, true, 1, false, false, false, 0, token_type::MNEMONIC_RL, 8, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RLA
        {"RLA", {{0x17}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RLA, 4, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RLC instructions
        {"RLC (HL)", {{0xCB, 0x06}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_RLC, 15, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RLC (IX+n)", {{0xDD, 0xCB, 0x00, 0x06}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_RLC, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RLC (IY+n)", {{0xFD, 0xCB, 0x00, 0x06}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_RLC, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RLC r", {{0xCB, 0x00}, 2, true, 1, false, false, false, 0, token_type::MNEMONIC_RLC, 8, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RLCA
        {"RLCA", {{0x07}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RLCA, 4, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RLD
        {"RLD", {{0xED, 0x6F}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_RLD, 18, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RR instructions
        {"RR (HL)", {{0xCB, 0x1E}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_RR, 15, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RR (IX+n)", {{0xDD, 0xCB, 0x00, 0x1E}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_RR, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RR (IY+n)", {{0xFD, 0xCB, 0x00, 0x1E}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_RR, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RR r", {{0xCB, 0x18}, 2, true, 1, false, false, false, 0, token_type::MNEMONIC_RR, 8, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RRA
        {"RRA", {{0x1F}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RRA, 4, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RRC instructions
        {"RRC (HL)", {{0xCB, 0x0E}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_RRC, 15, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RRC (IX+n)", {{0xDD, 0xCB, 0x00, 0x0E}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_RRC, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RRC (IY+n)", {{0xFD, 0xCB, 0x00, 0x0E}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_RRC, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"RRC r", {{0xCB, 0x08}, 2, true, 1, false, false, false, 0, token_type::MNEMONIC_RRC, 8, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RRCA
        {"RRCA", {{0x0F}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RRCA, 4, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RRD
        {"RRD", {{0xED, 0x67}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_RRD, 18, {false, false, true, false, true, false, false, false, false, false, true}}},

        // RST instructions
        {"RST 00h", {{0xC7}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RST, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RST 08h", {{0xCF}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RST, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RST 10h", {{0xD7}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RST, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RST 18h", {{0xDF}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RST, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RST 20h", {{0xE7}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RST, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RST 28h", {{0xEF}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RST, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RST 30h", {{0xF7}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RST, 11, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"RST 38h", {{0xFF}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_RST, 11, {false, false, false, false, false, false, false, false, false, false, false}}},

        // SBC instructions
        {"SBC A,(HL)", {{0x9E}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_SBC, 7, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SBC A,(IX+n)", {{0xDD, 0x9E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_SBC, 19, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SBC A,(IY+n)", {{0xFD, 0x9E, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_SBC, 19, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SBC A,r", {{0x98}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_SBC, 4, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SBC A,N", {{0xDE, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_SBC, 7, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SBC HL,BC", {{0xED, 0x42}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_SBC, 15, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SBC HL,DE", {{0xED, 0x52}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_SBC, 15, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SBC HL,HL", {{0xED, 0x62}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_SBC, 15, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SBC HL,SP", {{0xED, 0x72}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_SBC, 15, {false, false, true, true, false, false, false, true, false, false, true}}},

        // SCF
        {"SCF", {{0x37}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_SCF, 4, {false, false, true, false, true, false, false, false, false, false, true}}},

        // SET instructions
        {"SET b,(HL)", {{0xCB, 0xC6}, 2, false, 0, true, false, false, 0, token_type::MNEMONIC_SET, 15, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"SET b,(IX+n)", {{0xDD, 0xCB, 0x00, 0xC6}, 4, false, 0, true, true, false, 0, token_type::MNEMONIC_SET, 23, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"SET b,(IY+n)", {{0xFD, 0xCB, 0x00, 0xC6}, 4, false, 0, true, true, false, 0, token_type::MNEMONIC_SET, 23, {false, false, false, false, false, false, false, false, false, false, false}}},
        {"SET b,r", {{0xCB, 0xC0}, 2, true, 1, true, false, false, 0, token_type::MNEMONIC_SET, 8, {false, false, false, false, false, false, false, false, false, false, false}}},

        // SLA instructions
        {"SLA (HL)", {{0xCB, 0x26}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_SLA, 15, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SLA (IX+n)", {{0xDD, 0xCB, 0x00, 0x26}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_SLA, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SLA (IY+n)", {{0xFD, 0xCB, 0x00, 0x26}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_SLA, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SLA r", {{0xCB, 0x20}, 2, true, 1, false, false, false, 0, token_type::MNEMONIC_SLA, 8, {false, false, true, false, true, false, false, false, false, false, true}}},

        // SLL instructions
        {"SLL (HL)", {{0xCB, 0x36}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_SLL, 15, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SLL (IX+n)", {{0xDD, 0xCB, 0x00, 0x36}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_SLL, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SLL (IY+n)", {{0xFD, 0xCB, 0x00, 0x36}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_SLL, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SLL r", {{0xCB, 0x30}, 2, true, 1, false, false, false, 0, token_type::MNEMONIC_SLL, 8, {false, false, true, false, true, false, false, false, false, false, true}}},

        // SRA instructions
        {"SRA (HL)", {{0xCB, 0x2E}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_SRA, 15, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SRA (IX+n)", {{0xDD, 0xCB, 0x00, 0x2E}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_SRA, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SRA (IY+n)", {{0xFD, 0xCB, 0x00, 0x2E}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_SRA, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SRA r", {{0xCB, 0x28}, 2, true, 1, false, false, false, 0, token_type::MNEMONIC_SRA, 8, {false, false, true, false, true, false, false, false, false, false, true}}},

        // SRL instructions
        {"SRL (HL)", {{0xCB, 0x3E}, 2, false, 0, false, false, false, 0, token_type::MNEMONIC_SRL, 15, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SRL (IX+n)", {{0xDD, 0xCB, 0x00, 0x3E}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_SRL, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SRL (IY+n)", {{0xFD, 0xCB, 0x00, 0x3E}, 4, false, 0, false, true, false, 0, token_type::MNEMONIC_SRL, 23, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"SRL r", {{0xCB, 0x38}, 2, true, 1, false, false, false, 0, token_type::MNEMONIC_SRL, 8, {false, false, true, false, true, false, false, false, false, false, true}}},

        // SUB instructions
        {"SUB (HL)", {{0x96}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_SUB, 7, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SUB (IX+n)", {{0xDD, 0x96, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_SUB, 19, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SUB (IY+n)", {{0xFD, 0x96, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_SUB, 19, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SUB r", {{0x90}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_SUB, 4, {false, false, true, true, false, false, false, true, false, false, true}}},
        {"SUB N", {{0xD6, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_SUB, 7, {false, false, true, true, false, false, false, true, false, false, true}}},

        // XOR instructions
        {"XOR (HL)", {{0xAE}, 1, false, 0, false, false, false, 0, token_type::MNEMONIC_XOR, 7, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"XOR (IX+n)", {{0xDD, 0xAE, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_XOR, 19, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"XOR (IY+n)", {{0xFD, 0xAE, 0x00}, 3, false, 0, false, true, false, 0, token_type::MNEMONIC_XOR, 19, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"XOR r", {{0xA8}, 1, true, 0, false, false, false, 0, token_type::MNEMONIC_XOR, 4, {false, false, true, false, true, false, false, false, false, false, true}}},
        {"XOR N", {{0xEE, 0x00}, 2, false, 0, false, false, true, 1, token_type::MNEMONIC_XOR, 7, {false, false, true, false, true, false, false, false, false, false, true}}}};

    // Returns the register - token type map.
    const std::unordered_map<std::string, token_type> &get_register_tokens()
    {
        static std::unordered_map<std::string, token_type> m;
        if (m.empty())
        {
            m.reserve(register_info_map.size());
            for (auto &ri : register_info_map)
                m.emplace(ri.name, ri.tok);
        }
        return m;
    }

    // Returns the condition - token type map.
    const std::unordered_map<std::string, token_type> &get_condition_tokens()
    {
        static std::unordered_map<std::string, token_type> m;
        if (m.empty())
        {
            m.reserve(condition_info_map.size());
            for (auto &ci : condition_info_map)
                m.emplace(ci.name, ci.tok);
        }
        return m;
    }

    // Returns the mnemonic → token_type map
    const std::unordered_map<std::string, token_type> &get_mnemonic_tokens()
    {
        // Build once, on first call
        static std::unordered_map<std::string, token_type> tokens;
        if (tokens.empty())
        {
            tokens.reserve(instruction_table.size());
            for (const auto &[text, info] : instruction_table)
            {
                // Extract the mnemonic (first word before space or '(')
                auto end = text.find_first_of(" (");
                std::string mnem = (end == std::string::npos)
                                       ? text
                                       : text.substr(0, end);
                tokens.emplace(mnem, info.mnemonic);
            }
        }
        return tokens;
    }

    // split on spaces and commas into non‐empty tokens
    static std::vector<std::string> split_key(const std::string &s)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s)
        {
            if (c == ' ' || c == ',')
            {
                if (!cur.empty())
                {
                    out.push_back(cur);
                    cur.clear();
                }
            }
            else
            {
                cur += c;
            }
        }
        if (!cur.empty())
            out.push_back(cur);
        return out;
    }

    // is this a valid 8‑bit number literal?
    static bool is_imm8(const std::string &tok)
    {
        // decimal
        if (std::all_of(tok.begin(), tok.end(), ::isdigit))
            return std::stol(tok) >= 0 && std::stol(tok) <= 0xFF;
        // 0xNN
        if (tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X'))
        {
            return tok.size() <= 4 &&
                   std::all_of(tok.begin() + 2, tok.end(), ::isxdigit);
        }
        // NNh or NNh with optional sign $NN etc.
        if ((tok.back() == 'h' || tok.back() == 'H') &&
            std::all_of(tok.begin(), tok.end() - 1, ::isxdigit))
        {
            return true;
        }
        // $NN or $+NN or $-NN
        if (tok.size() > 1 && tok[0] == '$')
        {
            auto sub = tok.substr(1);
            if (sub[0] == '+' || sub[0] == '-')
                sub = sub.substr(1);
            return !sub.empty() && std::all_of(sub.begin(), sub.end(), ::isxdigit);
        }
        return false;
    }

    // is this a valid 16‑bit number literal?
    static bool is_imm16(const std::string &tok)
    {
        if (!is_imm8(tok))
        {
            // assume decimal up to 0xFFFF or hex up to 4 digits
            if (std::all_of(tok.begin(), tok.end(), ::isdigit))
            {
                long v = std::stol(tok);
                return v >= 0 && v <= 0xFFFF;
            }
            if ((tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')) || (tok.back() == 'h' || tok.back() == 'H') || tok[0] == '$')
            {
                // hex of any length up to 4 digits
                // we already handled 8‑bit form in is_imm8, so anything else is ok
                return tok.size() <= 6;
            }
        }
        return false;
    }

    // match a single token in the pattern against a key token
    static bool match_token(const std::string &pat, const std::string &key)
    {
        if (pat == "N")
        {
            return is_imm8(key);
        }
        if (pat == "NN")
        {
            return is_imm16(key);
        }
        // indexed displacement: (IX+n) or (IY+n)
        if ((pat == "(IX+n)" || pat == "(IY+n)"))
        {
            if (key.size() > pat.size() - 3 && key.front() == '(' && key.back() == ')')
            {
                // e.g. "(IX+5)" or "(IY-10)"
                auto inside = key.substr(1, key.size() - 2);
                // must start with IX+ or IY+
                if (inside.rfind(pat.substr(0, pat.find('+') + 1), 0) == 0)
                {
                    auto num = inside.substr(inside.find('+') + 1);
                    return !num.empty() &&
                           (match_token("N", num) || match_token("N", "+" + num) || match_token("N", "-" + num));
                }
            }
            return false;
        }
        // literal match
        return pat == key;
    }

    const instruction_info *get_instruction_info(const std::string &key)
    {
        // first try exact match
        auto it0 = instruction_table.find(key);
        if (it0 != instruction_table.end())
            return &it0->second;

        // split both
        auto kt = split_key(key);

        // try every pattern
        for (auto &pr : instruction_table)
        {
            auto pt = split_key(pr.first);
            if (pt.size() != kt.size())
                continue;
            bool ok = true;
            for (size_t i = 0; i < pt.size(); ++i)
            {
                if (!match_token(pt[i], kt[i]))
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
                return &pr.second;
        }
        return nullptr;
    }
} // namespace xas