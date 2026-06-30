//
// ast.h — Abstract Syntax Tree node types for C11.
//
// Defines every AST node used by the parser and consumed by IrGen.
// Nodes are split into three hierarchies — expr, stmt, decl — each
// with its own visitor interface (expr_visitor, stmt_visitor,
// decl_visitor).  Visitors are the primary traversal mechanism;
// IrGen implements all three.
//
// Ownership model: all nodes are heap-allocated and owned through
// unique_ptr aliases (expr_ptr, stmt_ptr, decl_ptr).  Shared
// ownership is not used for AST nodes; the translation_unit owns
// the top-level declaration list which transitively owns everything.
//
// The type field on every expr is filled in by the parser during
// expression resolution.  is_lvalue marks expressions that can appear
// on the left-hand side of an assignment.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "token.h"
#include "types.h"
#include "symtab.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace xcc {

// ----- C23 attribute representation ----------------------------------
//
// An attribute parsed from [[ns::name(arg1, arg2)]] syntax.
// ns is empty for standard (unnamespaced) attributes.
// args holds raw string tokens for arguments; empty when no parentheses.

struct attr {
    std::string              ns;
    std::string              name;
    std::vector<std::string> args;
    source_loc               loc;
};

using attr_list = std::vector<attr>;

// Forward declarations
struct expr;  struct stmt;  struct decl;
struct translation_unit;

using expr_ptr = std::unique_ptr<expr>;
using stmt_ptr = std::unique_ptr<stmt>;
using decl_ptr = std::unique_ptr<decl>;

// ----- visitor forward declarations ----------------------------------

struct expr_visitor;
struct stmt_visitor;
struct decl_visitor;

// ═══════════════════════════════════════════════════════════════════════════════
// Expressions
// ═══════════════════════════════════════════════════════════════════════════════

//
// Binary operator tag.  Assignment operators carry is_assign = true on
// the enclosing binary_expr to distinguish them from arithmetic ops
// that happen to share the same symbol.
//
enum class bin_op {
    ADD, SUB, MUL, DIV, MOD,
    AND, OR, XOR, SHL, SHR,
    EQ, NE, LT, LE, GT, GE,
    LAND, LOR,
    COMMA,
    ASSIGN,
    ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,
    AND_ASSIGN, OR_ASSIGN, XOR_ASSIGN, SHL_ASSIGN, RHS_ASSIGN,
};

//
// Unary operator tag.
//
enum class unary_op {
    NEG,        // unary -
    NOT,        // logical !
    BNOT,       // bitwise ~
    ADDR,       // &expr  (address-of)
    DEREF,      // *expr  (dereference)
    PRE_INC,    // ++x
    PRE_DEC,    // --x
    POST_INC,   // x++
    POST_DEC,   // x--
};

// ----- expr base -----------------------------------------------------

struct expr {
    source_loc loc;
    type_ptr   type;
    bool       is_lvalue = false;

    virtual ~expr() = default;
    virtual void accept(expr_visitor &) = 0;
};

// ----- concrete expression nodes -------------------------------------

struct int_literal_expr : expr {
    int64_t value;
    void accept(expr_visitor &v) override;
};

struct float_literal_expr : expr {
    double value;
    void accept(expr_visitor &v) override;
};

struct char_literal_expr : expr {
    int64_t value;
    void accept(expr_visitor &v) override;
};

//
// A string literal after adjacent-string concatenation.
// value holds the decoded byte content (without NUL terminator;
// the code generator appends one).
//
struct string_literal_expr : expr {
    std::string value;
    int char_width = 1; // 1=char, 2=char16_t, 4=char32_t, 8=char8_t
    void accept(expr_visitor &v) override;
};

//
// An identifier expression.  sym is the resolved symbol after name
// lookup; it is nullptr for unresolved identifiers (parse error).
//
struct ident_expr : expr {
    std::string name;
    sym_ptr     sym;
    void accept(expr_visitor &v) override;
};

//
// A binary expression.  Covers arithmetic, comparison, logical,
// bitwise, assignment, and comma operators.
//
struct binary_expr : expr {
    bin_op   op;
    expr_ptr left, right;
    void accept(expr_visitor &v) override;
};

struct unary_expr : expr {
    unary_op op;
    expr_ptr operand;
    void accept(expr_visitor &v) override;
};

struct cast_expr : expr {
    type_ptr target_type;
    expr_ptr operand;
    void accept(expr_visitor &v) override;
};

struct call_expr : expr {
    expr_ptr              callee;
    std::vector<expr_ptr> args;
    void accept(expr_visitor &v) override;
};

//
// Array subscript: base[index].
//
struct index_expr : expr {
    expr_ptr base, index;
    void accept(expr_visitor &v) override;
};

//
// Struct/union member access: object.member (is_arrow=false) or
// object->member (is_arrow=true).
//
struct member_expr : expr {
    expr_ptr    object;
    std::string member;
    bool        is_arrow = false;
    void accept(expr_visitor &v) override;
};

//
// sizeof(type) or sizeof(expr), and _Alignof(type).
// Exactly one of sizeof_type / sizeof_expr is non-null.
// is_alignof = true selects _Alignof behaviour.
//
struct sizeof_expr : expr {
    type_ptr sizeof_type;
    expr_ptr sizeof_expr_op;
    bool     is_alignof = false;
    bool     is_countof = false;
    int      align_override = 0; // symbol-level alignas override for alignof(expr)
    void accept(expr_visitor &v) override;
};

//
// Aggregate or scalar initializer list: { a, b, c }.
// Supports C11 designated initializers: { .field = val } and { [N] = val }.
// field_name and array_index are both empty for plain (non-designated) elements.
//
struct init_list_expr : expr {
    struct elem {
        std::optional<std::string> field_name;   // .field = value
        std::optional<int64_t>     array_index;  // [N]    = value
        expr_ptr                   value;
    };
    std::vector<elem> elements;
    void accept(expr_visitor &v) override;
};

//
// Compound literal: (type){ initializer }.
// sym is the anonymous local allocated by the parser during parsing;
// it is nullptr when the literal appears at file scope.
//
struct compound_literal_expr : expr {
    sym_ptr  sym;   // anonymous local variable (stack-allocated by parser)
    expr_ptr init;  // init_list_expr or scalar expression
    void accept(expr_visitor &v) override;
};

//
// Ternary conditional: cond ? then_expr : else_expr.
//
struct conditional_expr : expr {
    expr_ptr cond, then_expr, else_expr;
    void accept(expr_visitor &v) override;
};

//
// GNU statement expression: ({ stmts... }).
// body is a compound_stmt. type is the type of the last expr_stmt, or void.
//
struct stmt_expr : expr {
    stmt_ptr body; // compound_stmt
    void accept(expr_visitor &v) override;
};

// ----- expr_visitor --------------------------------------------------

struct expr_visitor {
    virtual void visit(int_literal_expr      &) = 0;
    virtual void visit(float_literal_expr    &) = 0;
    virtual void visit(char_literal_expr     &) = 0;
    virtual void visit(string_literal_expr   &) = 0;
    virtual void visit(ident_expr            &) = 0;
    virtual void visit(binary_expr           &) = 0;
    virtual void visit(unary_expr            &) = 0;
    virtual void visit(cast_expr             &) = 0;
    virtual void visit(call_expr             &) = 0;
    virtual void visit(index_expr            &) = 0;
    virtual void visit(member_expr           &) = 0;
    virtual void visit(sizeof_expr           &) = 0;
    virtual void visit(compound_literal_expr &) = 0;
    virtual void visit(conditional_expr      &) = 0;
    virtual void visit(init_list_expr        &) = 0;
    virtual void visit(stmt_expr             &) = 0;
    virtual ~expr_visitor() = default;
};

inline void int_literal_expr      ::accept(expr_visitor &v) { v.visit(*this); }
inline void float_literal_expr    ::accept(expr_visitor &v) { v.visit(*this); }
inline void char_literal_expr     ::accept(expr_visitor &v) { v.visit(*this); }
inline void string_literal_expr   ::accept(expr_visitor &v) { v.visit(*this); }
inline void ident_expr            ::accept(expr_visitor &v) { v.visit(*this); }
inline void binary_expr           ::accept(expr_visitor &v) { v.visit(*this); }
inline void unary_expr            ::accept(expr_visitor &v) { v.visit(*this); }
inline void cast_expr             ::accept(expr_visitor &v) { v.visit(*this); }
inline void call_expr             ::accept(expr_visitor &v) { v.visit(*this); }
inline void index_expr            ::accept(expr_visitor &v) { v.visit(*this); }
inline void member_expr           ::accept(expr_visitor &v) { v.visit(*this); }
inline void sizeof_expr           ::accept(expr_visitor &v) { v.visit(*this); }
inline void compound_literal_expr ::accept(expr_visitor &v) { v.visit(*this); }
inline void conditional_expr      ::accept(expr_visitor &v) { v.visit(*this); }
inline void init_list_expr        ::accept(expr_visitor &v) { v.visit(*this); }
inline void stmt_expr             ::accept(expr_visitor &v) { v.visit(*this); }

// ----- default_expr_visitor ------------------------------------------
// No-op implementations for all expression node types.
// Extend this instead of expr_visitor when a pass only cares about
// a subset of nodes — only override what you need.
struct default_expr_visitor : expr_visitor {
    void visit(int_literal_expr      &) override {}
    void visit(float_literal_expr    &) override {}
    void visit(char_literal_expr     &) override {}
    void visit(string_literal_expr   &) override {}
    void visit(ident_expr            &) override {}
    void visit(binary_expr           &) override {}
    void visit(unary_expr            &) override {}
    void visit(cast_expr             &) override {}
    void visit(call_expr             &) override {}
    void visit(index_expr            &) override {}
    void visit(member_expr           &) override {}
    void visit(sizeof_expr           &) override {}
    void visit(compound_literal_expr &) override {}
    void visit(conditional_expr      &) override {}
    void visit(init_list_expr        &) override {}
    void visit(stmt_expr             &) override {}
};

// ═══════════════════════════════════════════════════════════════════════════════
// Statements
// ═══════════════════════════════════════════════════════════════════════════════

struct stmt {
    source_loc loc;
    virtual ~stmt() = default;
    virtual void accept(stmt_visitor &) = 0;
};

struct compound_stmt : stmt {
    std::vector<stmt_ptr> body;
    void accept(stmt_visitor &v) override;
};

//
// An expression used as a statement (e.g., a function call or
// assignment).  expr may be nullptr for an empty statement (;).
//
struct expr_stmt : stmt {
    expr_ptr expr;
    void accept(stmt_visitor &v) override;
};

//
// A declaration used as a statement inside a block.
// Holds one or more local variable declarations.
//
struct decl_stmt : stmt {
    std::vector<decl_ptr> decls;
    void accept(stmt_visitor &v) override;
};

//
// A return statement.  value is nullptr for bare "return;".
//
struct return_stmt : stmt {
    expr_ptr value;
    void accept(stmt_visitor &v) override;
};

struct if_stmt : stmt {
    expr_ptr cond;
    stmt_ptr then_body;
    stmt_ptr else_body; // nullptr if no else branch
    void accept(stmt_visitor &v) override;
};

struct while_stmt : stmt {
    expr_ptr cond;
    stmt_ptr body;
    void accept(stmt_visitor &v) override;
};

struct do_while_stmt : stmt {
    stmt_ptr body;
    expr_ptr cond;
    void accept(stmt_visitor &v) override;
};

//
// For loop.  init is either a decl_stmt or expr_stmt (or nullptr).
// cond and step may both be nullptr.
//
struct for_stmt : stmt {
    stmt_ptr init;
    expr_ptr cond;
    expr_ptr step;
    stmt_ptr body;
    void accept(stmt_visitor &v) override;
};

struct break_stmt : stmt {
    void accept(stmt_visitor &v) override;
};

struct continue_stmt : stmt {
    void accept(stmt_visitor &v) override;
};

struct goto_stmt : stmt {
    std::string label;
    void accept(stmt_visitor &v) override;
};

struct label_stmt : stmt {
    std::string name;
    stmt_ptr    body;
    void accept(stmt_visitor &v) override;
};

struct switch_stmt : stmt {
    expr_ptr cond;
    stmt_ptr body;
    void accept(stmt_visitor &v) override;
};

//
// A case label (is_default=false) or a default label (is_default=true).
// value is the constant expression for case labels.
//
struct case_stmt : stmt {
    expr_ptr value;
    stmt_ptr body;
    bool     is_default = false;
    void accept(stmt_visitor &v) override;
};

//
// GNU-style inline assembly statement: __asm__("text") or __asm("text").
// text is the raw assembly string (already unescaped by the lexer).
// Constraints are not supported; only the basic form is handled.
//
struct asm_stmt : stmt {
    std::string text;
    void accept(stmt_visitor &v) override;
};

// ----- stmt_visitor --------------------------------------------------

struct stmt_visitor {
    virtual void visit(compound_stmt  &) = 0;
    virtual void visit(expr_stmt      &) = 0;
    virtual void visit(decl_stmt      &) = 0;
    virtual void visit(return_stmt    &) = 0;
    virtual void visit(if_stmt        &) = 0;
    virtual void visit(while_stmt     &) = 0;
    virtual void visit(do_while_stmt  &) = 0;
    virtual void visit(for_stmt       &) = 0;
    virtual void visit(break_stmt     &) = 0;
    virtual void visit(continue_stmt  &) = 0;
    virtual void visit(goto_stmt      &) = 0;
    virtual void visit(label_stmt     &) = 0;
    virtual void visit(switch_stmt    &) = 0;
    virtual void visit(case_stmt      &) = 0;
    virtual void visit(asm_stmt       &) = 0;
    virtual ~stmt_visitor() = default;
};

inline void compound_stmt ::accept(stmt_visitor &v) { v.visit(*this); }
inline void expr_stmt     ::accept(stmt_visitor &v) { v.visit(*this); }
inline void decl_stmt     ::accept(stmt_visitor &v) { v.visit(*this); }
inline void return_stmt   ::accept(stmt_visitor &v) { v.visit(*this); }
inline void if_stmt       ::accept(stmt_visitor &v) { v.visit(*this); }
inline void while_stmt    ::accept(stmt_visitor &v) { v.visit(*this); }
inline void do_while_stmt ::accept(stmt_visitor &v) { v.visit(*this); }
inline void for_stmt      ::accept(stmt_visitor &v) { v.visit(*this); }
inline void break_stmt    ::accept(stmt_visitor &v) { v.visit(*this); }
inline void continue_stmt ::accept(stmt_visitor &v) { v.visit(*this); }
inline void goto_stmt     ::accept(stmt_visitor &v) { v.visit(*this); }
inline void label_stmt    ::accept(stmt_visitor &v) { v.visit(*this); }
inline void switch_stmt   ::accept(stmt_visitor &v) { v.visit(*this); }
inline void asm_stmt      ::accept(stmt_visitor &v) { v.visit(*this); }
inline void case_stmt     ::accept(stmt_visitor &v) { v.visit(*this); }

// ----- default_stmt_visitor ------------------------------------------
struct default_stmt_visitor : stmt_visitor {
    void visit(compound_stmt  &) override {}
    void visit(expr_stmt      &) override {}
    void visit(decl_stmt      &) override {}
    void visit(return_stmt    &) override {}
    void visit(if_stmt        &) override {}
    void visit(while_stmt     &) override {}
    void visit(do_while_stmt  &) override {}
    void visit(for_stmt       &) override {}
    void visit(break_stmt     &) override {}
    void visit(continue_stmt  &) override {}
    void visit(goto_stmt      &) override {}
    void visit(label_stmt     &) override {}
    void visit(switch_stmt    &) override {}
    void visit(case_stmt      &) override {}
    void visit(asm_stmt       &) override {}
};

// ═══════════════════════════════════════════════════════════════════════════════
// Declarations
// ═══════════════════════════════════════════════════════════════════════════════

struct decl {
    source_loc    loc;
    std::string   name;
    type_ptr      type;
    storage_class storage = storage_class::NONE;
    virtual ~decl() = default;
    virtual void accept(decl_visitor &) = 0;
};

//
// A function parameter declaration.  sym is the resolved symbol set
// by the parser when the enclosing function body is parsed.
//
struct param_decl : decl {
    sym_ptr sym;
    void accept(decl_visitor &v) override;
};

//
// A variable declaration with optional initializer.
//
struct var_decl : decl {
    expr_ptr  init;      // nullptr if no initializer
    sym_ptr   sym;
    expr_ptr  vla_size;  // for VLA: runtime size expression (non-owning raw after parse)
    attr_list attrs;
    void accept(decl_visitor &v) override;
};

//
// A function declaration (prototype or definition).
// body is nullptr for a prototype.  local_bytes is the total stack
// space reserved for all locals, filled in during parsing.
//
struct func_decl : decl {
    std::vector<std::unique_ptr<param_decl>> params;
    bool      is_variadic = false;
    bool      is_inline = false;
    stmt_ptr  body;
    sym_ptr   sym;
    int       local_bytes = 0;
    attr_list attrs;
    void accept(decl_visitor &v) override;
};

//
// A typedef declaration.  type holds the aliased type.
//
struct typedef_decl : decl {
    void accept(decl_visitor &v) override;
};

//
// A struct or union definition at file scope.
// name holds the tag; type holds the complete struct/union type.
//
struct struct_decl : decl {
    void accept(decl_visitor &v) override;
};

// ----- decl_visitor --------------------------------------------------

struct decl_visitor {
    virtual void visit(var_decl    &) = 0;
    virtual void visit(func_decl   &) = 0;
    virtual void visit(param_decl  &) = 0;
    virtual void visit(typedef_decl&) = 0;
    virtual void visit(struct_decl &) = 0;
    virtual ~decl_visitor() = default;
};

inline void var_decl    ::accept(decl_visitor &v) { v.visit(*this); }
inline void func_decl   ::accept(decl_visitor &v) { v.visit(*this); }
inline void param_decl  ::accept(decl_visitor &v) { v.visit(*this); }
inline void typedef_decl::accept(decl_visitor &v) { v.visit(*this); }
inline void struct_decl ::accept(decl_visitor &v) { v.visit(*this); }

// ----- default_decl_visitor ------------------------------------------
struct default_decl_visitor : decl_visitor {
    void visit(var_decl     &) override {}
    void visit(func_decl    &) override {}
    void visit(param_decl   &) override {}
    void visit(typedef_decl &) override {}
    void visit(struct_decl  &) override {}
};

// ═══════════════════════════════════════════════════════════════════════════════
// Translation unit
// ═══════════════════════════════════════════════════════════════════════════════

//
// The root of the AST produced by the parser.
// decls holds all top-level declarations in source order.
//
struct translation_unit {
    std::vector<decl_ptr> decls;
};

} // namespace xcc
