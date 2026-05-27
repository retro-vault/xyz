//
// parser.cpp — parser top-level: constructor, token helpers, external
//              declaration, and function-definition parsing.
//
// The parser implementation is split across several translation units:
//   parser.cpp           — this file (constructor, helpers, top-level decls)
//   parser_declspec.cpp  — parse_declaration_specifiers, struct/enum bodies
//   parser_declarator.cpp — parse_pointer, parse_declarator, parse_param_list
//   parser_stmt.cpp      — statement parsing
//   parser_init.cpp      — parse_initializer
//   parser_expr.cpp      — expression parsing
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/parser.h"
#include "frontend/const_eval.h"
#include <cassert>
#include <cstring>
#include <algorithm>
#include <optional>

namespace xcc {

// ----- Constructor ---------------------------------------------------

parser::parser(lexer &lex) : lex_(lex), diag_(lex.diag()) {}

// ----- token helpers -------------------------------------------------

const token &parser::peek() { return lex_.peek(); }

token parser::consume() { return lex_.next(); }

token parser::expect(tk k) {
    if (!check(k)) {
        token tmp; tmp.kind = k;
        error(peek(), (std::string("expected '") +
              tmp.kind_name() + "'").c_str());
    }
    return consume();
}

bool parser::check(tk k) const { return lex_.peek().kind == k; }

bool parser::match(tk k) {
    if (!check(k)) return false;
    consume(); return true;
}

void parser::error(const token &t, const char *msg) {
    diag_.error(t.loc, "%s", msg);
}

void parser::error(const char *msg) { error(peek(), msg); }

void parser::sync_to_semicolon() {
    while (!check(tk::SEMICOLON) && !check(tk::END_OF_FILE))
        consume();
    if (check(tk::SEMICOLON)) consume();
}

std::string parser::fresh_label() {
    return "__xcc_L" + std::to_string(temp_label_++);
}

void parser::skip_attribute() {
    consume(); // consume __attribute__
    expect(tk::LPAREN);
    expect(tk::LPAREN);
    int depth = 2;
    while (depth > 0 && !check(tk::END_OF_FILE)) {
        if      (check(tk::LPAREN))  { depth++; consume(); }
        else if (check(tk::RPAREN))  { depth--; consume(); }
        else consume();
    }
}


// ----- type helpers --------------------------------------------------

bool parser::is_type_start() const {
    tk k = lex_.peek().kind;
    switch (k) {
    case tk::KW_VOID: case tk::KW_CHAR: case tk::KW_SHORT: case tk::KW_INT:
    case tk::KW_LONG: case tk::KW_FLOAT: case tk::KW_DOUBLE:
    case tk::KW_SIGNED: case tk::KW_UNSIGNED: case tk::KW__BOOL:
    case tk::KW__COMPLEX: case tk::KW__IMAGINARY:
    case tk::KW_STRUCT: case tk::KW_UNION: case tk::KW_ENUM:
    case tk::KW_CONST: case tk::KW_VOLATILE: case tk::KW_RESTRICT:
    case tk::KW__ATOMIC:
    case tk::KW_AUTO: case tk::KW_EXTERN: case tk::KW_REGISTER:
    case tk::KW_STATIC: case tk::KW_TYPEDEF: case tk::KW__THREAD_LOCAL:
    case tk::KW_INLINE: case tk::KW__NORETURN:
    case tk::KW___TYPEOF__:
        return true;
    case tk::IDENT: {
        auto sym = syms_.lookup(lex_.peek().text);
        return sym && sym->kind == sym_kind::TYPE;
    }
    default: return false;
    }
}

// ----- Symbol-creation helpers ---------------------------------------

std::shared_ptr<symbol> parser::make_local_sym(const std::string &name,
                                                type_ptr t, storage_class sc) {
    auto sym        = std::make_shared<symbol>();
    sym->name       = name;
    sym->kind       = sym_kind::VAR;
    sym->type       = t;
    sym->storage    = sc;
    sym->stack_offset = alloc_local(t->size());
    syms_.insert(sym);
    return sym;
}

std::string parser::make_static_sym(std::shared_ptr<symbol> &out,
                                     const std::string &name, type_ptr t) {
    std::string mangled = "_" + (cur_func_ ? cur_func_->name : "global")
                        + "__" + name
                        + "_" + std::to_string(static_local_counter_++);
    out           = std::make_shared<symbol>();
    out->name     = name;
    out->kind     = sym_kind::VAR;
    out->type     = t;
    out->storage  = storage_class::STATIC;
    out->is_global = true;
    out->asm_name  = mangled;
    syms_.insert(out);
    return mangled;
}

std::shared_ptr<symbol> parser::make_type_sym(const std::string &name, type_ptr t) {
    auto sym   = std::make_shared<symbol>();
    sym->name  = name;
    sym->kind  = sym_kind::TYPE;
    sym->type  = t;
    syms_.insert(sym);
    return sym;
}

// ----- External declaration ------------------------------------------

std::unique_ptr<translation_unit> parser::parse() {
    auto tu = std::make_unique<translation_unit>();
    while (!check(tk::END_OF_FILE)) {
        auto d = parse_external_declaration();
        if (d) tu->decls.push_back(std::move(d));
    }
    return tu;
}

decl_ptr parser::parse_external_declaration() {
    if (!pending_decls_.empty()) {
        auto d = std::move(pending_decls_.front());
        pending_decls_.pop_front();
        return d;
    }
    if (check(tk::END_OF_FILE)) return nullptr;
    if (check(tk::KW__STATIC_ASSERT)) { parse_static_assert(); return nullptr; }

    source_loc loc = peek().loc;
    decl_spec  ds  = parse_declaration_specifiers();

    // May have no declarator (e.g., struct declaration)
    if (check(tk::SEMICOLON)) { consume(); return nullptr; }

    auto di = parse_declarator(ds.base_type);
    const std::string &name      = di.name;
    type_ptr           decl_type = di.type;

    if (decl_type->is_func()) {
        std::vector<std::unique_ptr<param_decl>> params = std::move(di.params);
        bool variadic = di.variadic;
        // If for some reason params was empty, build unnamed stubs.
        if (params.empty() && !decl_type->params.empty()) {
            for (auto &pt : decl_type->params) {
                auto pd = std::make_unique<param_decl>();
                pd->type = pt;
                params.push_back(std::move(pd));
            }
        }

        if (check(tk::LBRACE)) {
            return parse_function_definition(decl_type->ret, name, ds.sc,
                                              std::move(params),
                                              variadic, loc);
        }
        // Prototype
        auto fd = std::make_unique<func_decl>();
        fd->loc        = loc;
        fd->name       = name;
        fd->type       = decl_type;
        fd->storage    = ds.sc;
        fd->is_variadic = decl_type->variadic;
        fd->params     = std::move(params);
        fd->body       = nullptr;

        auto sym = std::make_shared<symbol>();
        sym->name    = name;
        sym->kind    = sym_kind::FUNC;
        sym->type    = decl_type;
        sym->storage = ds.sc;
        sym->is_global = true;
        syms_.insert(sym);
        fd->sym = sym;

        // Handle comma-separated declarations: int f(int), g(int), a;
        while (match(tk::COMMA)) {
            auto edi = parse_declarator(ds.base_type);
            if (edi.type->is_func()) {
                auto efd = std::make_unique<func_decl>();
                efd->loc = loc; efd->name = edi.name; efd->type = edi.type;
                efd->storage = ds.sc; efd->is_variadic = edi.variadic;
                efd->params = std::move(edi.params); efd->body = nullptr;
                auto esym = std::make_shared<symbol>();
                esym->name = edi.name; esym->kind = sym_kind::FUNC;
                esym->type = edi.type; esym->storage = ds.sc; esym->is_global = true;
                syms_.insert(esym); efd->sym = esym;
                pending_decls_.push_back(std::move(efd));
            } else {
                auto evd = std::make_unique<var_decl>();
                evd->loc = loc; evd->name = edi.name; evd->type = edi.type;
                evd->storage = ds.sc;
                if (match(tk::EQ)) {
                    evd->init = check(tk::LBRACE) ? parse_initializer(edi.type)
                                                   : parse_assignment_expression();
                }
                auto esym = std::make_shared<symbol>();
                esym->name = edi.name; esym->kind = sym_kind::VAR;
                esym->type = edi.type; esym->storage = ds.sc; esym->is_global = true;
                syms_.insert(esym); evd->sym = esym;
                pending_decls_.push_back(std::move(evd));
            }
        }
        expect(tk::SEMICOLON);
        return fd;
    }

    // Typedef at file scope: register as TYPE symbol, no variable emitted.
    if (ds.sc == storage_class::TYPEDEF) {
        auto tsym = std::make_shared<symbol>();
        tsym->name = name;
        tsym->kind = sym_kind::TYPE;
        tsym->type = decl_type;
        syms_.insert(tsym);
        auto td = std::make_unique<typedef_decl>();
        td->loc  = loc;
        td->name = name;
        td->type = decl_type;
        // Handle comma-separated typedef names: typedef int a, b; (rare but valid)
        while (match(tk::COMMA)) {
            auto edi = parse_declarator(ds.base_type);
            auto esym = std::make_shared<symbol>();
            esym->name = edi.name; esym->kind = sym_kind::TYPE; esym->type = edi.type;
            syms_.insert(esym);
        }
        expect(tk::SEMICOLON);
        return td;
    }

    // Global variable declaration
    auto vd = std::make_unique<var_decl>();
    vd->loc     = loc;
    vd->name    = name;
    vd->type    = decl_type;
    vd->storage = ds.sc;

    if (match(tk::EQ)) {
        vd->init = check(tk::LBRACE) ? parse_initializer(decl_type)
                                      : parse_assignment_expression();
    }

    auto sym = std::make_shared<symbol>();
    sym->name      = name;
    sym->kind      = sym_kind::VAR;
    sym->type      = decl_type;
    sym->storage   = ds.sc;
    sym->is_global = true;
    sym->is_tls    = ds.is_tls;
    syms_.insert(sym);
    vd->sym = sym;
    if (sym->type && sym->type->is_const && sym->type->is_integer() && vd->init) {
        if (auto cv = const_expr_evaluator::evaluate(vd->init.get())) sym->const_val = cv;
    }

    // Handle comma-separated declarators: int a, b, c;
    while (match(tk::COMMA)) {
        auto edi = parse_declarator(ds.base_type);
        auto evd = std::make_unique<var_decl>();
        evd->loc     = loc;
        evd->name    = edi.name;
        evd->type    = edi.type;
        evd->storage = ds.sc;
        if (match(tk::EQ)) {
            evd->init = check(tk::LBRACE) ? parse_initializer(edi.type)
                                           : parse_assignment_expression();
        }
        auto esym = std::make_shared<symbol>();
        esym->name      = edi.name;
        esym->kind      = sym_kind::VAR;
        esym->type      = edi.type;
        esym->storage   = ds.sc;
        esym->is_global = true;
        esym->is_tls    = ds.is_tls;
        syms_.insert(esym);
        evd->sym = esym;
        pending_decls_.push_back(std::move(evd));
    }

    expect(tk::SEMICOLON);
    return vd;
}

// ----- Function definition -------------------------------------------

decl_ptr parser::parse_function_definition(
    type_ptr ret, std::string name, storage_class sc,
    std::vector<std::unique_ptr<param_decl>> params,
    bool variadic, source_loc loc)
{
    auto fd = std::make_unique<func_decl>();
    fd->loc        = loc;
    fd->name       = name;
    fd->storage    = sc;
    fd->is_variadic = variadic;

    // Build function type
    std::vector<type_ptr> ptypes;
    for (auto &p : params) ptypes.push_back(p->type);
    fd->type = type::make_function(ret, ptypes, variadic);

    // Register the function symbol
    auto fsym = std::make_shared<symbol>();
    fsym->name      = name;
    fsym->kind      = sym_kind::FUNC;
    fsym->type      = fd->type;
    fsym->storage   = sc;
    fsym->is_global = true;
    syms_.insert(fsym);
    fd->sym = fsym;

    // Enter function scope; RAII guard ensures pop even on early return.
    scope_guard sg{syms_};
    cur_func_ = fd.get();
    frame_.reset();

    // Register parameters
    // xcc ABI: params are pushed right-to-left; in a stack frame
    // they appear in order at IX+4, IX+6, ...
    static int unnamed_idx = 0;
    for (auto &p : params) {
        if (p->name.empty()) {
            p->name = "__arg" + std::to_string(unnamed_idx++);
        }
        auto psym = std::make_shared<symbol>();
        psym->name       = p->name;
        psym->kind       = sym_kind::VAR;
        psym->type       = p->type;
        psym->is_param   = true;
        psym->stack_offset = alloc_param(p->type->size());
        p->sym = psym;
        syms_.insert(psym);
    }
    fd->params = std::move(params);

    // Parse body
    fd->body = parse_compound_statement();

    fd->local_bytes = -frame_.local_offset;

    cur_func_ = nullptr;
    // sg destructs here → pop_scope()
    return fd;
}

} // namespace xcc
