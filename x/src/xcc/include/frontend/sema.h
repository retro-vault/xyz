//
// sema.h — semantic analysis pass for the xcc compiler.
//
// Performs checks that cannot be done during parsing because they require
// complete type information:
//
//   const enforcement   — assignments to const-qualified lvalues are errors
//   duplicate cases     — two case labels with the same value in a switch
//   duplicate default   — more than one default label in a switch
//   arg count mismatch  — call with wrong number of args for a known prototype
//
// sema::check() walks the AST after parsing and before IR generation.
// The error count mirrors the parser's interface so the driver can test
// both in the same way.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "frontend/ast.h"
#include "frontend/diag.h"
#include <unordered_map>

namespace xcc {

class sema
    : public default_expr_visitor
    , public default_stmt_visitor
    , public default_decl_visitor
{
public:
    explicit sema(diag_engine &diag,
                  const std::unordered_map<std::string, call_abi> *imported_abis = nullptr)
        : diag_(diag), imported_abis_(imported_abis) {}

    //
    // Walk the entire translation unit and report semantic errors to
    // the diagnostic engine.  Returns the number of errors found.
    //
    int check(const translation_unit &tu);

    //
    // Return the number of errors found so far.
    //
    int error_count() const { return diag_.error_count(); }

private:
    diag_engine &diag_;
    const std::unordered_map<std::string, call_abi> *imported_abis_ = nullptr;

    bool is_const_lval(const expr &e) const;

    //
    // Apply the attrs list to sym, setting flags and emitting diagnostics
    // for invalid or unrecognised attributes.
    //
    void apply_attrs(symbol &sym, const attr_list &attrs, source_loc loc);
    void apply_imported_call_abi(symbol &sym, const attr_list &attrs, source_loc loc);
    void normalize_variadic_call_abi(func_decl &d);

    // ----- expr_visitor overrides ------------------------------------
    void visit(binary_expr           &e) override;
    void visit(unary_expr            &e) override;
    void visit(cast_expr             &e) override;
    void visit(call_expr             &e) override;
    void visit(sizeof_expr           &e) override;
    void visit(index_expr            &e) override;
    void visit(member_expr           &e) override;
    void visit(compound_literal_expr &e) override;
    void visit(conditional_expr      &e) override;
    void visit(init_list_expr        &e) override;
    void visit(stmt_expr             &e) override;

    // ----- stmt_visitor overrides ------------------------------------
    void visit(compound_stmt  &s) override;
    void visit(expr_stmt      &s) override;
    void visit(decl_stmt      &s) override;
    void visit(return_stmt    &s) override;
    void visit(if_stmt        &s) override;
    void visit(while_stmt     &s) override;
    void visit(do_while_stmt  &s) override;
    void visit(for_stmt       &s) override;
    void visit(label_stmt     &s) override;
    void visit(switch_stmt    &s) override;
    void visit(case_stmt      &s) override;

    // ----- decl_visitor overrides ------------------------------------
    void visit(var_decl  &d) override;
    void visit(func_decl &d) override;
};

} // namespace xcc
