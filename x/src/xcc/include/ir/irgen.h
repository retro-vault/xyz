//
// irgen.h — AST-to-IR lowering pass for the xcc compiler.
//
// ir_gen walks a translation_unit AST and emits three-address icode
// instructions into an ir_module.  It implements all three visitor
// interfaces (expr_visitor, stmt_visitor, decl_visitor) and traverses
// the AST in a single top-down pass.
//
// Expression results are communicated through expr_result_: after any
// gen_expr() call returns, expr_result_ holds the operand for the
// computed value.  Callers capture this operand before making further
// gen_expr() calls.
//
// Lvalue handling: gen_assign(), gen_compound_assign(), and
// gen_member_ptr() work together to emit correct store sequences for
// all valid left-hand-side forms: identifiers, dereferences, array
// subscripts, and struct/union member access.
//
// Break/continue targets are saved on entry to each loop or switch
// and restored on exit, enabling correct code generation for nested
// control flow.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "icode.h"
#include "frontend/ast.h"
#include <memory>
#include <string>
#include <vector>

namespace xcc {

class ir_gen : public expr_visitor,
               public stmt_visitor,
               public decl_visitor {
public:
    //
    // Construct the IR generator.
    //
    ir_gen();

    //
    // Lower the entire translation unit to an IR module.
    // The returned module is owned by the caller.
    //
    std::unique_ptr<ir_module> lower(translation_unit &tu);

private:
    std::unique_ptr<ir_module> mod_;
    ir_function               *cur_fn_    = nullptr;
    int                        next_temp_ = 0;
    int                        next_lbl_  = 0;

    // Target labels for break and continue inside loops/switch.
    std::string break_lbl_;
    std::string cont_lbl_;

    // Argument slot counter for SEND sequences.
    int send_idx_ = 0;

    // Current source line being lowered; propagated to icode::line.
    int cur_line_ = 0;

    // Result of the most recently visited expression.
    operand expr_result_;

    // Hidden SDCC ABI aggregate-result pointer for the current function.
    operand sret_result_ptr_;

    // ----- helpers ---------------------------------------------------

    //
    // Allocate a fresh anonymous temporary with type t.
    //
    operand new_temp(type_ptr t);

    //
    // Return a unique label string (e.g., ".L42").
    //
    std::string new_label();

    //
    // Append ic to the current function's instruction list.
    //
    void emit(icode ic);

    //
    // Convert a constant operand to target type using the same storage
    // narrowing rules as runtime casts.  Non-constant operands are returned
    // unchanged so callers can decide whether to emit an explicit CAST.
    //
    operand coerce_const_operand(operand op, const type_ptr &target);

    //
    // Visit e through the expr_visitor interface and return the result
    // operand.  After this call expr_result_ holds the same value.
    //
    operand gen_expr(expr &e);

    //
    // Visit s through the stmt_visitor interface.
    //
    void gen_stmt(stmt &s);

    //
    // Lower e directly as a control-flow condition that branches to
    // true_lbl / false_lbl with proper short-circuit semantics.
    //
    void emit_cond_branch(expr &e,
                          const std::string &true_lbl,
                          const std::string &false_lbl);

    //
    // Lower e as a condition value for IFX, preserving the special
    // identifier handling and integer promotion rules used by control
    // statements.
    //
    operand gen_cond_value(expr &e);

    //
    // Visit d through the decl_visitor interface.
    //
    void gen_decl(decl &d);

    //
    // Emit the complete prologue/body/epilogue icode sequence for fd.
    //
    void gen_func(func_decl &fd);

    //
    // Convert a symbol table entry into an operand.
    //
    operand sym_to_operand(const symbol &sym, type_ptr ty);

    //
    // Widen op to at least int rank by emitting a CAST if needed.
    // Used to implement C integer promotions before arithmetic.
    //
    operand promote(operand op);

    //
    // Emit an ASSIGN from src to dst.
    //
    void emit_assign(operand dst, operand src);

    //
    // Allocate a new temp, emit a binary icode (op result = left op right),
    // and return the result temp.
    //
    operand emit_binop(icode_op op, operand left, operand right, type_ptr t);

    //
    // Allocate a new temp, emit a unary icode (op result = left), and
    // return the result temp.
    //
    operand emit_unop(icode_op op, operand left, type_ptr t);

    // ----- expr_visitor ----------------------------------------------
    void visit(int_literal_expr   &) override;
    void visit(float_literal_expr &) override;
    void visit(char_literal_expr  &) override;
    void visit(string_literal_expr&) override;
    void visit(ident_expr         &) override;
    void visit(binary_expr        &) override;
    void visit(unary_expr         &) override;
    void visit(cast_expr          &) override;
    void visit(call_expr          &) override;
    void visit(index_expr         &) override;
    void visit(member_expr        &) override;
    void visit(sizeof_expr           &) override;
    void visit(compound_literal_expr &) override;
    void visit(conditional_expr      &) override;
    void visit(init_list_expr        &) override;
    void visit(stmt_expr             &) override;

    //
    // Emit the icode sequence to initialise a local aggregate (array or
    // struct) from il.  sym identifies the local variable, type is its
    // declared type.  base_override supplies the object address for locals
    // whose storage is allocated dynamically (for example over-aligned
    // automatic arrays).
    //
    void gen_init_list(const symbol &sym, type_ptr type,
                       init_list_expr &il,
                       const operand *base_override = nullptr);

    void gen_binary_arith   (binary_expr &e);
    void gen_complex_arith  (operand lhs, operand rhs, operand result, bin_op op);
    void gen_binary_compare (binary_expr &e);
    void gen_binary_logical (binary_expr &e);

    //
    // Emit a simple or compound assignment.
    //
    void gen_assign         (binary_expr &e);
    void gen_compound_assign(binary_expr &e);

    //
    // Emit a store of src to the lvalue lhs.  Handles all lvalue forms
    // (ident, pointer deref, array subscript, struct member) in one
    // place, eliminating the duplicate RTTI chains in gen_assign and
    // gen_compound_assign.  Returns the operand that represents the
    // result of the assignment expression (ident → the symbol operand;
    // all other forms → src itself).
    //
    operand gen_lvalue_write(expr &lhs, operand src);

    //
    // Return a pointer operand addressing the struct/union field named
    // by e.  Used by both read (member_expr visitor) and write
    // (gen_assign, gen_compound_assign) code paths.
    //
    operand gen_lvalue_addr(expr &e, type_ptr ptr_t);
    operand gen_member_ptr(member_expr &e);

    // ----- stmt_visitor ----------------------------------------------
    void visit(compound_stmt  &) override;
    void visit(expr_stmt      &) override;
    void visit(decl_stmt      &) override;
    void visit(return_stmt    &) override;
    void visit(if_stmt        &) override;
    void visit(while_stmt     &) override;
    void visit(do_while_stmt  &) override;
    void visit(for_stmt       &) override;
    void visit(break_stmt     &) override;
    void visit(continue_stmt  &) override;
    void visit(goto_stmt      &) override;
    void visit(label_stmt     &) override;
    void visit(switch_stmt    &) override;
    void visit(case_stmt      &) override;
    void visit(asm_stmt       &) override;

    // ----- decl_visitor ----------------------------------------------
    void visit(var_decl    &) override;
    void visit(func_decl   &) override;
    void visit(param_decl  &) override;
    void visit(typedef_decl&) override;
    void visit(struct_decl &) override;
};

} // namespace xcc
