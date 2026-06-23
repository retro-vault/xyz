//
// parser_init.cpp — initializer parsing.
//
// Implements: parse_initializer().
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/parser.h"
#include "frontend/const_eval.h"

namespace xcc {

expr_ptr parser::parse_initializer(type_ptr type) {
    auto loc = peek().loc;
    expect(tk::LBRACE);

    // C23: empty braces {} zero-initialise any type.
    if (check(tk::RBRACE)) {
        consume();
        // For scalar types return a literal zero of the correct type.
        if (type && type->is_scalar() && !type->is_array()) {
            auto z = std::make_unique<int_literal_expr>();
            z->loc   = loc;
            z->value = 0;
            z->type  = type;
            return z;
        }
        // For aggregates return empty init-list (irgen zero-fills missing elements).
        auto il = std::make_unique<init_list_expr>();
        il->loc  = loc;
        il->type = type;
        return il;
    }

    auto il = std::make_unique<init_list_expr>();
    il->loc  = loc;
    il->type = nullptr;
    while (!check(tk::RBRACE) && !check(tk::END_OF_FILE)) {
        init_list_expr::elem e;
        // Consume a chain of designators (C99/C11: one or more .field or [idx] then =).
        bool has_desig = false;
        while (check(tk::DOT) || check(tk::LBRACKET)) {
            has_desig = true;
            if (check(tk::DOT)) {
                consume();
                e.field_name = expect(tk::IDENT).text;
            } else { // LBRACKET
                consume();
                auto idx_expr = parse_assignment_expression();
                if (check(tk::ELLIPSIS)) {
                    // GCC range: [lo ... hi]
                    consume();
                    parse_assignment_expression(); // hi — ignore range, use lo
                }
                auto idx_val  = const_expr_evaluator::evaluate(idx_expr.get());
                if (idx_val) e.array_index = *idx_val;
                expect(tk::RBRACKET);
            }
        }
        if (has_desig) expect(tk::EQ);

        if (check(tk::LBRACE))
            e.value = parse_initializer(nullptr);
        else
            e.value = parse_assignment_expression();
        il->elements.push_back(std::move(e));
        if (!match(tk::COMMA)) break;
    }
    expect(tk::RBRACE);
    return il;
}

} // namespace xcc
