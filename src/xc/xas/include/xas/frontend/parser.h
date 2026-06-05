// parser.h
//
// xas parser — converts a token list into a stmt_list (AST).
// The asm_mode flag controls which directive names are accepted:
//
//   sdcc  .area, .globl, .module, .ds, .dw, .byte, .ascii, .asciz, .org,
//         .equ, .define, .include, .if/.else/.endif
//
//   gnu   .section, .global, .space, .word, .2byte, .byte, .ascii, .asciz,
//         .org, .equ, .set, .include, .if/.else/.endif
//
// Both modes accept the same Z80 instruction mnemonics.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_FRONTEND_PARSER_HPP
#define XAS_FRONTEND_PARSER_HPP

#include <vector>

#include <xas/cli.h>
#include <xas/frontend/token.h>
#include <xas/frontend/ast.h>

namespace xas {

    class parser {
    public:
        //
        // Parse the token list into a statement list.
        // Throws parse_error on syntax errors.
        //
        stmt_list parse(const std::vector<token>& tokens, asm_mode mode);

    private:
        const std::vector<token>* tokens_ = nullptr;
        size_t                    pos_    = 0;
        asm_mode                  mode_   = asm_mode::sdcc;

        const token& peek(size_t ahead = 0) const;
        const token& advance();
        bool at_end() const;
        bool eat(token_kind k);
        const token& expect(token_kind k, const std::string& msg);
        void skip_to_newline();

        stmt parse_stmt();
        stmt parse_label(const std::string& name);
        stmt parse_instruction(const std::string& mnemonic);
        stmt parse_directive(const std::string& name);
        stmt parse_equ(const std::string& name);

        operand parse_operand();
        expr_ptr parse_expr();
        expr_ptr parse_or_expr();
        expr_ptr parse_xor_expr();
        expr_ptr parse_and_expr();
        expr_ptr parse_shift_expr();
        expr_ptr parse_add_expr();
        expr_ptr parse_mul_expr();
        expr_ptr parse_unary_expr();
        expr_ptr parse_primary_expr();
    };

} // namespace xas

#endif // XAS_FRONTEND_PARSER_HPP
