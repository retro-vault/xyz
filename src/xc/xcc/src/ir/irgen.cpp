//
// irgen.cpp — AST to three-address IR lowering: constructor, helpers,
//             and module entry point.
//
// The IR generation implementation is split across several translation units:
//   irgen.cpp           — this file (constructor, helpers, lower)
//   irgen_decl.cpp      — declaration visitors and gen_func
//   irgen_stmt.cpp      — statement visitors
//   irgen_expr.cpp      — expression visitors and arithmetic helpers
//   irgen_lvalue.cpp    — lvalue read/write, index/member access
//   irgen_init.cpp      — aggregate initialiser lowering
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"
#include "backend/z80/convention.h"
#include <cassert>

namespace xcc {

// ----- Helpers -------------------------------------------------------

ir_gen::ir_gen() {}

operand ir_gen::new_temp(type_ptr t) {
    return operand::make_temp(next_temp_++, t);
}

std::string ir_gen::new_label() {
    return "__xcc_L" + std::to_string(next_lbl_++);
}

void ir_gen::emit(icode ic) {
    assert(cur_fn_ && "emit outside function");
    ic.line = cur_line_;
    cur_fn_->icodes.push_back(std::move(ic));
}

operand ir_gen::gen_expr(expr &e) {
    e.accept(*this);
    return expr_result_;
}

void ir_gen::gen_stmt(stmt &s) {
    if (s.loc.line > 0) cur_line_ = s.loc.line;
    s.accept(*this);
}

void ir_gen::gen_decl(decl &d) { d.accept(*this); }

operand ir_gen::sym_to_operand(const symbol &sym, type_ptr ty) {
    const std::string &name = !sym.asm_name.empty() ? sym.asm_name : sym.name;
    bool is_sfr = (sym.sfr_port >= 0);
    return operand::make_symbol(name, ty ? ty : sym.type,
                                sym.is_global, sym.is_param,
                                sym.stack_offset, sym.kind == sym_kind::FUNC,
                                sym.is_tls,
                                is_sfr, sym.sfr_port);
}

operand ir_gen::promote(operand op) {
    if (!op.type || !op.type->is_integer()) return op;
    if (op.type->size() < 2) {
        operand t = new_temp(type::make_int());
        icode ic;
        ic.op     = icode_op::CAST;
        ic.result = t;
        ic.left   = op;
        emit(ic);
        return t;
    }
    return op;
}

void ir_gen::emit_assign(operand dst, operand src) {
    icode ic;
    ic.op     = icode_op::ASSIGN;
    ic.result = dst;
    ic.left   = src;
    emit(ic);
}

operand ir_gen::emit_binop(icode_op op, operand left, operand right, type_ptr t) {
    if (t) {
        if (left.kind == operand_kind::FLOAT_CONST)
            left.type = t;
        if (right.kind == operand_kind::FLOAT_CONST)
            right.type = t;
    }
    operand res = new_temp(t);
    icode ic; ic.op = op; ic.result = res; ic.left = left; ic.right = right;
    emit(ic);
    return res;
}

operand ir_gen::emit_unop(icode_op op, operand left, type_ptr t) {
    if (t && left.kind == operand_kind::FLOAT_CONST)
        left.type = t;
    operand res = new_temp(t);
    icode ic; ic.op = op; ic.result = res; ic.left = left;
    emit(ic);
    return res;
}

// ----- Module entry --------------------------------------------------

std::unique_ptr<ir_module> ir_gen::lower(translation_unit &tu) {
    mod_ = std::make_unique<ir_module>();
    for (auto &d : tu.decls)
        if (d) gen_decl(*d);
    return std::move(mod_);
}

} // namespace xcc
