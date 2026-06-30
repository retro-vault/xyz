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
// Handles: * [attribute-specifier-seq] [qualifiers] [more pointers]
//
// C23 §6.7.6.1 places an optional attribute-specifier-sequence right
// after the '*' (before the qualifier list); such attributes appertain
// to the pointer itself.  This is where [[xcc::far]] belongs:
//
//   char * [[xcc::far]] p;            // far pointer to char
//   char * [[xcc::far]] * [[xcc::far]] pp;  // far pointer to far pointer

type_ptr parser::parse_pointer(type_ptr base) {
    auto apply_pointer_attrs = [&](type_ptr &ptr, const attr_list &attrs) {
        for (const auto &a : attrs) {
            if (a.name != "far")
                continue;
            if (a.ns == "xcc") {
                ptr->is_far = true;
            } else if (a.ns == "sdcc") {
                diag_.error(a.loc,
                            "[[sdcc::far]] is not supported; use [[xcc::far]]");
            } else if (a.ns.empty()) {
                diag_.error(a.loc,
                            "[[far]] is not supported; use [[xcc::far]]");
            }
        }
        apply_call_abi_attrs_to_type(ptr, attrs);
    };

    while (check(tk::STAR)) {
        consume();
        auto ptr = type::make_pointer(base);
        // C23: attribute-specifier-sequence immediately after '*' applies
        // to the pointer.  [[xcc::far]] turns it into a 24-bit far pointer.
        if (check(tk::LATTR)) {
            attr_list pattrs = parse_attr_list();
            apply_pointer_attrs(ptr, pattrs);
        }
        // qualifiers on the pointer itself
        while (peek().is_type_qualifier()) {
            if (peek().kind == tk::KW_CONST)    { ptr->is_const    = true; consume(); }
            else if (peek().kind == tk::KW_VOLATILE)  { ptr->is_volatile = true; consume(); }
            else if (peek().kind == tk::KW_RESTRICT)  { ptr->is_restrict = true; consume(); }
            else consume();
        }
        // A second attribute slot may follow the qualifiers (some toolchains
        // emit it after const/volatile); accept far there too.
        if (check(tk::LATTR)) {
            attr_list pattrs = parse_attr_list();
            apply_pointer_attrs(ptr, pattrs);
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
            bool has_vla = base && base->is_vla;
            if (check(tk::STAR)) {
                // C11 §6.7.6.2: [*] or [qualifiers *] VLA unspecified size in prototype
                consume(); // consume the '*'; expect ']' next
            } else if (!check(tk::RBRACKET)) {
                auto e = parse_assignment_expression();
                if (auto cv = const_expr_evaluator::evaluate(e.get())) {
                    sz = static_cast<int>(*cv);
                } else if (cur_func_) {
                    // VLA: runtime-sized array (only valid inside a function)
                    last_vla_size_ = std::move(e);
                    has_vla = true;
                }
            }
            expect(tk::RBRACKET);
            base = type::make_array(base, sz);
            base->is_vla = has_vla;
        } else if (check(tk::LPAREN)) {
            // Function params: parse for both named and unnamed declarators.
            // Unnamed abstract declarators like int(*)(void) need this too.
            consume();
            bool empty_param_list = check(tk::RPAREN);
            bool variadic = false;
            auto params   = parse_param_list(variadic);
            expect(tk::RPAREN);
            std::vector<type_ptr> ptypes;
            for (auto &p : params) ptypes.push_back(p->type);
            base = type::make_function(base, std::move(ptypes), variadic);
            // Traditional C keeps f() as an old-style, non-prototype
            // declaration.  f(void) also has an empty lowered parameter list,
            // but empty_param_list is false before parsing it.
            base->is_prototyped = !empty_param_list;
            // Save named params for parse_external_declaration to recover.
            last_params_   = std::move(params);
            last_variadic_ = variadic;
        } else if (check(tk::KW___ATTRIBUTE__)) {
            skip_attribute(); // __attribute__ on a declarator — ignore
        } else if (check(tk::LATTR)) {
            attr_list attrs = parse_attr_list();
            apply_call_abi_attrs_to_type(base, attrs);
        } else {
            if (reject_legacy_callconv_keyword()) {
                continue;
            }
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
            bool empty_param_list = check(tk::RPAREN);
            bool inner_variadic = false;
            auto inner_params = parse_param_list(inner_variadic);
            expect(tk::RPAREN);
            std::vector<type_ptr> ptypes;
            for (auto &p : inner_params) ptypes.push_back(p->type);
            type_ptr func_type = type::make_function(base, std::move(ptypes), inner_variadic);
            func_type->is_prototyped = !empty_param_list;
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
    auto di = parse_declarator(ds.base_type);
    apply_call_abi_attrs_to_type(di.type, ds.attrs);
    return di.type;
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
        if (!ds.base_type) {
            if (ds.is_deduced)
                error("'auto' not allowed in function prototype");
            else
                error("a type specifier is required for parameter declaration");
            ds.base_type = type::make_int();
        }

        // If we see just 'void' followed by ')', that means (void)
        if (ds.base_type->kind == type_kind::VOID && check(tk::RPAREN)) break;

        auto di = parse_declarator(ds.base_type);
        apply_call_abi_attrs_to_type(di.type, ds.attrs);

        auto pd = std::make_unique<param_decl>();
        pd->name    = di.name;
        pd->type    = di.type;
        // C parameter adjustment: T a[] is really T *a, and function
        // parameters decay to pointers to function.
        if (pd->type && pd->type->kind == type_kind::ARRAY && pd->type->base)
            pd->type = type::make_pointer(pd->type->base);
        else if (pd->type && pd->type->kind == type_kind::FUNCTION)
            pd->type = type::make_pointer(pd->type);
        pd->storage = ds.sc;
        params.push_back(std::move(pd));

        if (!match(tk::COMMA)) break;
        if (check(tk::ELLIPSIS)) { consume(); variadic = true; break; }
    }
    return params;
}

} // namespace xcc
