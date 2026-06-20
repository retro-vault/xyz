//
// parser_expr.cpp — expression parsing (precedence climbing).
//
// Implements: parse_expression(), parse_assignment_expression(),
//             parse_conditional_expression(), parse_binary_expression(),
//             parse_cast_expression(), parse_unary_expression(),
//             parse_postfix_expression(), parse_primary_expression(),
//             parse_generic_selection(), and operator helpers.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/parser.h"
#include <cctype>
#include <limits>

namespace xcc {

static bool types_compatible(type_ptr a, type_ptr b); // forward decl

// ----- Expressions ---------------------------------------------------

expr_ptr parser::parse_expression() {
    auto e = parse_assignment_expression();
    while (check(tk::COMMA)) {
        auto loc = consume().loc;
        auto right = parse_assignment_expression();
        auto bin = std::make_unique<binary_expr>();
        bin->loc   = loc;
        bin->op    = bin_op::COMMA;
        bin->left  = std::move(e);
        bin->right = std::move(right);
        e = std::move(bin);
    }
    return e;
}

bool parser::is_assign_op(tk k) const {
    switch (k) {
    case tk::EQ: case tk::PLUS_EQ: case tk::MINUS_EQ:
    case tk::STAR_EQ: case tk::SLASH_EQ: case tk::PERCENT_EQ:
    case tk::AMP_EQ: case tk::PIPE_EQ: case tk::CARET_EQ:
    case tk::LSHIFT_EQ: case tk::RSHIFT_EQ:
        return true;
    default: return false;
    }
}

bin_op parser::token_to_assign_op(tk k) const {
    switch (k) {
    case tk::EQ:         return bin_op::ASSIGN;
    case tk::PLUS_EQ:    return bin_op::ADD_ASSIGN;
    case tk::MINUS_EQ:   return bin_op::SUB_ASSIGN;
    case tk::STAR_EQ:    return bin_op::MUL_ASSIGN;
    case tk::SLASH_EQ:   return bin_op::DIV_ASSIGN;
    case tk::PERCENT_EQ: return bin_op::MOD_ASSIGN;
    case tk::AMP_EQ:     return bin_op::AND_ASSIGN;
    case tk::PIPE_EQ:    return bin_op::OR_ASSIGN;
    case tk::CARET_EQ:   return bin_op::XOR_ASSIGN;
    case tk::LSHIFT_EQ:  return bin_op::SHL_ASSIGN;
    case tk::RSHIFT_EQ:  return bin_op::RHS_ASSIGN;
    default:             return bin_op::ASSIGN;
    }
}

expr_ptr parser::parse_assignment_expression() {
    auto lhs = parse_conditional_expression();
    if (is_assign_op(peek().kind)) {
        auto loc = peek().loc;
        bin_op op = token_to_assign_op(consume().kind);
        auto rhs = parse_assignment_expression();
        auto bin = std::make_unique<binary_expr>();
        bin->loc   = loc;
        bin->op    = op;
        bin->left  = std::move(lhs);
        bin->right = std::move(rhs);
        return bin;
    }
    return lhs;
}

expr_ptr parser::parse_conditional_expression() {
    auto e = parse_binary_expression(0);
    if (check(tk::QUESTION)) {
        auto loc = consume().loc;
        auto then_e = parse_expression();
        expect(tk::COLON);
        auto else_e = parse_conditional_expression();
        auto cond = std::make_unique<conditional_expr>();
        cond->loc       = loc;
        cond->cond      = std::move(e);
        cond->then_expr = std::move(then_e);
        cond->else_expr = std::move(else_e);
        // Compute result type (C11 §6.5.15): arithmetic → usual_arith_conv,
        // null-pointer-constant + pointer → pointer type, else use non-null arm.
        {
            type_ptr tt = cond->then_expr ? cond->then_expr->type : nullptr;
            type_ptr et = cond->else_expr ? cond->else_expr->type : nullptr;
            if (tt && et) {
                if (tt->is_arith() && et->is_arith())
                    cond->type = usual_arith_conv(tt->unqual(), et->unqual());
                else if (tt->is_ptr() && et->is_ptr())
                    cond->type = tt; // assume compatible
                else if (tt->is_ptr() && et->is_integer())
                    cond->type = tt; // null pointer constant on else side
                else if (tt->is_integer() && et->is_ptr())
                    cond->type = et; // null pointer constant on then side
                else
                    cond->type = tt ? tt : et;
            } else {
                cond->type = tt ? tt : et;
            }
        }
        return cond;
    }
    return e;
}

int parser::binop_prec(tk k) const {
    switch (k) {
    case tk::PIPE_PIPE:  return 1;
    case tk::AMP_AMP:    return 2;
    case tk::PIPE:       return 3;
    case tk::CARET:      return 4;
    case tk::AMP:        return 5;
    case tk::EQ_EQ:
    case tk::BANG_EQ:    return 6;
    case tk::LT: case tk::GT:
    case tk::LT_EQ: case tk::GT_EQ: return 7;
    case tk::LSHIFT:
    case tk::RSHIFT:     return 8;
    case tk::PLUS:
    case tk::MINUS:      return 9;
    case tk::STAR:
    case tk::SLASH:
    case tk::PERCENT:    return 10;
    default:             return -1;
    }
}

bin_op parser::token_to_binop(tk k) const {
    switch (k) {
    case tk::PLUS:     return bin_op::ADD;
    case tk::MINUS:    return bin_op::SUB;
    case tk::STAR:     return bin_op::MUL;
    case tk::SLASH:    return bin_op::DIV;
    case tk::PERCENT:  return bin_op::MOD;
    case tk::AMP:      return bin_op::AND;
    case tk::PIPE:     return bin_op::OR;
    case tk::CARET:    return bin_op::XOR;
    case tk::LSHIFT:   return bin_op::SHL;
    case tk::RSHIFT:   return bin_op::SHR;
    case tk::EQ_EQ:    return bin_op::EQ;
    case tk::BANG_EQ:  return bin_op::NE;
    case tk::LT:       return bin_op::LT;
    case tk::LT_EQ:    return bin_op::LE;
    case tk::GT:       return bin_op::GT;
    case tk::GT_EQ:    return bin_op::GE;
    case tk::AMP_AMP:  return bin_op::LAND;
    case tk::PIPE_PIPE:return bin_op::LOR;
    default:           return bin_op::ADD;
    }
}

expr_ptr parser::parse_binary_expression(int min_prec) {
    auto lhs = parse_cast_expression();
    while (true) {
        int prec = binop_prec(peek().kind);
        if (prec < min_prec) break;
        auto loc = peek().loc;
        bin_op op = token_to_binop(consume().kind);
        auto rhs = parse_binary_expression(prec + 1);
        auto bin = std::make_unique<binary_expr>();
        bin->loc   = loc;
        bin->op    = op;
        bin->left  = std::move(lhs);
        bin->right = std::move(rhs);
        // Set result type (needed by _Generic and type propagation).
        {
            type_ptr lt = bin->left  ? bin->left->type  : nullptr;
            type_ptr rt = bin->right ? bin->right->type : nullptr;
            if (lt && rt && op != bin_op::COMMA) {
                lt = lt->unqual();
                rt = rt->unqual();
                if (op == bin_op::ADD && lt->is_ptr() && rt->is_integer())
                    bin->type = lt;
                else if (op == bin_op::ADD && lt->is_integer() && rt->is_ptr())
                    bin->type = rt;
                else if (op == bin_op::SUB && lt->is_ptr() && rt->is_integer())
                    bin->type = lt;
                else if (op == bin_op::SUB && lt->is_ptr() && rt->is_ptr())
                    bin->type = type::make_int();
                else
                    bin->type = usual_arith_conv(lt, rt);
            }
        }
        lhs = std::move(bin);
    }
    return lhs;
}

expr_ptr parser::parse_cast_expression() {
    // Try: '(' type-name ')' cast-expression
    if (check(tk::LPAREN)) {
        // Peek at what follows - if it looks like a type, it's a cast
        // We do a speculative lookahead by checking if what's inside is a type name
        // This requires saving/restoring parser state which we don't support yet.
        // Simple heuristic: peek past '(' and see if it's a type keyword
        // (Works for explicit casts; doesn't handle arbitrary typedef names without
        //  proper lookahead - acceptable for initial version)
        token after_paren = peek(); // this is the '('
        // We'll check if the current token is '(' and next could be a type by
        // examining the lexer peek after consuming.
        // For now: parse as unary and fall through (casts via C-style will work
        // when they're obvious keywords).
        (void)after_paren;
    }
    return parse_unary_expression();
}

expr_ptr parser::parse_unary_expression() {
    auto loc = peek().loc;

    if (check(tk::KW___EXTENSION__)) {
        consume(); // __extension__ is a no-op prefix
        return parse_cast_expression();
    }
    if (check(tk::BANG)) {
        consume();
        auto e = std::make_unique<unary_expr>();
        e->loc = loc; e->op = unary_op::NOT;
        e->operand = parse_cast_expression();
        e->type = type::make_int();
        return e;
    }
    if (check(tk::TILDE)) {
        consume();
        auto e = std::make_unique<unary_expr>();
        e->loc = loc; e->op = unary_op::BNOT;
        e->operand = parse_cast_expression();
        if (e->operand && e->operand->type)
            e->type = integer_promote(e->operand->type->unqual());
        return e;
    }
    if (check(tk::MINUS)) {
        consume();
        auto e = std::make_unique<unary_expr>();
        e->loc = loc; e->op = unary_op::NEG;
        e->operand = parse_cast_expression();
        if (e->operand && e->operand->type) {
            type_ptr t = e->operand->type->unqual();
            e->type = t->is_integer() ? integer_promote(t) : t;
        }
        return e;
    }
    if (check(tk::PLUS)) {
        consume();
        auto operand = parse_cast_expression();
        // C11 §6.5.3.3: unary + performs integer promotion on the operand.
        if (operand && operand->type) {
            type_ptr t = operand->type->unqual();
            type_ptr promoted;
            switch (t->kind) {
            case type_kind::BOOL:
            case type_kind::CHAR:
            case type_kind::UCHAR:
            case type_kind::SHORT:
                promoted = type::make_int();  break;
            case type_kind::USHORT:
                promoted = type::make_uint(); break;
            default: break;
            }
            if (promoted) {
                auto cast = std::make_unique<cast_expr>();
                cast->loc         = operand->loc;
                cast->target_type = promoted;
                cast->operand     = std::move(operand);
                cast->type        = promoted;
                return cast;
            }
        }
        return operand;
    }
    if (check(tk::STAR)) {
        consume();
        auto e = std::make_unique<unary_expr>();
        e->loc = loc; e->op = unary_op::DEREF;
        e->operand = parse_cast_expression();
        // *p has the pointee type (if we can determine it now)
        if (e->operand && e->operand->type && e->operand->type->is_ptr())
            e->type = e->operand->type->base;
        e->is_lvalue = true;
        return e;
    }
    if (check(tk::AMP)) {
        consume();
        auto e = std::make_unique<unary_expr>();
        e->loc = loc; e->op = unary_op::ADDR;
        e->operand = parse_cast_expression();
        // &expr has type pointer-to-operand-type
        if (e->operand && e->operand->type)
            e->type = type::make_pointer(e->operand->type->unqual());
        return e;
    }
    if (check(tk::PLUS_PLUS)) {
        consume();
        auto e = std::make_unique<unary_expr>();
        e->loc = loc; e->op = unary_op::PRE_INC;
        e->operand = parse_unary_expression();
        if (e->operand) e->type = e->operand->type;
        return e;
    }
    if (check(tk::MINUS_MINUS)) {
        consume();
        auto e = std::make_unique<unary_expr>();
        e->loc = loc; e->op = unary_op::PRE_DEC;
        e->operand = parse_unary_expression();
        if (e->operand) e->type = e->operand->type;
        return e;
    }
    if (check(tk::KW__ALIGNOF)) {
        consume();
        auto e = std::make_unique<sizeof_expr>();
        e->loc        = loc;
        e->is_alignof = true;
        if (check(tk::LPAREN)) {
            consume();
            if (is_type_start()) {
                e->sizeof_type = parse_type_name();
                expect(tk::RPAREN);
            } else {
                e->sizeof_expr_op = parse_expression();
                expect(tk::RPAREN);
            }
        } else {
            e->sizeof_expr_op = parse_unary_expression();
        }
        if (e->sizeof_expr_op) {
            if (auto *id = dynamic_cast<ident_expr *>(e->sizeof_expr_op.get())) {
                if (id->sym && id->sym->requested_align > 0)
                    e->align_override = id->sym->requested_align;
            }
        }
        e->type = type::make_uint();
        return e;
    }
    if (check(tk::KW_SIZEOF)) {
        consume();
        auto e = std::make_unique<sizeof_expr>();
        e->loc = loc;
        e->type = type::make_uint(); // sizeof result is size_t (unsigned int on Z80)
        if (check(tk::LPAREN)) {
            consume(); // consume '('
            if (is_type_start()) {
                // sizeof(type-name)
                e->sizeof_type = parse_type_name();
                expect(tk::RPAREN);
            } else {
                // sizeof(expr) — parse expression inside parens
                e->sizeof_expr_op = parse_expression();
                expect(tk::RPAREN);
            }
        } else {
            e->sizeof_expr_op = parse_unary_expression();
        }
        return e;
    }

    return parse_postfix_expression();
}

expr_ptr parser::parse_postfix_expression() {
    auto e = parse_primary_expression();
    while (true) {
        auto loc = peek().loc;
        if (check(tk::LBRACKET)) {
            consume();
            auto idx = std::make_unique<index_expr>();
            idx->loc   = loc;
            idx->base  = std::move(e);
            idx->index = parse_expression();
            expect(tk::RBRACKET);
            e = std::move(idx);
        } else if (check(tk::LPAREN)) {
            consume();
            auto call = std::make_unique<call_expr>();
            call->loc    = loc;
            call->callee = std::move(e);
            if (!check(tk::RPAREN)) {
                do {
                    call->args.push_back(parse_assignment_expression());
                } while (match(tk::COMMA));
            }
            expect(tk::RPAREN);
            e = std::move(call);
        } else if (check(tk::DOT)) {
            consume();
            auto mem = std::make_unique<member_expr>();
            mem->loc      = loc;
            mem->object   = std::move(e);
            mem->member   = expect(tk::IDENT).text;
            mem->is_arrow = false;
            // Resolve member type for __typeof__ and type-based operations.
            if (mem->object && mem->object->type) {
                type_ptr st = mem->object->type->unqual();
                if (st && (st->kind == type_kind::STRUCT || st->kind == type_kind::UNION)) {
                    for (auto &f : st->fields)
                        if (f.name == mem->member) { mem->type = f.type; break; }
                }
            }
            e = std::move(mem);
        } else if (check(tk::ARROW)) {
            consume();
            auto mem = std::make_unique<member_expr>();
            mem->loc      = loc;
            mem->object   = std::move(e);
            mem->member   = expect(tk::IDENT).text;
            mem->is_arrow = true;
            // Resolve member type for __typeof__ and type-based operations.
            if (mem->object && mem->object->type) {
                type_ptr st = mem->object->type->unqual();
                if (st && st->is_ptr()) st = st->base ? st->base->unqual() : nullptr;
                if (st && (st->kind == type_kind::STRUCT || st->kind == type_kind::UNION)) {
                    for (auto &f : st->fields)
                        if (f.name == mem->member) { mem->type = f.type; break; }
                }
            }
            e = std::move(mem);
        } else if (check(tk::PLUS_PLUS)) {
            consume();
            auto u = std::make_unique<unary_expr>();
            u->loc = loc; u->op = unary_op::POST_INC;
            u->operand = std::move(e);
            if (u->operand) u->type = u->operand->type;
            e = std::move(u);
        } else if (check(tk::MINUS_MINUS)) {
            consume();
            auto u = std::make_unique<unary_expr>();
            u->loc = loc; u->op = unary_op::POST_DEC;
            u->operand = std::move(e);
            if (u->operand) u->type = u->operand->type;
            e = std::move(u);
        } else {
            break;
        }
    }
    return e;
}

expr_ptr parser::parse_primary_expression() {
    auto loc = peek().loc;

    if (check(tk::INT_LIT)) {
        token t = consume();
        auto e = std::make_unique<int_literal_expr>();
        e->loc   = t.loc;
        e->value = t.ival;
        auto lower_text = t.text;
        for (char &ch : lower_text) ch = (char)std::tolower((unsigned char)ch);
        if (lower_text.size() >= 2 &&
            lower_text.substr(lower_text.size() - 2) == "wb") {
            bool is_unsigned_bitint = lower_text.size() >= 3 &&
                                      lower_text.substr(lower_text.size() - 3) == "uwb";
            int width = 1;
            if (is_unsigned_bitint) {
                unsigned long long u = (unsigned long long)e->value;
                while (width < 64 && u > ((1ULL << width) - 1ULL)) ++width;
            } else {
                width = 2;
                while (width < 64) {
                    long long minv = -(1LL << (width - 1));
                    long long maxv =  (1LL << (width - 1)) - 1LL;
                    if (e->value >= minv && e->value <= maxv) break;
                    ++width;
                }
            }
            e->type = type::make_bitint(width, is_unsigned_bitint);
            return e;
        }
        // Derive type from integer suffix: L/l → long, LL/ll → long long, U/u → unsigned
        bool has_u = false, has_l = false, has_ll = false;
        for (char ch : t.text) {
            char uc = (char)std::toupper((unsigned char)ch);
            if (uc == 'U') has_u = true;
            else if (uc == 'L') { if (has_l) has_ll = true; else has_l = true; }
        }
        if (has_ll)     { e->type = has_u ? type::make_ullong() : type::make_llong(); }
        else if (has_l) {
            if (has_u) {
                // UL: ulong if fits, else ullong (C11 §6.4.4.1)
                unsigned long long v = (unsigned long long)e->value;
                e->type = (v <= 0xFFFFFFFFull) ? type::make_ulong() : type::make_ullong();
            } else {
                // L: long if fits, else llong (decimal); or ulong/ullong for hex/octal
                long long v = e->value;
                bool is_hex_oct = (t.text.size() >= 2 && t.text[0] == '0' &&
                                   (t.text[1]=='x'||t.text[1]=='X'||
                                    (t.text[1]>='0'&&t.text[1]<='7')));
                if (v >= -2147483648LL && v <= 2147483647LL)
                    e->type = type::make_long();
                else if (is_hex_oct && (unsigned long long)v <= 0xFFFFFFFFull)
                    e->type = type::make_ulong();
                else
                    e->type = type::make_llong();
            }
        }
        else if (has_u) {
            // Unsuffixed U: uint if fits, else ulong, else ullong (C11 §6.4.4.1).
            unsigned long long v = (unsigned long long)e->value;
            if      (v <= 0xFFFFu)               e->type = type::make_uint();
            else if (v <= 0xFFFFFFFFull)         e->type = type::make_ulong();
            else                                 e->type = type::make_ullong();
        } else {
            // No suffix: int if fits, else long, else llong (C11 §6.4.4.1).
            long long v = e->value;
            bool is_hex_or_octal = (t.text.size() >= 2 &&
                                    (t.text[0] == '0' && (t.text[1]=='x'||t.text[1]=='X'||
                                     (t.text[1]>='0'&&t.text[1]<='7'))));
            if      (v >= -32768LL      && v <= 32767LL)      e->type = type::make_int();
            else if (is_hex_or_octal &&
                     (unsigned long long)v <= 0xFFFFu)        e->type = type::make_uint();
            else if (v >= -2147483648LL && v <= 2147483647LL) e->type = type::make_long();
            else if (is_hex_or_octal &&
                     (unsigned long long)v <= 0xFFFFFFFFull)  e->type = type::make_ulong();
            else if (v >= (-9223372036854775807LL-1) &&
                     v <= 9223372036854775807LL)              e->type = type::make_llong();
            else                                              e->type = type::make_ullong();
        }
        return e;
    }
    if (check(tk::FLOAT_LIT)) {
        token t = consume();
        auto e = std::make_unique<float_literal_expr>();
        e->loc   = t.loc;
        e->value = t.fval;
        if (!t.text.empty()) {
            char suffix = t.text.back();
            if (suffix == 'f' || suffix == 'F')
                e->type = type::make_float();
            else
                e->type = type::make_double();
        } else {
            e->type = type::make_double();
        }
        return e;
    }
    if (check(tk::CHAR_LIT)) {
        token t = consume();
        auto e = std::make_unique<char_literal_expr>();
        e->loc   = t.loc;
        e->value = t.ival;
        e->type  = (t.char_width == 8) ? type::make_char8t()
                                       : type::make_int(); // ordinary char constant has type int in C
        return e;
    }
    if (check(tk::STR_LIT)) {
        token t = consume();
        auto e = std::make_unique<string_literal_expr>();
        e->loc        = t.loc;
        e->value      = t.sval;
        e->char_width = (int)t.ival;
        // Concatenate adjacent string literals
        while (check(tk::STR_LIT)) {
            e->value += lex_.next().sval;
        }
        e->type = type::make_pointer(type::make_char());
        e->type->is_const = true;
        return e;
    }
    // C23 keyword literals
    if (check(tk::KW_TRUE)) {
        token t = consume();
        auto e = std::make_unique<int_literal_expr>();
        e->loc = t.loc; e->value = 1; e->type = type::make_bool();
        return e;
    }
    if (check(tk::KW_FALSE)) {
        token t = consume();
        auto e = std::make_unique<int_literal_expr>();
        e->loc = t.loc; e->value = 0; e->type = type::make_bool();
        return e;
    }
    if (check(tk::KW_NULLPTR)) {
        token t = consume();
        auto e = std::make_unique<int_literal_expr>();
        e->loc = t.loc; e->value = 0;
        e->type = type::make_pointer(type::make_void()); // void* null pointer constant
        return e;
    }

    if (check(tk::IDENT)) {
        token t = consume();
        if (t.text == "__func__") {
            if (!cur_func_)
                diag_.warning(warning_group::C23_EXTENSIONS, t.loc,
                              "'__func__' used outside a function");
            auto e = std::make_unique<string_literal_expr>();
            e->loc   = t.loc;
            e->value = cur_func_ ? cur_func_->name : "";
            e->type  = type::make_pointer(type::make_char());
            e->type->is_const = true;
            return e;
        }
        if (t.text == "__builtin_expect") {
            // __builtin_expect(expr, hint) — return expr unchanged
            expect(tk::LPAREN);
            auto e = parse_assignment_expression();
            expect(tk::COMMA);
            parse_assignment_expression(); // discard hint
            expect(tk::RPAREN);
            return e;
        }
        if (t.text == "__builtin_inf" || t.text == "__builtin_inff" ||
            t.text == "__builtin_infl") {
            // GCC-compatible floating infinity builtins used by libc headers.
            expect(tk::LPAREN);
            expect(tk::RPAREN);
            auto e = std::make_unique<float_literal_expr>();
            e->loc   = t.loc;
            e->value = std::numeric_limits<double>::infinity();
            e->type  = (t.text == "__builtin_inff") ? type::make_float()
                                                     : type::make_double();
            return e;
        }
        if (t.text == "__builtin_nan" || t.text == "__builtin_nanf" ||
            t.text == "__builtin_nanl") {
            // Payload string is accepted and ignored for now; the target
            // runtime only needs a quiet NaN bit-pattern.
            expect(tk::LPAREN);
            if (!check(tk::RPAREN))
                parse_assignment_expression();
            expect(tk::RPAREN);
            auto e = std::make_unique<float_literal_expr>();
            e->loc   = t.loc;
            e->value = std::numeric_limits<double>::quiet_NaN();
            e->type  = (t.text == "__builtin_nanf") ? type::make_float()
                                                     : type::make_double();
            return e;
        }
        if (t.text == "__builtin_va_start") {
            // __builtin_va_start(ap [, last]) — lower to an assignment-like
            // side effect while keeping the expression type as void.
            expect(tk::LPAREN);
            auto ap = parse_assignment_expression();
            expr_ptr last;
            if (match(tk::COMMA))
                last = parse_assignment_expression();
            expect(tk::RPAREN);

            auto assign = std::make_unique<binary_expr>();
            assign->loc  = t.loc;
            assign->op   = bin_op::ASSIGN;
            assign->left = std::move(ap);

            type_ptr ap_type = assign->left && assign->left->type
                                 ? assign->left->type
                                 : type::make_pointer(type::make_char());

            expr_ptr rhs;
            if (last) {
                auto addr = std::make_unique<unary_expr>();
                addr->loc       = t.loc;
                addr->op        = unary_op::ADDR;
                addr->operand   = std::move(last);
                addr->type      = type::make_pointer(addr->operand && addr->operand->type
                                                     ? addr->operand->type->unqual()
                                                     : type::make_int());
                addr->is_lvalue = false;

                auto char_ptr = std::make_unique<cast_expr>();
                char_ptr->loc         = t.loc;
                char_ptr->target_type = type::make_pointer(type::make_char());
                char_ptr->operand     = std::move(addr);
                char_ptr->type        = char_ptr->target_type;

                auto sz = std::make_unique<int_literal_expr>();
                sz->loc   = t.loc;
                sz->value = (char_ptr->operand && static_cast<unary_expr *>(char_ptr->operand.get())->operand &&
                             static_cast<unary_expr *>(char_ptr->operand.get())->operand->type)
                              ? static_cast<unary_expr *>(char_ptr->operand.get())->operand->type->size()
                              : 0;
                sz->type  = type::make_int();

                auto add = std::make_unique<binary_expr>();
                add->loc   = t.loc;
                add->op    = bin_op::ADD;
                add->left  = std::move(char_ptr);
                add->right = std::move(sz);
                add->type  = type::make_pointer(type::make_char());

                auto cast = std::make_unique<cast_expr>();
                cast->loc         = t.loc;
                cast->target_type = ap_type;
                cast->operand     = std::move(add);
                cast->type        = ap_type;
                rhs = std::move(cast);
            } else {
                auto zero = std::make_unique<int_literal_expr>();
                zero->loc   = t.loc;
                zero->value = 0;
                zero->type  = type::make_int();

                auto cast = std::make_unique<cast_expr>();
                cast->loc         = t.loc;
                cast->target_type = ap_type;
                cast->operand     = std::move(zero);
                cast->type        = ap_type;
                rhs = std::move(cast);
            }

            assign->right = std::move(rhs);
            assign->type  = type::make_void();
            return assign;
        }
        if (t.text == "__builtin_unreachable") {
            // C23 unreachable(): marks a code path as never reached.
            // Emit an inline halt so the Z80 stops if it gets here anyway.
            expect(tk::LPAREN);
            expect(tk::RPAREN);
            // Wrap in a GNU statement expression that emits halt and returns void.
            // We model this as a call to an external __builtin_unreachable symbol
            // that is [[noreturn]]; the backend will omit subsequent dead code.
            auto callee_e = std::make_unique<ident_expr>();
            callee_e->loc  = t.loc;
            callee_e->name = "__builtin_unreachable";
            // Build or retrieve the symbol
            auto sym = syms_.lookup("__builtin_unreachable");
            if (!sym) {
                sym = std::make_shared<symbol>();
                sym->name           = "__builtin_unreachable";
                sym->kind           = sym_kind::FUNC;
                sym->type           = type::make_function(type::make_void(), {}, false);
                sym->is_global      = true;
                sym->attr_noreturn  = true;
                syms_.insert(sym);
            }
            callee_e->sym  = sym;
            callee_e->type = sym->type;
            auto call_e = std::make_unique<call_expr>();
            call_e->loc    = t.loc;
            call_e->callee = std::move(callee_e);
            call_e->type   = type::make_void();
            return call_e;
        }
        auto e = std::make_unique<ident_expr>();
        e->loc  = t.loc;
        e->name = t.text;
        // Resolve symbol
        e->sym = syms_.lookup(t.text);
        if (!e->sym) {
            // C23: implicit function declaration is not permitted.
            // Emit a warning and create a variadic-int placeholder for error recovery.
            diag_.warning(warning_group::IMPLICIT_FUNCTION_DECLARATION, t.loc,
                          "implicit declaration of function '%s'", t.text.c_str());
            auto sym = std::make_shared<symbol>();
            sym->name      = t.text;
            sym->kind      = sym_kind::FUNC;
            sym->type      = type::make_function(type::make_int(), {}, true);
            sym->is_global = true;
            e->sym = sym;
        }
        if (e->sym) e->type = e->sym->type;
        e->is_lvalue = (e->sym && e->sym->kind == sym_kind::VAR);
        return e;
    }
    if (check(tk::KW__GENERIC)) {
        return parse_generic_selection();
    }
    if (check(tk::LPAREN)) {
        consume();
        // GNU statement expression: ({ stmts... })
        if (check(tk::LBRACE)) {
            auto se  = std::make_unique<stmt_expr>();
            se->loc  = loc;
            se->body = parse_compound_statement();
            // Infer type from last expression statement.
            if (auto *cs = dynamic_cast<compound_stmt*>(se->body.get())) {
                if (!cs->body.empty()) {
                    if (auto *es = dynamic_cast<expr_stmt*>(cs->body.back().get())) {
                        if (es->expr && es->expr->type)
                            se->type = es->expr->type;
                    }
                }
            }
            if (!se->type) se->type = type::make_void();
            expect(tk::RPAREN);
            return se;
        }
        // Check for cast or compound literal: '(' [storage-class] type-name ')' ...
        // C23 §6.5.2.5: compound literals may carry a storage-class specifier.
        // __attribute__ can precede the type name in GCC-style casts.
        if (is_type_start() || check(tk::KW___ATTRIBUTE__)) {
            decl_spec ds = parse_declaration_specifiers();
            type_ptr tname = parse_abstract_declarator(ds.base_type);
            if (!tname) tname = ds.base_type;
            expect(tk::RPAREN);
            // Compound literal: (type){ initializer }
            if (check(tk::LBRACE)) {
                auto cl   = std::make_unique<compound_literal_expr>();
                cl->loc   = loc;
                cl->type  = tname;
                bool is_static_cl = (ds.sc == storage_class::STATIC ||
                                     ds.sc == storage_class::EXTERN);
                if (is_static_cl) {
                    // Static compound literal: treat as a named static variable.
                    std::string lname = "__sclit" + std::to_string(clit_counter_++);
                    auto sym        = std::make_shared<symbol>();
                    sym->name       = lname;
                    sym->kind       = sym_kind::VAR;
                    sym->type       = tname;
                    sym->is_global  = true;
                    sym->storage    = storage_class::STATIC;
                    sym->asm_name   = "_" + lname;
                    syms_.insert(sym);
                    cl->sym = sym;
                } else if (cur_func_) {
                    auto sym        = std::make_shared<symbol>();
                    sym->name       = "__clit" + std::to_string(clit_counter_++);
                    sym->kind       = sym_kind::VAR;
                    sym->type       = tname;
                    sym->stack_offset = alloc_local(tname->size());
                    syms_.insert(sym);
                    cl->sym = sym;
                }
                cl->init     = parse_initializer(tname);
                complete_unsized_array_from_initializer(tname, cl->init);
                cl->is_lvalue = true;
                return cl;
            }
            // Regular cast — but only if no storage class (storage on a cast is invalid)
            auto cast         = std::make_unique<cast_expr>();
            cast->loc         = loc;
            cast->target_type = tname;
            cast->operand     = parse_cast_expression();
            cast->type        = tname;
            return cast;
        }
        auto e = parse_expression();
        expect(tk::RPAREN);
        e->loc = loc;
        return e;
    }

    // __builtin_types_compatible_p(type1, type2) — GNU extension; returns 1 if compatible.
    if (check(tk::KW___TYPES_COMPAT_P)) {
        consume();
        expect(tk::LPAREN);
        type_ptr t1 = parse_type_name();
        expect(tk::COMMA);
        type_ptr t2 = parse_type_name();
        expect(tk::RPAREN);
        auto e = std::make_unique<int_literal_expr>();
        e->loc   = loc;
        e->type  = type::make_int();
        e->value = types_compatible(t1 ? t1->unqual() : nullptr,
                                    t2 ? t2->unqual() : nullptr) ? 1 : 0;
        return e;
    }

    // __builtin_bit_cast(type, expr) — C23; reinterpret bits of expr as type.
    // Implemented as a regular cast for same-size integer types.
    if (check(tk::KW___BIT_CAST)) {
        consume();
        expect(tk::LPAREN);
        type_ptr tgt = parse_type_name();
        expect(tk::COMMA);
        auto operand = parse_assignment_expression();
        expect(tk::RPAREN);
        auto cast         = std::make_unique<cast_expr>();
        cast->loc         = loc;
        cast->target_type = tgt;
        cast->operand     = std::move(operand);
        cast->type        = tgt;
        return cast;
    }

    error(peek(), "expected expression");
    consume();
    // Return dummy integer to allow parsing to continue
    auto e = std::make_unique<int_literal_expr>();
    e->loc   = loc;
    e->value = 0;
    e->type  = type::make_int();
    return e;
}

// ----- _Generic type matching ----------------------------------------

static type_ptr generic_match_type(type_ptr t, bool decay) {
    if (!t)
        return nullptr;
    t = t->unqual();
    if (!t)
        return nullptr;
    if (decay && t->kind == type_kind::ARRAY && t->base)
        return type::make_pointer(t->base);
    if (decay && t->is_func())
        return type::make_pointer(t);
    return t;
}

static bool types_compatible_impl(type_ptr a, type_ptr b, bool decay) {
    a = generic_match_type(a, decay);
    b = generic_match_type(b, decay);
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    if (a->kind == type_kind::POINTER)
        return types_compatible_impl(a->base, b->base, false);
    if (a->kind == type_kind::ARRAY)
        return a->array_size == b->array_size &&
               types_compatible_impl(a->base, b->base, false);
    if (a->kind == type_kind::FUNCTION) {
        if (!types_compatible_impl(a->ret, b->ret, false) ||
            a->params.size() != b->params.size() ||
            a->variadic != b->variadic)
            return false;
        for (size_t i = 0; i < a->params.size(); ++i)
            if (!types_compatible_impl(a->params[i], b->params[i], false))
                return false;
        return true;
    }
    if (a->kind == type_kind::STRUCT || a->kind == type_kind::UNION)
        return a.get() == b.get() || (!a->tag.empty() && a->tag == b->tag);
    return true; // scalar: kind equality is sufficient
}

static bool types_compatible(type_ptr a, type_ptr b) {
    return types_compatible_impl(a, b, true);
}

expr_ptr parser::parse_generic_selection() {
    auto loc = peek().loc;
    consume(); // _Generic
    expect(tk::LPAREN);

    auto ctrl   = parse_assignment_expression();
    type_ptr ct = generic_match_type(ctrl ? ctrl->type : nullptr, true);

    expect(tk::COMMA);

    expr_ptr result;
    bool matched = false;

    while (!check(tk::RPAREN) && !check(tk::END_OF_FILE)) {
        if (check(tk::KW_DEFAULT)) {
            consume();
            expect(tk::COLON);
            auto e = parse_assignment_expression();
            if (!matched && !result) result = std::move(e); // default is fallback
        } else {
            type_ptr assoc = parse_type_name();
            expect(tk::COLON);
            auto e = parse_assignment_expression();
            if (!matched && types_compatible(ct ? ct->unqual() : nullptr,
                                             assoc ? assoc->unqual() : nullptr)) {
                result  = std::move(e);
                matched = true;
            }
        }
        if (!match(tk::COMMA)) break;
    }
    expect(tk::RPAREN);

    if (!result) {
        error("_Generic: no matching association");
        auto e = std::make_unique<int_literal_expr>();
        e->loc = loc; e->value = 0; e->type = type::make_int();
        return e;
    }
    if (!result->type) result->type = type::make_int();
    result->loc = loc;
    return result;
}

} // namespace xcc
