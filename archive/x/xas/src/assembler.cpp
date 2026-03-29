//
// Two‑pass Z80 assembler: parses tokens into machine code and a symbol table.
// First pass records labels and computes sizes. Second pass emits bytes,
// using `get_instruction_info()` and falling back to generic patterns
// (e.g. replacing immediate values with `N`).
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#include "assembler.h"
#include "z80.h"
#include <stdexcept>
#include <sstream>

namespace xas
{

    assembler::assembler(const std::vector<token> &tokens)
        : toks(tokens)
    {
    }

    obj assembler::assemble()
    {
        first_pass();
        index = 0;
        location_counter = 0;
        second_pass();
        return result;
    }

    void assembler::first_pass()
    {
        while (!is_at_end())
        {
            if (is_label_decl())
            {
                auto name = advance(); // IDENT
                advance();             // COLON
                result.labels[name.lexeme] = location_counter;
            }
            else if (is_directive())
            {
                auto dir = advance();
                if (dir.type == token_type::DIRECTIVE_ORG)
                {
                    auto addr = advance();
                    location_counter = std::stoul(addr.lexeme, nullptr, 0);
                }
                else if (dir.type == token_type::DIRECTIVE_DB)
                {
                    size_t count = 0;
                    do
                    {
                        if (peek().type == token_type::IMMEDIATE_VALUE)
                        {
                            advance();
                            count++;
                        }
                        if (peek().type == token_type::COMMA)
                            advance();
                    } while (!is_at_end() && peek().type == token_type::IMMEDIATE_VALUE);
                    location_counter += count;
                }
                else
                {
                    while (!is_at_end() && !is_label_decl() && !is_directive() && !is_mnemonic())
                        advance();
                }
            }
            else if (is_mnemonic())
            {
                auto mnem = advance();
                std::vector<token> ops;
                while (!is_at_end())
                {
                    auto t = peek();
                    if (t.type == token_type::COMMA)
                    {
                        advance();
                        ops.push_back(t);
                        continue;
                    }
                    if (is_label_decl() || is_directive() || is_mnemonic())
                        break;
                    ops.push_back(advance());
                }
                // build key
                std::ostringstream key;
                key << mnem.lexeme;
                if (!ops.empty())
                {
                    key << ' ';
                    for (auto &o : ops)
                        key << o.lexeme;
                }
                auto info = get_instruction_info(key.str());
                if (!info)
                {
                    // fallback: replace all immediates with 'N'
                    std::ostringstream pat;
                    pat << mnem.lexeme;
                    if (!ops.empty())
                    {
                        pat << ' ';
                        for (auto &o : ops)
                        {
                            if (o.type == token_type::IMMEDIATE_VALUE)
                                pat << 'N';
                            else
                                pat << o.lexeme;
                        }
                    }
                    info = get_instruction_info(pat.str());
                    if (!info)
                        throw std::runtime_error("Unknown instruction: " + key.str());
                }
                location_counter += info->size;
            }
            else if (peek().type == token_type::IDENTIFIER)
            {
                throw std::runtime_error("Unexpected identifier: " + peek().lexeme);
            }
            else
            {
                advance();
            }
        }
    }

    void assembler::second_pass()
    {
        while (!is_at_end())
        {
            if (is_label_decl())
            {
                advance(); // IDENT
                advance(); // COLON
            }
            else if (is_directive())
            {
                auto dir = advance();
                if (dir.type == token_type::DIRECTIVE_ORG)
                {
                    auto addr = advance();
                    location_counter = std::stoul(addr.lexeme, nullptr, 0);
                }
                else if (dir.type == token_type::DIRECTIVE_DB)
                {
                    do
                    {
                        auto val = advance();
                        uint8_t b = static_cast<uint8_t>(std::stoul(val.lexeme, nullptr, 0));
                        result.code.push_back(b);
                        location_counter++;
                        if (peek().type == token_type::COMMA)
                            advance();
                    } while (!is_at_end() && peek().type == token_type::IMMEDIATE_VALUE);
                }
                else
                {
                    while (!is_at_end() && !is_label_decl() && !is_directive() && !is_mnemonic())
                        advance();
                }
            }
            else if (is_mnemonic())
            {
                auto mnem = advance();
                std::vector<token> ops;
                while (!is_at_end())
                {
                    auto t = peek();
                    if (t.type == token_type::COMMA)
                    {
                        advance();
                        ops.push_back(t);
                        continue;
                    }
                    if (is_label_decl() || is_directive() || is_mnemonic())
                        break;
                    ops.push_back(advance());
                }
                std::ostringstream key;
                key << mnem.lexeme;
                if (!ops.empty())
                {
                    key << ' ';
                    for (auto &o : ops)
                        key << o.lexeme;
                }
                const instruction_info *info = get_instruction_info(key.str());
                if (!info)
                {
                    std::ostringstream pat;
                    pat << mnem.lexeme;
                    if (!ops.empty())
                    {
                        pat << ' ';
                        for (auto &o : ops)
                        {
                            if (o.type == token_type::IMMEDIATE_VALUE)
                                pat << 'N';
                            else
                                pat << o.lexeme;
                        }
                    }
                    info = get_instruction_info(pat.str());
                    if (!info)
                        throw std::runtime_error("Unknown instruction: " + key.str());
                }
                // emit opcodes
                for (auto b : info->opcode)
                    result.code.push_back(b);

                // patch register bits
                if (info->has_rb)
                {
                    auto &regmap = get_register_tokens();
                    for (auto &o : ops)
                    {
                        if (token_category(o.type) == CAT_REGISTER8)
                        {
                            auto tt = regmap.at(o.lexeme);
                            uint8_t idx = static_cast<uint16_t>(tt) & ~CAT_REGISTER8;
                            size_t pos = result.code.size() - info->opcode.size() + info->rb_position;
                            result.code[pos] |= idx;
                            break;
                        }
                    }
                }

                if (info->has_displacement)
                {
                    int d = std::stoi(ops.back().lexeme, nullptr, 0);
                    result.code.push_back(static_cast<uint8_t>(d));
                }
                if (info->has_immediate)
                {
                    int v = std::stoi(ops.back().lexeme, nullptr, 0);
                    if (info->immediate_bytes == 1)
                        result.code.push_back(static_cast<uint8_t>(v));
                    else
                    {
                        result.code.push_back(static_cast<uint8_t>(v & 0xFF));
                        result.code.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
                    }
                }
                location_counter += info->size;
            }
            else if (peek().type == token_type::IDENTIFIER)
            {
                throw std::runtime_error("Unexpected identifier: " + peek().lexeme);
            }
            else
            {
                advance();
            }
        }
    }

    token assembler::peek() const
    {
        return index < toks.size()
                   ? toks[index]
                   : token(token_type::END_OF_FILE, "");
    }

    token assembler::advance()
    {
        return toks[index++];
    }

    bool assembler::is_at_end() const
    {
        return peek().type == token_type::END_OF_FILE;
    }

    bool assembler::is_label_decl() const
    {
        return peek().type == token_type::IDENTIFIER && (index + 1 < toks.size() && toks[index + 1].type == token_type::COLON);
    }

    bool assembler::is_directive() const
    {
        return token_category(peek().type) == CAT_DIRECTIVE;
    }

    bool assembler::is_mnemonic() const
    {
        return token_category(peek().type) == CAT_MNEMONIC;
    }

} // namespace xas
