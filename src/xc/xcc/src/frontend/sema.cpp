//
// sema.cpp — semantic analysis pass for the xcc compiler.
//
// Walks the parsed AST and emits diagnostics for semantic errors that
// cannot be caught during parsing:
//
//   const enforcement: assignment to a const-qualified variable or to
//   memory through a pointer-to-const is a compile error.
//   duplicate cases: two case labels with the same value in a switch.
//   duplicate default: more than one default label in a switch.
//   arg count mismatch: call with wrong number of args for a known prototype.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/sema.h"
#include "frontend/const_eval.h"
#include <unordered_set>

namespace xcc {

// ----- helpers -------------------------------------------------------

bool sema::is_const_lval(const expr &e) const {
    if (auto *id = dynamic_cast<const ident_expr*>(&e))
        return id->sym && id->sym->type && id->sym->type->is_const;
    if (auto *u = dynamic_cast<const unary_expr*>(&e))
        if (u->op == unary_op::DEREF && u->operand &&
            u->operand->type && u->operand->type->base)
            return u->operand->type->base->is_const;
    if (auto *m = dynamic_cast<const member_expr*>(&e))
        return m->type && m->type->is_const;
    return false;
}

static bool is_assign_op(bin_op op) {
    switch (op) {
    case bin_op::ASSIGN:
    case bin_op::ADD_ASSIGN: case bin_op::SUB_ASSIGN:
    case bin_op::MUL_ASSIGN: case bin_op::DIV_ASSIGN:
    case bin_op::MOD_ASSIGN: case bin_op::AND_ASSIGN:
    case bin_op::OR_ASSIGN:  case bin_op::XOR_ASSIGN:
    case bin_op::SHL_ASSIGN: case bin_op::RHS_ASSIGN:
        return true;
    default: return false;
    }
}

// ----- expr_visitor --------------------------------------------------

void sema::visit(binary_expr &e) {
    if (is_assign_op(e.op) && e.left && is_const_lval(*e.left))
        diag_.error(e.loc, "assignment to const-qualified lvalue");
    if (e.left)  e.left->accept(*this);
    if (e.right) e.right->accept(*this);
}

void sema::visit(unary_expr &e) {
    if (e.operand) e.operand->accept(*this);
}

void sema::visit(cast_expr &e) {
    if (e.operand) e.operand->accept(*this);
}

void sema::visit(call_expr &e) {
    if (e.callee) e.callee->accept(*this);
    for (auto &a : e.args) if (a) a->accept(*this);

    // Argument count check against known prototype.
    // Only fires when the function type has at least one declared parameter
    // (empty params = K&R / void, can't reliably distinguish without extra state).
    const type *fn_type = nullptr;
    if (e.callee && e.callee->type) {
        if (e.callee->type->is_func())
            fn_type = e.callee->type.get();
        else if (e.callee->type->is_ptr() && e.callee->type->base &&
                 e.callee->type->base->is_func())
            fn_type = e.callee->type->base.get();
    }

    // Propagate the callee return type onto the call expression so IR
    // generation can size temporaries and returns correctly.
    if (fn_type && fn_type->ret)
        e.type = fn_type->ret;

    if (fn_type && !fn_type->variadic && !fn_type->params.empty()) {
        if (e.args.size() != fn_type->params.size())
            diag_.error(e.loc, "wrong number of arguments to function call");
    }
}

void sema::visit(index_expr &e) {
    if (e.base)  e.base->accept(*this);
    if (e.index) e.index->accept(*this);
}

void sema::visit(member_expr &e) {
    if (e.object) e.object->accept(*this);
}

void sema::visit(compound_literal_expr &e) {
    if (e.init) e.init->accept(*this);
}

void sema::visit(conditional_expr &e) {
    if (e.cond)      e.cond->accept(*this);
    if (e.then_expr) e.then_expr->accept(*this);
    if (e.else_expr) e.else_expr->accept(*this);
}

void sema::visit(init_list_expr &e) {
    for (auto &elem : e.elements) if (elem.value) elem.value->accept(*this);
}

void sema::visit(stmt_expr &e) {
    if (e.body) e.body->accept(*this);
}

// ----- stmt_visitor --------------------------------------------------

void sema::visit(compound_stmt &s) {
    for (auto &sub : s.body) if (sub) sub->accept(*this);
}

void sema::visit(expr_stmt &s) {
    if (s.expr) s.expr->accept(*this);
}

void sema::visit(decl_stmt &s) {
    for (auto &d : s.decls) if (d) d->accept(*this);
}

void sema::visit(return_stmt &s) {
    if (s.value) s.value->accept(*this);
}

void sema::visit(if_stmt &s) {
    if (s.cond)      s.cond->accept(*this);
    if (s.then_body) s.then_body->accept(*this);
    if (s.else_body) s.else_body->accept(*this);
}

void sema::visit(while_stmt &s) {
    if (s.cond) s.cond->accept(*this);
    if (s.body) s.body->accept(*this);
}

void sema::visit(do_while_stmt &s) {
    if (s.body) s.body->accept(*this);
    if (s.cond) s.cond->accept(*this);
}

void sema::visit(for_stmt &s) {
    if (s.init) s.init->accept(*this);
    if (s.cond) s.cond->accept(*this);
    if (s.step) s.step->accept(*this);
    if (s.body) s.body->accept(*this);
}

void sema::visit(label_stmt &s) {
    if (s.body) s.body->accept(*this);
}

void sema::visit(switch_stmt &s) {
    if (s.cond) s.cond->accept(*this);

    // Duplicate case/default detection: scan direct children of the switch body.
    if (auto *cs = dynamic_cast<compound_stmt*>(s.body.get())) {
        std::unordered_set<int64_t> seen_vals;
        bool seen_default = false;
        for (auto &st : cs->body) {
            auto *c = dynamic_cast<case_stmt*>(st.get());
            if (!c) continue;
            if (c->is_default) {
                if (seen_default)
                    diag_.error(c->loc, "duplicate default label in switch");
                seen_default = true;
            } else if (c->value) {
                auto opt = const_expr_evaluator::evaluate(c->value.get());
                if (opt) {
                    if (!seen_vals.insert(*opt).second)
                        diag_.error(c->loc, "duplicate case value in switch");
                }
            }
        }
    }

    if (s.body) s.body->accept(*this);
}

void sema::visit(case_stmt &s) {
    if (s.value) s.value->accept(*this);
    if (s.body)  s.body->accept(*this);
}

// ----- decl_visitor --------------------------------------------------

void sema::visit(var_decl &d) {
    if (d.init) d.init->accept(*this);
}

void sema::visit(func_decl &d) {
    if (d.body) d.body->accept(*this);
}

// ----- entry point ---------------------------------------------------

int sema::check(const translation_unit &tu) {
    for (auto &d : tu.decls)
        if (d) const_cast<decl&>(*d).accept(*this);
    return diag_.error_count();
}

} // namespace xcc
