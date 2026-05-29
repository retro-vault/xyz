//
// parser_stmt.cpp — statement parsing.
//
// Implements: parse_statement(), parse_compound_statement(),
//             parse_if_statement(), parse_while_statement(),
//             parse_do_while_statement(), parse_for_statement(),
//             parse_return_statement(), parse_static_assert(),
//             parse_asm_statement(), parse_switch_statement(),
//             parse_label_or_expr_statement().
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/parser.h"
#include "frontend/const_eval.h"

namespace xcc {

stmt_ptr parser::parse_statement() {
    tk k = peek().kind;

    if (k == tk::KW__STATIC_ASSERT) return parse_static_assert();
    if (k == tk::KW___ASM__)        return parse_asm_statement();
    if (k == tk::KW__PRAGMA) {
        // _Pragma("string") — parse and discard as a no-op statement
        auto s = std::make_unique<expr_stmt>();
        s->loc = consume().loc;
        expect(tk::LPAREN);
        if (check(tk::STR_LIT)) consume();
        expect(tk::RPAREN);
        return s;
    }
    if (k == tk::LBRACE)    return parse_compound_statement();
    if (k == tk::KW_IF)     return parse_if_statement();
    if (k == tk::KW_WHILE)  return parse_while_statement();
    if (k == tk::KW_DO)     return parse_do_while_statement();
    if (k == tk::KW_FOR)    return parse_for_statement();
    if (k == tk::KW_RETURN) return parse_return_statement();
    if (k == tk::KW_SWITCH) return parse_switch_statement();
    if (k == tk::KW_BREAK) {
        auto s = std::make_unique<break_stmt>();
        s->loc = consume().loc;
        expect(tk::SEMICOLON);
        return s;
    }
    if (k == tk::KW_CONTINUE) {
        auto s = std::make_unique<continue_stmt>();
        s->loc = consume().loc;
        expect(tk::SEMICOLON);
        return s;
    }
    if (k == tk::KW_GOTO) {
        auto s = std::make_unique<goto_stmt>();
        s->loc = consume().loc;
        s->label = expect(tk::IDENT).text;
        expect(tk::SEMICOLON);
        return s;
    }
    if (k == tk::KW_CASE || k == tk::KW_DEFAULT) {
        auto s = std::make_unique<case_stmt>();
        s->loc = consume().loc;
        if (k == tk::KW_CASE) {
            s->value = parse_expression();
            s->is_default = false;
        } else {
            s->is_default = true;
        }
        expect(tk::COLON);
        // A case label is just a jump target; the body is NOT owned here.
        // Following statements are siblings in the enclosing compound_stmt.
        s->body = nullptr;
        return s;
    }
    if (k == tk::SEMICOLON) {
        auto s = std::make_unique<expr_stmt>();
        s->loc = consume().loc;
        return s;
    }

    return parse_label_or_expr_statement();
}

stmt_ptr parser::parse_compound_statement() {
    auto stmt = std::make_unique<compound_stmt>();
    stmt->loc = peek().loc;
    expect(tk::LBRACE);
    scope_guard sg{syms_};

    while (!check(tk::RBRACE) && !check(tk::END_OF_FILE)) {
        if (is_type_start()) {
            // Declaration inside compound statement
            auto ds = std::make_unique<decl_stmt>();
            ds->loc = peek().loc;
            decl_spec dspec = parse_declaration_specifiers();

            if (check(tk::SEMICOLON)) { consume(); continue; } // bare type decl

            do {
                // For C23 auto deduction, pass a placeholder int base so
                // parse_declarator never receives nullptr; the real type
                // is resolved from the initializer below.
                type_ptr parse_base = dspec.is_deduced ? type::make_int() : dspec.base_type;
                auto      di    = parse_declarator(parse_base);
                auto      vname = di.name;
                type_ptr  vtype = di.type;
                auto vd = std::make_unique<var_decl>();
                vd->loc     = peek().loc;
                vd->name    = vname;
                vd->type    = vtype;
                vd->storage = dspec.sc;

                if (dspec.sc == storage_class::TYPEDEF) {
                    make_type_sym(vname, vtype);
                } else if (vtype->is_vla && last_vla_size_) {
                    // Hidden size local stores the runtime byte count.
                    auto sz_sym        = make_local_sym(
                        "__vsz_" + vname + "_" + std::to_string(vla_counter_++),
                        type::make_uint(), storage_class::NONE);
                    auto ptr_type      = type::make_pointer(vtype->base);
                    auto vsym          = make_local_sym(vname, ptr_type, dspec.sc);
                    vsym->vla_size_sym = sz_sym;
                    vd->sym      = vsym;
                    vd->type     = ptr_type;
                    vd->vla_size = std::move(last_vla_size_);
                } else if (dspec.sc == storage_class::STATIC) {
                    std::shared_ptr<symbol> vsym;
                    std::string mangled = make_static_sym(vsym, vname, vtype);
                    vd->sym  = vsym;
                    vd->name = mangled; // irgen uses this for global emission
                    if (match(tk::EQ)) {
                        vd->init = check(tk::LBRACE) ? parse_initializer(vtype)
                                                      : parse_assignment_expression();
                        if (vsym && vsym->type && vsym->type->is_const &&
                            vsym->type->is_integer() && vd->init) {
                            if (auto cv = const_expr_evaluator::evaluate(vd->init.get()))
                                vsym->const_val = cv;
                        }
                    }
                } else if (dspec.sc == storage_class::EXTERN) {
                    auto vsym       = std::make_shared<symbol>();
                    vsym->name      = vname;
                    vsym->kind      = sym_kind::VAR;
                    vsym->type      = vtype;
                    vsym->storage   = dspec.sc;
                    vsym->is_global = true;
                    syms_.insert(vsym);
                    vd->sym = vsym;
                } else {
                    // C23 auto deduction: allocate with placeholder type,
                    // then fix up after parsing the initializer.
                    type_ptr alloc_type = dspec.is_deduced ? type::make_int() : vtype;
                    vd->sym = make_local_sym(vname, alloc_type, dspec.sc);
                    if (match(tk::EQ)) {
                        vd->init = check(tk::LBRACE) ? parse_initializer(vtype)
                                                      : parse_assignment_expression();
                        if (dspec.is_deduced && vd->init && vd->init->type) {
                            // Resolve deduced type from the initializer.
                            vtype        = vd->init->type->unqual();
                            vd->type     = vtype;
                            vd->sym->type = vtype;
                            // Shrink/grow the stack slot to the correct size.
                            // (The alloc_local already advanced local_offset by 2;
                            //  adjust if the real type is smaller or larger.)
                            int real_sz = vtype->size() > 0 ? vtype->size() : 2;
                            frame_.local_offset -= (real_sz - (alloc_type->size() > 0 ? alloc_type->size() : 2));
                        } else if (dspec.is_deduced) {
                            error("'auto' variable requires an initializer with a known type");
                        }
                        if (vd->sym && vd->sym->type && vd->sym->type->is_const &&
                            vd->sym->type->is_integer() && vd->init) {
                            if (auto cv = const_expr_evaluator::evaluate(vd->init.get()))
                                vd->sym->const_val = cv;
                        }
                    } else if (dspec.is_deduced) {
                        error("'auto' variable requires an initializer");
                    }
                }
                ds->decls.push_back(std::move(vd));
            } while (match(tk::COMMA));

            expect(tk::SEMICOLON);
            stmt->body.push_back(std::move(ds));
        } else {
            stmt->body.push_back(parse_statement());
        }
    }

    // sg destructs here → pop_scope()
    expect(tk::RBRACE);
    return stmt;
}

stmt_ptr parser::parse_if_statement() {
    auto s = std::make_unique<if_stmt>();
    s->loc = consume().loc; // consume 'if'
    // C99: declarations in the controlling expression are scoped to the 'if'
    // statement, not the enclosing block (e.g. enum constants in sizeof()).
    syms_.push_scope();
    expect(tk::LPAREN);
    s->cond = parse_expression();
    expect(tk::RPAREN);
    s->then_body = parse_statement();
    if (match(tk::KW_ELSE))
        s->else_body = parse_statement();
    syms_.pop_scope();
    return s;
}

stmt_ptr parser::parse_while_statement() {
    auto s = std::make_unique<while_stmt>();
    s->loc = consume().loc;
    syms_.push_scope(); // C99: same scoping rule for 'while'
    expect(tk::LPAREN);
    s->cond = parse_expression();
    expect(tk::RPAREN);
    s->body = parse_statement();
    syms_.pop_scope();
    return s;
}

stmt_ptr parser::parse_do_while_statement() {
    auto s = std::make_unique<do_while_stmt>();
    s->loc = consume().loc;
    s->body = parse_statement();
    expect(tk::KW_WHILE);
    expect(tk::LPAREN);
    s->cond = parse_expression();
    expect(tk::RPAREN);
    expect(tk::SEMICOLON);
    return s;
}

stmt_ptr parser::parse_for_statement() {
    auto s = std::make_unique<for_stmt>();
    s->loc = consume().loc; // 'for'
    expect(tk::LPAREN);

    scope_guard sg{syms_};

    // Init
    if (check(tk::SEMICOLON)) {
        consume();
    } else if (is_type_start()) {
        auto      ds    = std::make_unique<decl_stmt>();
        decl_spec fdspec = parse_declaration_specifiers();
        do {
            auto     fdi   = parse_declarator(fdspec.base_type);
            auto     vname = fdi.name;
            type_ptr vtype = fdi.type;
            auto vd = std::make_unique<var_decl>();
            vd->name  = vname; vd->type = vtype; vd->storage = fdspec.sc;
            auto vsym = std::make_shared<symbol>();
            vsym->name = vname; vsym->kind = sym_kind::VAR;
            vsym->type = vtype; vsym->storage = fdspec.sc;
            vsym->stack_offset = alloc_local(vtype->size());
            syms_.insert(vsym);
            vd->sym = vsym;
            if (match(tk::EQ))
                vd->init = check(tk::LBRACE) ? parse_initializer(vtype)
                                              : parse_assignment_expression();
            ds->decls.push_back(std::move(vd));
        } while (match(tk::COMMA));
        expect(tk::SEMICOLON);
        s->init = std::move(ds);
    } else {
        auto es = std::make_unique<expr_stmt>();
        es->expr = parse_expression();
        expect(tk::SEMICOLON);
        s->init = std::move(es);
    }

    // Condition
    if (!check(tk::SEMICOLON)) s->cond = parse_expression();
    expect(tk::SEMICOLON);

    // Step
    if (!check(tk::RPAREN)) s->step = parse_expression();
    expect(tk::RPAREN);

    s->body = parse_statement();
    // sg destructs here → pop_scope()
    return s;
}

stmt_ptr parser::parse_return_statement() {
    auto s = std::make_unique<return_stmt>();
    s->loc = consume().loc; // 'return'
    if (!check(tk::SEMICOLON))
        s->value = parse_expression();
    expect(tk::SEMICOLON);
    return s;
}

stmt_ptr parser::parse_static_assert() {
    source_loc loc = peek().loc;
    consume(); // _Static_assert
    expect(tk::LPAREN);
    auto expr = parse_assignment_expression();
    std::string msg = "static assertion failed";
    if (match(tk::COMMA)) {
        if (check(tk::STR_LIT))
            msg = consume().sval;
    }
    expect(tk::RPAREN);
    expect(tk::SEMICOLON);

    auto val = const_expr_evaluator::evaluate(expr.get());
    if (!val)
        diag_.error(loc, "_Static_assert expression is not a constant");
    else if (*val == 0)
        diag_.error(loc, "static assertion failed: %s", msg.c_str());
    auto s = std::make_unique<expr_stmt>();
    s->loc = loc;
    return s;
}

stmt_ptr parser::parse_asm_statement() {
    auto s = std::make_unique<asm_stmt>();
    s->loc = consume().loc; // consume __asm__ / __asm
    expect(tk::LPAREN);
    // Concatenate adjacent string literals (basic form; no constraints).
    while (check(tk::STR_LIT))
        s->text += consume().sval;
    expect(tk::RPAREN);
    expect(tk::SEMICOLON);
    return s;
}

stmt_ptr parser::parse_switch_statement() {
    auto s = std::make_unique<switch_stmt>();
    s->loc = consume().loc;
    expect(tk::LPAREN);
    s->cond = parse_expression();
    expect(tk::RPAREN);
    s->body = parse_statement();
    return s;
}

stmt_ptr parser::parse_label_or_expr_statement() {
    // IDENT ':' -> named label statement (uses second lookahead to avoid consuming).
    if (check(tk::IDENT) && lex_.peek2().kind == tk::COLON) {
        token id = consume();  // consume IDENT
        consume();             // consume ':'
        auto s = std::make_unique<label_stmt>();
        s->loc  = id.loc;
        s->name = id.text;
        // C23: a label may be followed directly by a declaration.
        // Attach an empty statement as the label body; the declaration
        // is handled normally by the enclosing compound-statement loop.
        if (!check(tk::RBRACE) && !check(tk::END_OF_FILE) && !is_type_start())
            s->body = parse_statement();
        // else: body stays nullptr — the outer compound loop picks up the decl.
        return s;
    }

    auto s = std::make_unique<expr_stmt>();
    s->loc = peek().loc;
    s->expr = parse_expression();
    expect(tk::SEMICOLON);
    return s;
}

} // namespace xcc
