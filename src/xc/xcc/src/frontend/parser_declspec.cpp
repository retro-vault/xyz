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
    storage_class sc           = storage_class::NONE;
    bool          is_inline    = false;
    bool          is_tls       = false;
    bool          is_constexpr = false;
    int           requested_align = 0;
    attr_list     local_attrs;

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

        // C23 constexpr — compile-time constant; implies const + static/auto storage
        if (k == tk::KW_CONSTEXPR) { is_constexpr = true; is_const = true; consume(); continue; }

        // _Alignas(N) / alignas(N) — record the requested alignment.
        if (k == tk::KW__ALIGNAS) {
            consume();
            expect(tk::LPAREN);
            if (is_type_start()) {
                auto at = parse_type_name();
                if (at) requested_align = std::max(requested_align, at->align());
            } else {
                auto ae = parse_assignment_expression();
                if (auto av = const_expr_evaluator::evaluate(ae.get()))
                    requested_align = std::max(requested_align, (int)*av);
            }
            expect(tk::RPAREN);
            continue;
        }

        // Function specifiers
        if (k == tk::KW_INLINE)     { is_inline = true; consume(); continue; }
        if (k == tk::KW__NORETURN)  { consume(); continue; }

        // Qualifiers
        if (k == tk::KW_CONST)    { is_const    = true; consume(); continue; }
        if (k == tk::KW_VOLATILE) { is_volatile = true; consume(); continue; }
        if (k == tk::KW_RESTRICT) { is_restrict = true; consume(); continue; }
        if (k == tk::KW__ATOMIC) {
            consume();
            // _Atomic(type-name) form — parse and strip the qualifier (Z80 uses DI/EI stubs)
            if (check(tk::LPAREN) && !explicit_type) {
                expect(tk::LPAREN);
                if (is_type_start()) {
                    decl_spec ads = parse_declaration_specifiers();
                    explicit_type = ads.base_type ? ads.base_type->unqual() : nullptr;
                    parse_abstract_declarator(explicit_type ? explicit_type : type::make_int());
                }
                expect(tk::RPAREN);
            }
            continue;
        }

        // type specifiers
        if (k == tk::KW_VOID)     { has_void   = true; consume(); continue; }
        if (k == tk::KW__BOOL)    { has_bool   = true; consume(); continue; }
        if (k == tk::KW_BOOL)     { has_bool   = true; consume(); continue; } // C23 bool
        if (k == tk::KW_CHAR8_T && !explicit_type) {
            consume(); explicit_type = type::make_char8t(); continue;
        }
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
                if (!tag.empty())
                    stype = syms_.lookup_tag(tag);
                if (!stype ||
                    (is_union && stype->kind != type_kind::UNION) ||
                    (!is_union && stype->kind != type_kind::STRUCT)) {
                    stype = is_union ? type::make_union(tag) : type::make_struct(tag);
                    if (!tag.empty()) syms_.insert_tag(tag, stype);
                } else {
                    // Complete the existing forward declaration in place so
                    // typedefs and self-references keep pointing at this type.
                    stype->fields.clear();
                    stype->complete = false;
                }
                parse_struct_body(stype, is_union);
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

        // Enum — optionally with C23 underlying type: enum E : unsigned char { ... }
        if (k == tk::KW_ENUM) {
            consume();
            std::string tag;
            if (peek().kind == tk::IDENT) tag = consume().text; // optional tag
            // C23: optional underlying-type specifier after ':'
            type_ptr enum_base = type::make_int(); // default: int
            if (check(tk::COLON)) {
                consume();
                decl_spec uds = parse_declaration_specifiers();
                if (uds.base_type) enum_base = uds.base_type->unqual();
            }
            if (peek().kind == tk::LBRACE) {
                parse_enum_body();
                if (!tag.empty()) syms_.insert_tag(tag, enum_base);
            } else if (!tag.empty()) {
                if (auto existing = syms_.lookup_tag(tag))
                    enum_base = existing;
            }
            explicit_type = enum_base;
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

        // __typeof__(type-or-expr) — GNU extension, and C23 typeof
        if ((k == tk::KW___TYPEOF__ || k == tk::KW_TYPEOF_UNQUAL) && !explicit_type) {
            bool strip_quals = (k == tk::KW_TYPEOF_UNQUAL);
            consume();
            expect(tk::LPAREN);
            if (is_type_start()) {
                decl_spec ds = parse_declaration_specifiers();
                type_ptr t = parse_abstract_declarator(ds.base_type);
                explicit_type = t ? t : ds.base_type;
            } else {
                auto e = parse_expression();
                if (e && e->type) explicit_type = e->type;
                else              explicit_type = type::make_int();
            }
            if (strip_quals && explicit_type) explicit_type = explicit_type->unqual();
            expect(tk::RPAREN);
            continue;
        }

        // _BitInt(N) — C23 bit-precise integer
        if (k == tk::KW__BITINT && !explicit_type) {
            consume();
            expect(tk::LPAREN);
            auto we = parse_assignment_expression();
            auto wv = const_expr_evaluator::evaluate(we.get());
            int width = wv ? (int)*wv : 0;
            expect(tk::RPAREN);
            if (width < 1) {
                error("_BitInt width must be at least 1");
                width = 1;
            }
            if (width > 64) {
                // C23 allows arbitrary widths; xcc caps at 64 on Z80.
                diag_.warning(warning_group::BITINT_WIDTH, peek().loc,
                    "_BitInt(%d) exceeds xcc maximum (64); capped at 64", width);
                width = 64;
            }
            explicit_type = type::make_bitint(width, has_unsigned);
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

    // C23 auto deduction: 'auto' with no type specifier = deduced type.
    bool is_deduced = (!explicit_type && !has_void && !has_bool && !has_char &&
                       !has_int && !has_float && !has_double && !has_complex &&
                       !has_short && !has_long && !has_llong && !has_unsigned &&
                       sc == storage_class::AUTO);
    // If deducing, reset storage class to NONE (auto deduction ≠ auto storage).
    if (is_deduced) sc = storage_class::NONE;

    if (!is_deduced) {
        bool preserves_tag_identity =
            explicit_type &&
            (base->kind == type_kind::STRUCT ||
             base->kind == type_kind::UNION ||
             base->kind == type_kind::ENUM) &&
            !is_const && !is_volatile && !is_restrict;
        if (!preserves_tag_identity) {
            base = base->unqual();
            base->is_const    = is_const;
            base->is_volatile = is_volatile;
            base->is_restrict = is_restrict;
        }
    }

    decl_spec ds;
    ds.base_type   = is_deduced ? nullptr : base;
    ds.sc          = sc;
    ds.is_inline   = is_inline;
    ds.is_tls      = is_tls;
    ds.is_constexpr= is_constexpr;
    ds.is_deduced  = is_deduced;
    ds.align_req   = requested_align;
    ds.attrs       = std::move(local_attrs);
    return ds;
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
            apply_call_abi_attrs_to_type(ftype, fds.attrs);

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
