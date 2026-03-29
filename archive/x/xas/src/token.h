//
// Defines Z80‑specific token types with embedded category bits.
//
// USAGE:
//   xas::token t(xas::token_type::MNEMONIC_LD, "LD");
//   if (token_category(t.type) == CAT_MNEMONIC) { /* handle mnemonic */ }
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#pragma once

#include <cstdint>
#include <string>

namespace xas
{

    /// Top‑three‑bit masks for token_type categories
    constexpr uint16_t CAT_MASK = 0xE000;
    constexpr uint16_t CAT_MNEMONIC = 0x2000;
    constexpr uint16_t CAT_REGISTER8 = 0x4000;
    constexpr uint16_t CAT_REGISTER16 = 0x6000;
    constexpr uint16_t CAT_CONDITION = 0x8000;
    constexpr uint16_t CAT_DIRECTIVE = 0xA000; ///< added directive category
    constexpr uint16_t CAT_IMMEDIATE = 0xC000;
    constexpr uint16_t CAT_PUNCTUATION = 0xE000; ///< operators & punctuation

    /// Z80 token types for assembly lexemes.
    enum class token_type : uint16_t
    {
        // Z80 mnemonics
        MNEMONIC_ADC = CAT_MNEMONIC | 0x01,
        MNEMONIC_ADD = CAT_MNEMONIC | 0x02,
        MNEMONIC_AND = CAT_MNEMONIC | 0x03,
        MNEMONIC_BIT = CAT_MNEMONIC | 0x04,
        MNEMONIC_CALL = CAT_MNEMONIC | 0x05,
        MNEMONIC_CCF = CAT_MNEMONIC | 0x06,
        MNEMONIC_CP = CAT_MNEMONIC | 0x07,
        MNEMONIC_CPD = CAT_MNEMONIC | 0x08,
        MNEMONIC_CPDR = CAT_MNEMONIC | 0x09,
        MNEMONIC_CPI = CAT_MNEMONIC | 0x0A,
        MNEMONIC_CPIR = CAT_MNEMONIC | 0x0B,
        MNEMONIC_CPL = CAT_MNEMONIC | 0x0C,
        MNEMONIC_DAA = CAT_MNEMONIC | 0x0D,
        MNEMONIC_DEC = CAT_MNEMONIC | 0x0E,
        MNEMONIC_DI = CAT_MNEMONIC | 0x0F,
        MNEMONIC_DJNZ = CAT_MNEMONIC | 0x10,
        MNEMONIC_EI = CAT_MNEMONIC | 0x11,
        MNEMONIC_EX = CAT_MNEMONIC | 0x12,
        MNEMONIC_EXX = CAT_MNEMONIC | 0x13,
        MNEMONIC_HALT = CAT_MNEMONIC | 0x14,
        MNEMONIC_IM = CAT_MNEMONIC | 0x15,
        MNEMONIC_IN = CAT_MNEMONIC | 0x16,
        MNEMONIC_INC = CAT_MNEMONIC | 0x17,
        MNEMONIC_IND = CAT_MNEMONIC | 0x18,
        MNEMONIC_INDR = CAT_MNEMONIC | 0x19,
        MNEMONIC_INI = CAT_MNEMONIC | 0x1A,
        MNEMONIC_INIR = CAT_MNEMONIC | 0x1B,
        MNEMONIC_JP = CAT_MNEMONIC | 0x1C,
        MNEMONIC_JR = CAT_MNEMONIC | 0x1D,
        MNEMONIC_LD = CAT_MNEMONIC | 0x1E,
        MNEMONIC_LDD = CAT_MNEMONIC | 0x1F,
        MNEMONIC_LDDR = CAT_MNEMONIC | 0x20,
        MNEMONIC_LDI = CAT_MNEMONIC | 0x21,
        MNEMONIC_LDIR = CAT_MNEMONIC | 0x22,
        MNEMONIC_NEG = CAT_MNEMONIC | 0x23,
        MNEMONIC_NOP = CAT_MNEMONIC | 0x24,
        MNEMONIC_OR = CAT_MNEMONIC | 0x25,
        MNEMONIC_OTDR = CAT_MNEMONIC | 0x26,
        MNEMONIC_OTIR = CAT_MNEMONIC | 0x27,
        MNEMONIC_OUT = CAT_MNEMONIC | 0x28,
        MNEMONIC_OUTD = CAT_MNEMONIC | 0x29,
        MNEMONIC_OUTI = CAT_MNEMONIC | 0x2A,
        MNEMONIC_POP = CAT_MNEMONIC | 0x2B,
        MNEMONIC_PUSH = CAT_MNEMONIC | 0x2C,
        MNEMONIC_RES = CAT_MNEMONIC | 0x2D,
        MNEMONIC_RET = CAT_MNEMONIC | 0x2E,
        MNEMONIC_RETI = CAT_MNEMONIC | 0x2F,
        MNEMONIC_RETN = CAT_MNEMONIC | 0x30,
        MNEMONIC_RL = CAT_MNEMONIC | 0x31,
        MNEMONIC_RLA = CAT_MNEMONIC | 0x32,
        MNEMONIC_RLC = CAT_MNEMONIC | 0x33,
        MNEMONIC_RLCA = CAT_MNEMONIC | 0x34,
        MNEMONIC_RLD = CAT_MNEMONIC | 0x35,
        MNEMONIC_RR = CAT_MNEMONIC | 0x36,
        MNEMONIC_RRA = CAT_MNEMONIC | 0x37,
        MNEMONIC_RRC = CAT_MNEMONIC | 0x38,
        MNEMONIC_RRCA = CAT_MNEMONIC | 0x39,
        MNEMONIC_RRD = CAT_MNEMONIC | 0x3A,
        MNEMONIC_RST = CAT_MNEMONIC | 0x3B,
        MNEMONIC_SBC = CAT_MNEMONIC | 0x3C,
        MNEMONIC_SCF = CAT_MNEMONIC | 0x3D,
        MNEMONIC_SET = CAT_MNEMONIC | 0x3E,
        MNEMONIC_SLA = CAT_MNEMONIC | 0x3F,
        MNEMONIC_SLL = CAT_MNEMONIC | 0x40,
        MNEMONIC_SRA = CAT_MNEMONIC | 0x41,
        MNEMONIC_SRL = CAT_MNEMONIC | 0x42,
        MNEMONIC_SUB = CAT_MNEMONIC | 0x43,
        MNEMONIC_XOR = CAT_MNEMONIC | 0x44,

        // 8‑bit Registers
        REGISTER8_A = CAT_REGISTER8 | 0x00,
        REGISTER8_B = CAT_REGISTER8 | 0x01,
        REGISTER8_C = CAT_REGISTER8 | 0x02,
        REGISTER8_D = CAT_REGISTER8 | 0x03,
        REGISTER8_E = CAT_REGISTER8 | 0x04,
        REGISTER8_H = CAT_REGISTER8 | 0x05,
        REGISTER8_L = CAT_REGISTER8 | 0x06,
        REGISTER8_I = CAT_REGISTER8 | 0x07,
        REGISTER8_R = CAT_REGISTER8 | 0x08,

        // 16‑bit Registers
        REGISTER16_AF = CAT_REGISTER16 | 0x00,
        REGISTER16_BC = CAT_REGISTER16 | 0x01,
        REGISTER16_DE = CAT_REGISTER16 | 0x02,
        REGISTER16_HL = CAT_REGISTER16 | 0x03,
        REGISTER16_SP = CAT_REGISTER16 | 0x04,
        REGISTER16_IX = CAT_REGISTER16 | 0x05,
        REGISTER16_IY = CAT_REGISTER16 | 0x06,

        // Condition codes
        CONDITION_NZ = CAT_CONDITION | 0x00,
        CONDITION_Z = CAT_CONDITION | 0x01,
        CONDITION_NC = CAT_CONDITION | 0x02,
        CONDITION_C = CAT_CONDITION | 0x03,
        CONDITION_PO = CAT_CONDITION | 0x04,
        CONDITION_PE = CAT_CONDITION | 0x05,
        CONDITION_P = CAT_CONDITION | 0x06,
        CONDITION_M = CAT_CONDITION | 0x07,

        // Directives
        DIRECTIVE_ORG = CAT_DIRECTIVE | 0x01,
        DIRECTIVE_DB = CAT_DIRECTIVE | 0x02,

        // Other tokens
        OPERATOR = CAT_PUNCTUATION | 0x00,      ///< '+', '-' operators
        IMMEDIATE_VALUE = CAT_IMMEDIATE | 0x00, ///< numeric literal
        IDENTIFIER = 0x0000,                    ///< user-defined names (labels, symbols)
        COLON = CAT_PUNCTUATION | 0x01,         ///< ':'
        COMMA = CAT_PUNCTUATION | 0x02,         ///< ','
        LBRACKET = CAT_PUNCTUATION | 0x03,      ///< '('
        RBRACKET = CAT_PUNCTUATION | 0x04,      ///< ')'
        UNKNOWN = 0x0001,                       ///< unrecognized lexeme
        END_OF_FILE = 0x0002                    ///< end of input marker
    };

    /// Represents a single token lexeme and its type.
    struct token
    {
        token_type type;    ///< The specific type of this token.
        std::string lexeme; ///< The raw text of the token.

        token(token_type type, const std::string &lexeme)
            : type(type), lexeme(lexeme) {}
    };

    /// Extract the category bits from a token_type
    inline uint16_t token_category(token_type t)
    {
        return static_cast<uint16_t>(t) & CAT_MASK;
    }

} // namespace xas
