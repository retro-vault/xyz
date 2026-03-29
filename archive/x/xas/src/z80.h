//
// Z80 instruction metadata and lookup interfaces.
// Defines structures for describing instruction opcodes, flag effects,
// and provides accessors for building an instruction decoder.
//
// USAGE:
//   auto& table = z80::get_instruction_table();
//   auto info = table.at("LD");
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "token.h" // token_type definitions

namespace xas
{
    // Register info including token type.
    struct reg_info
    {
        std::string name;
        int idx;
        token_type tok;
    };

    // Condition info including token type.
    struct condition_info
    {
        std::string name;
        int idx;
        token_type tok;
    };

    /// Effects on CPU flags caused by an instruction.
    struct flag_effects
    {
        bool z_set : 1;      ///< Zero flag is set.
        bool z_reset : 1;    ///< Zero flag is reset.
        bool z_affected : 1; ///< Zero flag is affected by this instruction.
        bool n_set : 1;      ///< Subtract/negative flag is set.
        bool n_reset : 1;    ///< Subtract/negative flag is reset.
        bool h_set : 1;      ///< Half-carry flag is set.
        bool h_reset : 1;    ///< Half-carry flag is reset.
        bool h_affected : 1; ///< Half-carry flag is affected.
        bool c_set : 1;      ///< Carry flag is set.
        bool c_reset : 1;    ///< Carry flag is reset.
        bool c_affected : 1; ///< Carry flag is affected.
    };

    /// Metadata describing a single Z80 instruction.
    struct instruction_info
    {
        std::vector<uint8_t> opcode; ///< Base opcode bytes for the instruction.
        uint8_t size;                ///< Total size of the instruction (bytes).
        bool has_rb;                 ///< True if instruction encodes a register in opcode.
        uint8_t rb_position;         ///< Byte index where register bits are inserted.
        bool has_bit;                ///< True if instruction encodes a bit number.
        bool has_displacement;       ///< True if instruction uses IX/IY displacement.
        bool has_immediate;          ///< True if instruction has immediate operand(s).
        uint8_t immediate_bytes;     ///< Number of immediate bytes (1 or 2).
        token_type mnemonic;         ///< Token type for the mnemonic (e.g. LD, INC).
        uint16_t cycles;             ///< Clock cycles consumed (T-states).
        flag_effects flags;          ///< CPU flag effects for this instruction.
    };

    // Returns the register - token type map.
    extern const std::unordered_map<std::string, token_type> &get_register_tokens();

    // Returns the condition - token type map.
    extern const std::unordered_map<std::string, token_type> &get_condition_tokens();

    // Returns the mnemonic → token_type map
    extern const std::unordered_map<std::string, token_type> &get_mnemonic_tokens();

    /// @brief Returns instruction info for given mnemonic.
    extern const instruction_info *get_instruction_info(const std::string &key);

} // namespace xas