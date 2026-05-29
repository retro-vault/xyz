//
// parser_declarator.cpp — pointer, declarator, and parameter-list parsing.
//
// Implements: parse_pointer(), parse_direct_declarator_suffix(),
//             parse_declarator(), parse_abstract_declarator(),
//             parse_type_name(), parse_param_list().
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/parser.h"

namespace xcc {

// ----- parse_pointer -------------------------------------------------
// Handles: * [qualifiers] [more pointers]

type_ptr parser::parse_pointer(type_ptr base) {
    while (check(tk::STAR)) {
        consume();
        auto ptr = type::make_pointer(base);
        // qualifiers on the pointer itself
        while (peek().is_type_qualifier()) {
            if (peek().kind == tk::KW_CONST)    { ptr->is_const    = true; consume(); }
            else if (peek().kind == tk::KW_VOLATILE)  { ptr->is_volatile = true; consume(); }
            else if (peek().kind == tk::KW_RESTRICT)  { ptr->is_restrict = true; consume(); }
            else consume();
        }
        base = ptr;
    }
    return base;
}

// ----- parse_declarator ----------------------------------------------
// Parses: [*...] [ident | '(' declarator ')'] [array/func suffixes]
// Sets name to the declared identifier.

type_ptr parser::parse_direct_declarator_suffix(type_ptr base, std::string &/*name*/) {
    while (true) {
        if (check(tk::LBRACKET)) {
            consume();
            // C11 §6.7.6.3: optional 'static' and type-qualifiers before array size
            if (check(tk::KW_STATIC)) consume();
            while (peek().is_type_qualifier()) consume();
            if (check(tk::KW_STATIC)) consume(); // static may follow qualifiers
            int sz = 0;
            if (check(tk::STAR)) {
                // C11 §6.7.6.2: [*] or [qualifiers *] VLA unspecified size in prototype
                consume(); // consume the '*'; expect ']' next
            } else if (!check(tk::RBRACKET)) {
                auto e = parse_assignment_expression();
                if (auto *lit = dynamic_cast<int_literal_expr*>(e.get())) {
                    sz = static_cast<int>(lit->value);
                } else if (cur_func_) {
                    // VLA: runtime-sized array (only valid inside a function)
                    last_vla_size_ = std::move(e);
                    sz = 0;
                }
            }
            expect(tk::RBRACKET);
            base = type::make_array(base, sz);
            if (last_vla_size_) base->is_vla = true;
        } else if (check(tk::LPAREN)) {
            // Function params: parse for both named and unnamed declarators.
            // Unnamed abstract declarators like int(*)(void) need this too.
            consume();
            bool variadic = false;
            auto params   = parse_param_list(variadic);
            expect(tk::RPAREN);
            std::vector<type_ptr> ptypes;
            for (auto &p : params) ptypes.push_back(p->type);
            base = type::make_function(base, std::move(ptypes), variadic);
            // Save named params for parse_external_declaration to recover.
            last_params_   = std::move(params);
            last_variadic_ = variadic;
        } else if (check(tk::KW___ATTRIBUTE__)) {
            skip_attribute(); // __attribute__ on a declarator — ignore
        } else if (check(tk::LATTR)) {
            parse_attr_list(); // [[...]] on a declarator suffix — ignore for now
        } else {
            break;
        }
    }
    return base;
}

declarator_info parser::parse_declarator(type_ptr base) {
    declarator_info di;

    base = parse_pointer(base);

    // Grouped declarator: '(' '*'... [name] ')' — function pointer syntax.
    // e.g.  int (*fp)(int, int)   typedef void (*CB)(void)
    // Detect by consuming '(' then peeking for '*'.
    // If NOT '*', fall back to the redundant-paren case: int (name).
    if (check(tk::LPAREN)) {
        consume();
        // Skip any __attribute__((...)) that appears before the declarator content.
        while (check(tk::KW___ATTRIBUTE__)) skip_attribute();
        if (check(tk::STAR)) {
            // Grouped declarator: (*...) handles all variants uniformly via recursion:
            //   (*fp)(T)          — simple function pointer
            //   (*)(T)            — unnamed function pointer (abstract)
            //   (*fptr4[4])(T)    — array of function pointers
            //   (*f(a,b))(T)      — function returning function pointer
            //   (* (*p)(a,b))(T)  — nested grouped declarator
            type_ptr ptr_chain = parse_pointer(type::make_void());

            // Parse whatever is inside the parens as an inner declarator.
            // This naturally handles name, name[n], name(params), nested (*...), etc.
            auto inner = parse_declarator(type::make_void());
            di.name = inner.name;
            expect(tk::RPAREN);
            type_ptr outer = parse_direct_declarator_suffix(base, di.name);

            // Thread outer (the return/base type) into ptr_chain's void leaf.
            type_ptr *leaf = &ptr_chain;
            while ((*leaf)->kind == type_kind::POINTER && (*leaf)->base &&
                   (*leaf)->base->kind != type_kind::VOID)
                leaf = &(*leaf)->base;
            (*leaf)->base = outer;

            // Thread ptr_chain into inner.type's void leaf.
            auto thread_into = [&](type_ptr &t, type_ptr val) {
                type_ptr *p2 = &t;
                while (*p2 && (*p2)->base && (*p2)->base->kind != type_kind::VOID)
                    p2 = &(*p2)->base;
                if (*p2 && (*p2)->base) (*p2)->base = val;
                else if (*p2 && (*p2)->kind == type_kind::VOID) t = val;
            };
            type_ptr inner_type = inner.type;
            thread_into(inner_type, ptr_chain);
            di.type = inner_type;
            di.params   = std::move(inner.params);
            di.variadic = inner.variadic;
            last_params_.clear(); last_variadic_ = false;
            return di;
        } else if (is_type_start() || check(tk::RPAREN) || check(tk::ELLIPSIS)) {
            // Function-type parameter: int (int x, ...) or int (void)
            bool inner_variadic = false;
            auto inner_params = parse_param_list(inner_variadic);
            expect(tk::RPAREN);
            std::vector<type_ptr> ptypes;
            for (auto &p : inner_params) ptypes.push_back(p->type);
            type_ptr func_type = type::make_function(base, std::move(ptypes), inner_variadic);
            di.type = parse_direct_declarator_suffix(func_type, di.name);
            last_params_   = std::move(inner_params);
            last_variadic_ = inner_variadic;
        } else {
            // Redundant-paren case: int (name) — recurse and close paren.
            auto inner = parse_declarator(base);
            di.name = inner.name;
            expect(tk::RPAREN);
            di.type = parse_direct_declarator_suffix(inner.type, di.name);
        }
    } else {
        if (check(tk::IDENT)) di.name = consume().text;
        di.type = parse_direct_declarator_suffix(base, di.name);
    }

    // Collect side channels written by parse_direct_declarator_suffix.
    di.params   = std::move(last_params_);
    di.variadic = last_variadic_;
    last_params_.clear();
    last_variadic_ = false;

    return di;
}

type_ptr parser::parse_abstract_declarator(type_ptr base) {
    return parse_declarator(base).type;
}

type_ptr parser::parse_type_name() {
    decl_spec ds = parse_declaration_specifiers();
    return parse_declarator(ds.base_type).type;
}

// ----- parse_param_list ----------------------------------------------

std::vector<std::unique_ptr<param_decl>>
parser::parse_param_list(bool &variadic) {
    variadic = false;
    std::vector<std::unique_ptr<param_decl>> params;

    // void parameter list = no parameters
    if (check(tk::KW_VOID) && lex_.peek().kind == tk::KW_VOID) {
        // peek one more ahead is hard without two-tok lookahead; just check RParen
        // We handle it: if the next after void is ) it means (void)
        auto saved_tok = peek();
        if (saved_tok.kind == tk::KW_VOID) {
            // consume if followed by ')'
            lexer &l = lex_;
            (void)l; // We peek at lex to see what's after void
            // Simplest: just parse as normal; if we see void ) we're done.
        }
    }

    if (check(tk::RPAREN)) return params; // empty ()

    while (true) {
        if (check(tk::ELLIPSIS)) {
            consume();
            variadic = true;
            break;
        }

        // Parameter may be just 'void' with no name -> empty params
        if (peek().kind == tk::KW_VOID && lex_.peek().kind == tk::KW_VOID) {
            // hard to double-peek cleanly; parse anyway
        }

        decl_spec ds = parse_declaration_specifiers();

        // If we see just 'void' followed by ')', that means (void)
        if (ds.base_type->kind == type_kind::VOID && check(tk::RPAREN)) break;

        auto di = parse_declarator(ds.base_type);

        auto pd = std::make_unique<param_decl>();
        pd->name    = di.name;
        pd->type    = di.type;
        pd->storage = ds.sc;
        params.push_back(std::move(pd));

        if (!match(tk::COMMA)) break;
        if (check(tk::ELLIPSIS)) { consume(); variadic = true; break; }
    }
    return params;
}

} // namespace xcc
