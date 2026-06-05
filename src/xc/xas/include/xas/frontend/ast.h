// ast.h
//
// Abstract syntax tree nodes for xas.  A source file is represented as a
// flat list of stmt nodes (one per logical line).  Each stmt is one of:
//
//   label_stmt    — "name:"
//   instr_stmt    — mnemonic with zero or more operand expressions
//   directive_stmt— .area, .section, .globl, .byte, .word, .ds, .org, …
//   equ_stmt      — "name EQU expr" / ".equ name, expr"
//
// Expressions (expr) can be:
//   integer_expr  — a literal integer value
//   symbol_expr   — a symbol name
//   current_expr  — $ or . (current address)
//   unary_expr    — ~, - applied to a sub-expression
//   binary_expr   — +, -, *, /, %, <<, >>, &, |, ^ applied to two sub-exprs
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_FRONTEND_AST_HPP
#define XAS_FRONTEND_AST_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace xas {

    // =========================================================================
    // Expressions
    // =========================================================================

    enum class expr_kind {
        integer,
        symbol,
        current_addr,
        unary,
        binary
    };

    struct expr;
    using expr_ptr = std::unique_ptr<expr>;

    struct expr {
        expr_kind kind;
        // integer
        int64_t   int_val = 0;
        // symbol / label reference
        std::string name;
        // unary / binary
        char      op  = 0;
        expr_ptr  lhs;
        expr_ptr  rhs;

        int source_line = 0;

        static expr_ptr make_int(int64_t v, int line = 0) {
            auto e = std::make_unique<expr>();
            e->kind = expr_kind::integer;
            e->int_val = v;
            e->source_line = line;
            return e;
        }
        static expr_ptr make_sym(const std::string& n, int line = 0) {
            auto e = std::make_unique<expr>();
            e->kind = expr_kind::symbol;
            e->name = n;
            e->source_line = line;
            return e;
        }
        static expr_ptr make_cur(int line = 0) {
            auto e = std::make_unique<expr>();
            e->kind = expr_kind::current_addr;
            e->source_line = line;
            return e;
        }
        static expr_ptr make_unary(char op, expr_ptr sub, int line = 0) {
            auto e = std::make_unique<expr>();
            e->kind = expr_kind::unary;
            e->op  = op;
            e->lhs = std::move(sub);
            e->source_line = line;
            return e;
        }
        static expr_ptr make_binary(char op, expr_ptr l, expr_ptr r, int line = 0) {
            auto e = std::make_unique<expr>();
            e->kind = expr_kind::binary;
            e->op  = op;
            e->lhs = std::move(l);
            e->rhs = std::move(r);
            e->source_line = line;
            return e;
        }
    };

    // =========================================================================
    // Operand
    // =========================================================================

    enum class operand_kind {
        reg,            // register name (a, b, hl, ix, ...)
        imm,            // immediate expression
        ind_reg,        // (reg)
        ind_ix_off,     // (IX+expr)
        ind_iy_off,     // (IY+expr)
        ind_expr,       // (expr) — indirect via 16-bit address
        cond            // condition code (NZ, Z, NC, C, PO, PE, P, M)
    };

    struct operand {
        operand_kind kind = operand_kind::imm;
        std::string  reg_name;      // for reg / ind_reg / ind_ix_off / ind_iy_off
        expr_ptr     value;         // for imm / ind_ix_off / ind_iy_off / ind_expr
        std::string  cond_name;     // for cond
        int          source_line = 0;
    };

    // =========================================================================
    // Statements
    // =========================================================================

    enum class stmt_kind {
        label,
        instruction,
        directive,
        equ
    };

    struct stmt {
        stmt_kind   kind;
        int         source_line = 0;
        std::string source_file;

        // label_stmt
        std::string label_name;
        bool        label_global = false; // exported via .globl / .global

        // instr_stmt
        std::string mnemonic;
        std::vector<operand> operands;

        // directive_stmt
        std::string directive_name; // without leading dot
        std::vector<expr_ptr> args;
        std::string string_arg;     // for .ascii / .asciz / .module

        // equ_stmt
        std::string equ_name;
        expr_ptr    equ_value;
    };

    using stmt_list = std::vector<stmt>;

} // namespace xas

#endif // XAS_FRONTEND_AST_HPP
