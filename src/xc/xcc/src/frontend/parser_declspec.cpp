//
// parser_declspec.cpp — declaration-specifier and aggregate-body parsing.
//
// Implements: parse_declaration_specifiers(), parse_struct_body(),
//             parse_enum_body().
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/parser.h"
#include "frontend/const_eval.h"

namespace xcc {

// ----- parse_declaration_specifiers ----------------------------------
// Parses a sequence of: storage-class-specifier | type-specifier | type-qualifier
// Returns the combined base type.

decl_spec parser::parse_declaration_specifiers() {
    storage_class sc        = storage_class::NONE;
    bool          is_inline = false;
    bool          is_tls    = false;
    attr_list     local_attrs; // accumulates [[...]] found inside the specifier sequence

    bool has_unsigned = false;
    bool has_short    = false;
    bool has_long     = false;
    bool has_llong    = false;
    bool has_int      = false;
    bool has_char     = false;
    bool has_float    = false;
    bool has_double   = false;
    bool has_void     = false;
    bool has_bool     = false;
    bool has_complex  = false;

    bool is_const    = false;
    bool is_volatile = false;
    bool is_restrict = false;

    type_ptr explicit_type; // for struct/union/enum

    while (true) {
        tk k = peek().kind;

        // Storage class
        if (k == tk::KW_AUTO)     { sc = storage_class::AUTO;     consume(); continue; }
        if (k == tk::KW_EXTERN)   { sc = storage_class::EXTERN;   consume(); continue; }
        if (k == tk::KW_REGISTER) { sc = storage_class::REGISTER; consume(); continue; }
        if (k == tk::KW_STATIC)   { sc = storage_class::STATIC;   consume(); continue; }
        if (k == tk::KW_TYPEDEF)  { sc = storage_class::TYPEDEF;  consume(); continue; }
        if (k == tk::KW__THREAD_LOCAL) { is_tls = true;             consume(); continue; }

        // GNU extension: __attribute__((...)) — parse and ignore
        if (k == tk::KW___ATTRIBUTE__) { skip_attribute(); continue; }

        // C23 [[attributes]] — parse and accumulate into local_attrs
        if (k == tk::LATTR) {
            auto more = parse_attr_list();
            for (auto &a : more) local_attrs.push_back(std::move(a));
            continue;
        }

        // Function specifiers
        if (k == tk::KW_INLINE)     { is_inline = true; consume(); continue; }
        if (k == tk::KW__NORETURN)  { consume(); continue; }

        // Qualifiers
        if (k == tk::KW_CONST)    { is_const    = true; consume(); continue; }
        if (k == tk::KW_VOLATILE) { is_volatile = true; consume(); continue; }
        if (k == tk::KW_RESTRICT) { is_restrict = true; consume(); continue; }
        if (k == tk::KW__ATOMIC)  { consume(); continue; }

        // type specifiers
        if (k == tk::KW_VOID)     { has_void   = true; consume(); continue; }
        if (k == tk::KW__BOOL)    { has_bool   = true; consume(); continue; }
        if (k == tk::KW_CHAR)     { has_char   = true; consume(); continue; }
        if (k == tk::KW_INT)      { has_int    = true; consume(); continue; }
        if (k == tk::KW_FLOAT)     { has_float   = true; consume(); continue; }
        if (k == tk::KW_DOUBLE)    { has_double  = true; consume(); continue; }
        if (k == tk::KW__COMPLEX)  { has_complex = true; consume(); continue; }
        if (k == tk::KW__IMAGINARY){ has_complex = true; consume(); continue; }
        if (k == tk::KW_SIGNED)   { consume(); continue; }
        if (k == tk::KW_UNSIGNED) { has_unsigned = true; consume(); continue; }
        if (k == tk::KW_SHORT)    { has_short  = true; consume(); continue; }
        if (k == tk::KW_LONG) {
            consume();
            if (has_long) has_llong = true;
            else          has_long  = true;
            continue;
        }

        // Struct / union
        if (k == tk::KW_STRUCT || k == tk::KW_UNION) {
            bool is_union = (k == tk::KW_UNION);
            consume();
            // Allow __attribute__ between struct/union keyword and tag
            while (peek().kind == tk::KW___ATTRIBUTE__) skip_attribute();
            std::string tag;
            if (peek().kind == tk::IDENT) tag = consume().text;
            // Allow __attribute__ between tag and body
            while (peek().kind == tk::KW___ATTRIBUTE__) skip_attribute();

            type_ptr stype;
            if (peek().kind == tk::LBRACE) {
                stype = is_union ? type::make_union(tag) : type::make_struct(tag);
                parse_struct_body(stype, is_union);
                if (!tag.empty()) syms_.insert_tag(tag, stype);
            } else if (!tag.empty()) {
                stype = syms_.lookup_tag(tag);
                if (!stype) {
                    stype = is_union ? type::make_union(tag) : type::make_struct(tag);
                    syms_.insert_tag(tag, stype);
                }
            } else {
                error("expected tag or '{' after struct/union");
                stype = type::make_struct();
            }
            explicit_type = stype;
            continue;
        }

        // Enum
        if (k == tk::KW_ENUM) {
            consume();
            if (peek().kind == tk::IDENT) consume(); // optional tag
            if (peek().kind == tk::LBRACE) {
                parse_enum_body();
            }
            explicit_type = type::make_int(); // enum treated as int
            continue;
        }

        // Typedef names (identifiers that resolve to types)
        if (k == tk::IDENT && !explicit_type) {
            auto sym = syms_.lookup(peek().text);
            if (sym && sym->kind == sym_kind::TYPE) {
                consume();
                explicit_type = sym->type;
                continue;
            }
        }

        // __typeof__(type-or-expr) — GNU extension
        if (k == tk::KW___TYPEOF__ && !explicit_type) {
            consume(); // consume __typeof__
            expect(tk::LPAREN);
            // Try to parse as a type first; fall back to expression.
            if (is_type_start()) {
                decl_spec ds = parse_declaration_specifiers();
                type_ptr t = parse_abstract_declarator(ds.base_type);
                explicit_type = t ? t : ds.base_type;
            } else {
                auto e = parse_expression();
                if (e && e->type) explicit_type = e->type->unqual();
                else              explicit_type = type::make_int();
            }
            expect(tk::RPAREN);
            continue;
        }

        break;
    }

    // Compose the base type from the specifier combination
    type_ptr base;
    if (explicit_type) {
        base = explicit_type;
    } else if (has_void) {
        base = type::make_void();
    } else if (has_bool) {
        base = type::make_bool();
    } else if (has_char) {
        base = has_unsigned ? type::make_uchar() : type::make_char();
    } else if (has_complex) {
        base = type::make_complex(); // float _Complex or double _Complex → same 8-byte type
    } else if (has_float) {
        base = type::make_float();
    } else if (has_double) {
        base = type::make_double();
    } else if (has_llong) {
        base = has_unsigned ? type::make_ullong() : type::make_llong();
    } else if (has_long) {
        base = has_unsigned ? type::make_ulong() : type::make_long();
    } else if (has_short) {
        base = has_unsigned ? type::make_ushort() : type::make_short();
    } else if (has_unsigned && !has_int && !has_char && !has_short && !has_long) {
        base = type::make_uint(); // bare 'unsigned' = unsigned int
    } else {
        base = has_unsigned ? type::make_uint() : type::make_int();
    }

    base = base->unqual();
    base->is_const    = is_const;
    base->is_volatile = is_volatile;
    base->is_restrict = is_restrict;
    return decl_spec{base, sc, is_inline, is_tls, std::move(local_attrs)};
}

// ----- parse_struct_body ---------------------------------------------

void parser::parse_struct_body(type_ptr stype, bool is_union) {
    consume(); // consume '{'

    record_layout layout;

    auto promote_anon = [&](type_ptr anon) {
        layout.flush(is_union);
        for (auto &inner : anon->fields) {
            struct_field promoted = inner;
            if (!is_union) promoted.offset += layout.offset;
            stype->fields.push_back(promoted);
        }
        if (!is_union) layout.offset += anon->size();
        else if (anon->size() > layout.offset) layout.offset = anon->size();
    };

    while (!check(tk::RBRACE) && !check(tk::END_OF_FILE)) {
        // _Static_assert inside struct/union body (C11).
        if (check(tk::KW__STATIC_ASSERT)) { parse_static_assert(); continue; }

        decl_spec fds   = parse_declaration_specifiers();
        type_ptr  fbase = fds.base_type;

        // Anonymous struct/union with no declarator: promote fields directly.
        if (check(tk::SEMICOLON) &&
            (fbase->kind == type_kind::STRUCT || fbase->kind == type_kind::UNION)) {
            promote_anon(fbase);
            expect(tk::SEMICOLON);
            continue;
        }

        do {
            auto     fdi   = parse_declarator(fbase);
            auto     fname = fdi.name;
            type_ptr ftype = fdi.type;

            // Unnamed embedded struct/union (anonymous member).
            if (fname.empty() && (ftype->kind == type_kind::STRUCT ||
                                   ftype->kind == type_kind::UNION)) {
                promote_anon(ftype);
                continue;
            }

            struct_field field;
            field.name = fname;
            field.type = ftype;

            if (check(tk::COLON)) {
                consume();
                auto we    = parse_assignment_expression();
                auto wv    = const_expr_evaluator::evaluate(we.get());
                int  width = wv ? (int)*wv : 0;

                if (width == 0 && fname.empty()) { layout.pad_bitfield(is_union); continue; }
                layout.place_bitfield(field, ftype, width, is_union);
            } else {
                layout.place_field(field, ftype, is_union);
            }
            stype->fields.push_back(field);
        } while (match(tk::COMMA));
        expect(tk::SEMICOLON);
    }

    layout.flush(is_union);
    expect(tk::RBRACE);
    stype->complete = true;
}

// ----- parse_enum_body -----------------------------------------------

void parser::parse_enum_body() {
    consume(); // consume '{'
    int64_t val = 0;
    while (!check(tk::RBRACE) && !check(tk::END_OF_FILE)) {
        std::string ename = expect(tk::IDENT).text;
        if (match(tk::EQ)) {
            auto e = parse_assignment_expression();
            auto v = const_expr_evaluator::evaluate(e.get());
            if (v) val = *v;
        }
        auto sym       = std::make_shared<symbol>();
        sym->name      = ename;
        sym->kind      = sym_kind::ENUM_CONST;
        sym->type      = type::make_int();
        sym->enum_val  = val++;
        syms_.insert(sym);
        match(tk::COMMA);
    }
    expect(tk::RBRACE);
}

} // namespace xcc
