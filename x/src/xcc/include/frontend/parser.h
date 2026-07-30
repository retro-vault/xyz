//
// parser.h — recursive-descent C11 parser for the xcc compiler.
//
// The parser reads tokens from a lexer and produces a translation_unit
// AST.  It performs name resolution (typedef lookup, struct tag lookup,
// enumerator lookup) during parsing using a symbol_table, which means
// there is no separate semantic analysis pass for name resolution.
//
// The parser follows the C11 grammar with a single-token lookahead.
// Operator precedence is handled by Pratt-style precedence climbing in
// parse_binary_expression().
//
// Stack frame layout for the Z80 (IX frame pointer, xcc ABI):
//   parameters : IX+4, IX+4+sizeof(p0), ...   (positive offsets)
//   locals     : IX-2, IX-4, ...              (negative offsets)
// local_offset_ and param_offset_ track the next available slot.
//
// Error recovery: on a syntax error the parser emits a diagnostic and
// tries to resync to the next semicolon before continuing.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "lexer.h"
#include "ast.h"
#include "symtab.h"
#include "const_eval.h"
#include "frontend/diag.h"
#include <deque>
#include <memory>
#include <functional>

namespace xcc {

// ----- decl_spec -----------------------------------------------------
//
// Bundles the output of parse_declaration_specifiers() into a single
// value so callers use named fields instead of output parameters.

struct decl_spec {
    type_ptr      base_type;
    storage_class sc           = storage_class::NONE;
    bool          is_inline    = false;
    bool          is_tls       = false;
    bool          is_constexpr = false; // C23 constexpr: object with constant initializer
    bool          is_deduced   = false; // C23 auto: type inferred from initializer
    int           align_req    = 0;     // requested _Alignas/alignas byte alignment
    attr_list     attrs;
};

// ----- declarator_info -----------------------------------------------
//
// Bundles the output of parse_declarator() together with the parameter
// list that may accompany a function declarator.  Replaces the
// last_params_ / last_variadic_ side-channel members.

struct declarator_info {
    std::string                              name;
    type_ptr                                 type;
    std::vector<std::unique_ptr<param_decl>> params;
    bool                                     variadic = false;
};

// ----- record_layout -------------------------------------------------
//
// Tracks byte-offset and bit-field state while building a struct or union
// type.  All mutation goes through the named methods so the layout
// arithmetic is in one place rather than scattered across parse_struct_body.

struct record_layout {
    int offset   = 0;  // next byte offset (= total struct byte-size so far)
    int bf_start = 0;  // byte offset where the current bit-field unit starts
    int bf_used  = 0;  // bits consumed in the current bit-field unit
    int bf_unit  = 0;  // current bit-field unit size in bytes (0 = none active)

    // Close the active bit-field unit and advance offset.
    void flush(bool is_union) {
        if (bf_unit && !is_union) offset = bf_start + bf_unit;
        bf_unit = bf_used = 0;
    }

    // Assign layout for a zero-width bit-field (padding, forces new unit).
    void pad_bitfield(bool is_union) { flush(is_union); }

    // Assign layout for a regular (non-bit-field) field; sets f.offset.
    void place_field(struct_field &f, type_ptr ftype, bool is_union) {
        flush(is_union);
        f.offset = is_union ? 0 : offset;
        if (!is_union) offset += ftype->size();
        else if (ftype->size() > offset) offset = ftype->size();
    }

    // Assign layout for a bit-field; sets f.offset, f.bit_offset, f.bit_width.
    void place_bitfield(struct_field &f, type_ptr ftype, int width, bool is_union) {
        int unit_bytes = ftype->size() ? ftype->size() : 2;
        if (bf_unit == 0 || bf_unit != unit_bytes ||
            bf_used + width > bf_unit * 8) {
            if (bf_unit && !is_union) offset = bf_start + bf_unit;
            bf_start = is_union ? 0 : offset;
            bf_used  = 0;
            bf_unit  = unit_bytes;
        }
        f.offset     = is_union ? 0 : bf_start;
        f.bit_offset = bf_used;
        f.bit_width  = width;
        bf_used += width;
        if (is_union && bf_unit > offset) offset = bf_unit;
    }
};

// ----- stack_layout --------------------------------------------------
//
// Tracks the IX-relative offset budget for one function's locals and
// parameters.  Owned by the parser and reset for each function body.
//   locals: grow downward  (IX-2, IX-4, ...)
//   params: grow upward   (IX+0, IX+2, ...; real IX offset = value + 4)

struct stack_layout {
    int local_offset = 0;
    int param_offset = 0;

    void reset() { local_offset = 0; param_offset = 0; }

    // Allocate size bytes for a local; returns the (negative) IX offset.
    int alloc_local(int size) {
        local_offset -= size;
        return local_offset;
    }

    // Allocate size bytes for a param; returns the 0-based offset.
    // The real IX offset is returned_value + 4 (applied in z80gen).
    // Z80 push is always 2 bytes, so each param occupies at least 2 bytes.
    int alloc_param(int size) {
        int off = param_offset;
        if (size < 2) size = 2;
        param_offset += size;
        return off;
    }
};

class parser {
public:
    //
    // Construct a parser that reads tokens from lex.
    // The lexer must outlive the parser.
    //
    explicit parser(lexer &lex);

    //
    // Parse the entire translation unit and return the AST root.
    // Returns a non-null translation_unit regardless of error count;
    // check error_count() before proceeding to IR generation.
    //
    std::unique_ptr<translation_unit> parse();

    //
    // Return the number of parse errors encountered.
    //
    int error_count() const { return diag_.error_count(); }

private:
    lexer        &lex_;
    diag_engine  &diag_;
    symbol_table  syms_;
    int           temp_label_ = 0;
    func_decl    *cur_func_   = nullptr;
    stack_layout  frame_;

    // ----- token helpers ---------------------------------------------
    const token &peek();
    token        consume();
    token        expect(tk k);
    bool         check(tk k) const;
    bool         match(tk k);

    // ----- error recovery --------------------------------------------
    void error(const token &t, const char *msg);
    void error(const char *msg);
    void sync_to_semicolon();

    // ----- type parsing ----------------------------------------------
    decl_spec       parse_declaration_specifiers();
    declarator_info parse_declarator(type_ptr base);
    type_ptr        parse_abstract_declarator(type_ptr base);
    type_ptr parse_pointer(type_ptr base);
    type_ptr parse_direct_declarator_suffix(type_ptr base,
                                            std::string &name);
    type_ptr parse_type_name();

    // ----- declaration parsing ---------------------------------------
    decl_ptr parse_external_declaration();
    decl_ptr parse_function_definition(
        type_ptr ret, std::string name, storage_class sc,
        std::vector<std::unique_ptr<param_decl>> params,
        bool variadic, bool is_inline, source_loc loc, attr_list attrs = {},
        call_abi abi = call_abi::DEFAULT);
    std::vector<decl_ptr> parse_declaration(
        bool allow_function_def = false);
    std::unique_ptr<var_decl> parse_var_decl(
        type_ptr base_type, std::string name,
        storage_class sc, source_loc loc);
    std::vector<std::unique_ptr<param_decl>>
        parse_param_list(bool &variadic);

    // ----- statement parsing -----------------------------------------
    stmt_ptr parse_statement();
    stmt_ptr parse_compound_statement();
    stmt_ptr parse_if_statement();
    stmt_ptr parse_while_statement();
    stmt_ptr parse_do_while_statement();
    stmt_ptr parse_for_statement();
    stmt_ptr parse_return_statement();
    stmt_ptr parse_switch_statement();
    stmt_ptr parse_label_or_expr_statement();
    stmt_ptr parse_static_assert();
    stmt_ptr parse_asm_statement();

    // ----- initializer parsing ---------------------------------------
    expr_ptr parse_initializer(type_ptr type);
    void complete_unsized_char_array_from_string(type_ptr t,
                                                 const expr_ptr &init);
    void complete_unsized_array_from_initializer(type_ptr t,
                                                 const expr_ptr &init);

    // ----- expression parsing (precedence climbing) ------------------
    expr_ptr parse_expression();
    expr_ptr parse_assignment_expression();
    expr_ptr parse_conditional_expression();
    expr_ptr parse_binary_expression(int min_prec);
    expr_ptr parse_cast_expression();
    expr_ptr parse_unary_expression();
    expr_ptr parse_postfix_expression();
    expr_ptr parse_primary_expression();

    // ----- aggregate and enum parsers --------------------------------

    // Parse the body of a struct or union definition { field ; ... }.
    // Updates stype->fields and marks stype->complete = true.
    void parse_struct_body(type_ptr stype, bool is_union);

    // Parse the body of an enum definition { IDENT [= expr] , ... }.
    // Inserts enumerator symbols into the current scope.
    type_ptr parse_enum_body();

    // ----- helpers ---------------------------------------------------

    //
    // Skip a GNU __attribute__((...)) clause if present.
    // Handles arbitrary nesting of parentheses inside the attribute.
    //
    void skip_attribute();

    //
    // Parse one or more consecutive C23 [[attr]] sequences and return
    // the accumulated list.  The first token must be LATTR; if the
    // current token is not LATTR the method returns an empty list
    // without consuming anything.
    //
    attr_list parse_attr_list();

    //
    // Reject one legacy z88dk/SDCC calling-convention keyword spelling
    // such as __z88dk_callee or __z88dk_fastcall with a fix-it style
    // diagnostic that points to the C23 [[...]] attribute form.
    //
    bool reject_legacy_callconv_keyword();

    //
    // Apply ABI-bearing attributes such as [[sdcc::sdccall(N)]] to the
    // nearest function type in a declarator.  This is what preserves the
    // calling convention for function pointer typedefs and prototypes.
    //
    void apply_call_abi_attrs_to_type(type_ptr t, const attr_list &attrs);

    //
    // Return true if the current token can begin a type-specifier,
    // type-qualifier, storage-class specifier, or a typedef name.
    // Consults the symbol table to recognize typedef names.
    //
    bool is_type_start() const;

    int   binop_prec(tk k) const;
    bin_op token_to_binop(tk k) const;
    bin_op token_to_assign_op(tk k) const;
    bool   is_assign_op(tk k) const;

    // Convenience wrappers that delegate to frame_.
    int alloc_local(int size) { return frame_.alloc_local(size); }
    int alloc_param(int size) { return frame_.alloc_param(size); }

    //
    // Generate a unique label name for internal use (e.g., switch).
    //
    std::string fresh_label();

    // Counter for naming anonymous compound-literal locals (__clit0, ...).
    int clit_counter_ = 0;

    // Counter for mangling static-local variable names (_func__var_N).
    int static_local_counter_ = 0;

    // Overflow queue for comma-separated global declarations.
    // parse_external_declaration drains this before parsing new input.
    std::deque<decl_ptr> pending_decls_;

    expr_ptr last_vla_size_;   // saved during parse_direct_declarator_suffix for VLA
    int      vla_counter_ = 0; // unique suffix for hidden VLA size locals

    // ----- expression helpers ----------------------------------------

    //
    // Parse a _Generic(expr, type: expr, ..., default: expr) selection.
    // Returns the expression for the first matching type association, or
    // the default expression if no type matches.
    //
    expr_ptr parse_generic_selection();

    // ----- symbol-creation helpers -----------------------------------

    // Make and insert a stack-allocated local variable symbol.
    // Returns the new symbol; stack_offset is assigned via alloc_local().
    std::shared_ptr<symbol> make_local_sym(const std::string &name,
                                           type_ptr t, storage_class sc);

    // Make and insert a static-local variable symbol with a mangled asm name.
    // Advances static_local_counter_.  Returns the mangled asm name.
    std::string make_static_sym(std::shared_ptr<symbol> &out,
                                const std::string &name, type_ptr t);

    // Make and insert a typedef symbol into the current scope.
    std::shared_ptr<symbol> make_type_sym(const std::string &name, type_ptr t);

    // Internal side-channels between parse_declarator and
    // parse_direct_declarator_suffix.  Collected into declarator_info
    // before parse_declarator returns; not visible to callers.
    std::vector<std::unique_ptr<param_decl>> last_params_;
    bool                                     last_variadic_ = false;
};

} // namespace xcc
