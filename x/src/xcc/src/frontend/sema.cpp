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
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace xcc {

// ----- attribute application -----------------------------------------

static type_ptr apply_array_qualifiers_to_element(const type_ptr &elem,
                                                  const type_ptr &array_ty) {
    if (!elem || !array_ty)
        return elem;
    if (!array_ty->is_const && !array_ty->is_volatile &&
        !array_ty->is_restrict)
        return elem;

    auto qualified = std::make_shared<type>(*elem);
    qualified->is_const    = qualified->is_const || array_ty->is_const;
    qualified->is_volatile = qualified->is_volatile || array_ty->is_volatile;
    qualified->is_restrict = qualified->is_restrict || array_ty->is_restrict;
    return qualified;
}

static type_ptr subscript_element_type(const type_ptr &ty) {
    if (!ty)
        return nullptr;

    type_ptr unqualified = ty->unqual();
    if (!unqualified)
        return nullptr;

    if ((unqualified->kind == type_kind::POINTER ||
         unqualified->kind == type_kind::ARRAY) &&
        unqualified->base) {
        if (unqualified->kind == type_kind::ARRAY)
            return apply_array_qualifiers_to_element(unqualified->base, ty);
        return unqualified->base;
    }

    return nullptr;
}

static bool attrs_specify_call_abi(const attr_list &attrs) {
    for (const auto &a : attrs) {
        if (a.ns == "sdcc" &&
            (a.name == "naked" || a.name == "interrupt"
             || a.name == "critical" || a.name == "sdccall")) {
            return true;
        }
        if (a.ns == "z88dk" &&
            (a.name == "fastcall" || a.name == "callee")) {
            return true;
        }
    }
    return false;
}

static const char *call_abi_name(call_abi abi) {
    switch (abi) {
    case call_abi::DEFAULT:         return "default";
    case call_abi::SDCCCALL0:       return "sdcccall(0)";
    case call_abi::SDCCCALL1:       return "sdcccall(1)";
    case call_abi::Z88DK_FASTCALL:  return "z88dk::fastcall";
    case call_abi::Z88DK_CALLEE:    return "z88dk::callee";
    case call_abi::NAKED:           return "sdcc::naked";
    case call_abi::INTERRUPT:       return "sdcc::interrupt";
    case call_abi::CRITICAL:        return "sdcc::critical";
    default:                        return "unknown";
    }
}

static bool variadic_abi_must_be_stack(call_abi abi) {
    return abi == call_abi::DEFAULT || abi == call_abi::SDCCCALL0;
}

void sema::apply_attrs(symbol &sym, const attr_list &attrs, source_loc loc) {
    for (auto &a : attrs) {
        // ----- standard C23 attributes (no namespace) ----------------
        if (a.ns.empty()) {
            if (a.name == "noreturn") {
                sym.attr_noreturn = true;
            } else if (a.name == "deprecated") {
                sym.attr_deprecated = true;
                if (!a.args.empty()) sym.deprecated_msg = a.args[0];
            } else if (a.name == "nodiscard") {
                sym.attr_nodiscard = true;
                if (!a.args.empty()) sym.nodiscard_msg = a.args[0];
            } else if (a.name == "maybe_unused") {
                sym.attr_maybe_unused = true;
            } else if (a.name == "fallthrough") {
                // Statement attribute; no symbol effect
            } else if (a.name == "unsequenced") {
                sym.attr_unsequenced = true;
            } else if (a.name == "reproducible") {
                sym.attr_reproducible = true;
            } else {
                diag_.warning(warning_group::ATTRIBUTES, a.loc,
                              "unknown standard attribute '[[%s]]' ignored",
                              a.name.c_str());
            }
        }
        // ----- SDCC vendor attributes (namespace sdcc) ---------------
        else if (a.ns == "sdcc") {
            if (a.name == "naked") {
                sym.abi = call_abi::NAKED;
            } else if (a.name == "interrupt") {
                sym.abi = call_abi::INTERRUPT;
            } else if (a.name == "critical") {
                sym.abi = call_abi::CRITICAL;
            } else if (a.name == "sdccall") {
                if (a.args.empty()) {
                    diag_.error(a.loc, "[[sdcc::sdccall]] requires an integer argument (0 or 1)");
                } else {
                    int n = std::stoi(a.args[0]);
                    if (n == 0)       sym.abi = call_abi::SDCCCALL0;
                    else if (n == 1)  sym.abi = call_abi::SDCCCALL1;
                    else diag_.error(a.loc, "[[sdcc::sdccall(%d)]]: only 0 and 1 are supported", n);
                }
            } else if (a.name == "at") {
                if (a.args.empty()) {
                    diag_.error(a.loc, "[[sdcc::at]] requires an address argument");
                } else {
                    try {
                        sym.at_address = (int64_t)std::stoull(a.args[0], nullptr, 0);
                    } catch (...) {
                        diag_.error(a.loc, "[[sdcc::at]]: invalid address '%s'",
                                    a.args[0].c_str());
                    }
                }
            } else if (a.name == "sfr") {
                if (a.args.empty()) {
                    diag_.error(a.loc, "[[sdcc::sfr]] requires a port number argument");
                } else {
                    try {
                        unsigned long long port = std::stoull(a.args[0], nullptr, 0);
                        if (port > 0xffffull) {
                            diag_.error(a.loc, "[[sdcc::sfr]]: port '%s' is outside 16-bit range",
                                        a.args[0].c_str());
                        } else {
                            sym.sfr_port = (int)port;
                        }
                    } catch (...) {
                        diag_.error(a.loc, "[[sdcc::sfr]]: invalid port '%s'",
                                    a.args[0].c_str());
                    }
                }
            } else {
                diag_.warning(warning_group::ATTRIBUTES, a.loc,
                              "unknown sdcc attribute '[[sdcc::%s]]' ignored",
                              a.name.c_str());
            }
        }
        // ----- z88dk vendor attributes (namespace z88dk) ------------
        else if (a.ns == "z88dk") {
            if (a.name == "fastcall") {
                sym.abi = call_abi::Z88DK_FASTCALL;
            } else if (a.name == "callee") {
                sym.abi = call_abi::Z88DK_CALLEE;
            } else {
                diag_.warning(warning_group::ATTRIBUTES, a.loc,
                              "unknown z88dk attribute '[[z88dk::%s]]' ignored",
                              a.name.c_str());
            }
        }
        // ----- unknown namespace ------------------------------------
        else {
            // Unknown namespace — silently ignore per C23 rules.
            (void)loc;
        }
    }
}

void sema::apply_imported_call_abi(symbol &sym,
                                   const attr_list &attrs,
                                   source_loc loc) {
    if (!imported_abis_ || sym.kind != sym_kind::FUNC)
        return;

    auto it = imported_abis_->find(sym.name);
    if (it == imported_abis_->end())
        return;

    const call_abi imported = it->second;
    if (imported == call_abi::DEFAULT)
        return;

    if (attrs_specify_call_abi(attrs)) {
        if (sym.abi != imported) {
            diag_.warning(warning_group::ABI, loc,
                          "declaration for '%s' overrides imported library calling convention %s",
                          sym.name.c_str(), call_abi_name(imported));
        }
        return;
    }

    if (sym.abi == call_abi::DEFAULT)
        sym.abi = imported;
}

void sema::normalize_variadic_call_abi(func_decl &d) {
    if (!d.is_variadic || !d.sym)
        return;

    if (!variadic_abi_must_be_stack(d.sym->abi)) {
        diag_.error(d.loc,
                    "variadic function '%s' must use sdcccall(0)",
                    d.name.c_str());
        return;
    }

    d.sym->abi = call_abi::SDCCCALL0;
}

static void sync_nested_function_type_abi(type_ptr t, call_abi abi) {
    if (!t)
        return;
    if (t->kind == type_kind::FUNCTION) {
        t->func_abi = abi;
        return;
    }
    if ((t->kind == type_kind::POINTER || t->kind == type_kind::ARRAY) && t->base)
        sync_nested_function_type_abi(t->base, abi);
}

static void sync_function_symbol_abi_from_type(func_decl &d) {
    if (!d.sym || !d.type || !d.type->is_func())
        return;
    if (d.sym->abi == call_abi::DEFAULT &&
        d.type->func_abi != call_abi::DEFAULT) {
        d.sym->abi = d.type->func_abi;
    }
}

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
    const symbol *callee_sym = nullptr;
    if (e.callee && e.callee->type) {
        if (e.callee->type->is_func())
            fn_type = e.callee->type.get();
        else if (e.callee->type->is_ptr() && e.callee->type->base &&
                 e.callee->type->base->is_func())
            fn_type = e.callee->type->base.get();
    }
    if (auto *id = dynamic_cast<ident_expr*>(e.callee.get()))
        callee_sym = id->sym.get();

    // Propagate the callee return type onto the call expression so IR
    // generation can size temporaries and returns correctly.
    if (fn_type && fn_type->ret)
        e.type = fn_type->ret;

    // C23: any prototyped function (including f(void) and f()) enforces arg count.
    if (fn_type && fn_type->is_prototyped && !fn_type->variadic) {
        if (e.args.size() != fn_type->params.size())
            diag_.error(e.loc, "wrong number of arguments to function call");
    }

    // Attribute use-site checks
    if (callee_sym) {
        if (callee_sym->attr_deprecated) {
            if (callee_sym->deprecated_msg.empty())
                diag_.warning(warning_group::DEPRECATED_DECLARATIONS, e.loc,
                              "'%s' is deprecated", callee_sym->name.c_str());
            else
                diag_.warning(warning_group::DEPRECATED_DECLARATIONS, e.loc,
                              "'%s' is deprecated: %s",
                              callee_sym->name.c_str(),
                              callee_sym->deprecated_msg.c_str());
        }
        // nodiscard: checked in visit(expr_stmt) since we need the stmt context
    }
}

void sema::visit(sizeof_expr &e) {
    if (e.sizeof_expr_op)
        e.sizeof_expr_op->accept(*this);
}

void sema::visit(index_expr &e) {
    if (e.base)  e.base->accept(*this);
    if (e.index) e.index->accept(*this);

    auto set_from_subscriptable = [&](const type_ptr &t) -> bool {
        e.type = subscript_element_type(t);
        e.is_lvalue = (e.type != nullptr);
        return e.type != nullptr;
    };

    if (set_from_subscriptable(e.base ? e.base->type : nullptr))
        return;
    set_from_subscriptable(e.index ? e.index->type : nullptr);
}

void sema::visit(member_expr &e) {
    if (e.object) e.object->accept(*this);

    type_ptr st = e.object ? e.object->type : nullptr;
    if (!st)
        return;

    st = st->unqual();
    if (e.is_arrow) {
        if (st && st->is_ptr())
            st = st->base ? st->base->unqual() : nullptr;
        else
            return;
    }

    if (!st || (st->kind != type_kind::STRUCT && st->kind != type_kind::UNION))
        return;

    for (auto &f : st->fields) {
        if (f.name != e.member)
            continue;
        e.type = f.type;
        e.is_lvalue = true;
        return;
    }
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
    // [[nodiscard]] check: warn when a nodiscard function's return value is discarded
    if (s.expr) {
        auto *ce = dynamic_cast<call_expr*>(s.expr.get());
        if (ce && ce->callee) {
            if (auto *id = dynamic_cast<ident_expr*>(ce->callee.get())) {
                if (id->sym && id->sym->attr_nodiscard) {
                    if (id->sym->nodiscard_msg.empty())
                        diag_.warning(warning_group::UNUSED_RESULT, s.loc,
                                      "return value of '%s' should not be discarded",
                                      id->sym->name.c_str());
                    else
                        diag_.warning(warning_group::UNUSED_RESULT, s.loc,
                                      "return value of '%s' should not be discarded: %s",
                                      id->sym->name.c_str(),
                                      id->sym->nodiscard_msg.c_str());
                }
            }
        }
    }
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
    if (d.sym && !d.attrs.empty())
        apply_attrs(*d.sym, d.attrs, d.loc);
    if (d.init) d.init->accept(*this);
    // constexpr requires a constant initializer.
    if (d.sym && d.sym->type && d.sym->type->is_const && d.init) {
        if (!const_expr_evaluator::evaluate(d.init.get()))
            diag_.warning(warning_group::CONSTEXPR_NOT_CONSTANT, d.loc,
                          "constexpr / const initializer is not a constant expression");
    }
}

void sema::visit(func_decl &d) {
    sync_function_symbol_abi_from_type(d);
    if (d.sym && !d.attrs.empty())
        apply_attrs(*d.sym, d.attrs, d.loc);
    if (d.sym)
        apply_imported_call_abi(*d.sym, d.attrs, d.loc);
    normalize_variadic_call_abi(d);
    if (d.sym) {
        sync_nested_function_type_abi(d.type, d.sym->abi);
        sync_nested_function_type_abi(d.sym->type, d.sym->abi);
    }
    if (d.sym && d.sym->abi == call_abi::Z88DK_FASTCALL) {
        if (d.params.size() != 1) {
            diag_.error(d.loc, "[[z88dk::fastcall]] requires exactly one parameter");
        } else {
            type_ptr param_type = d.params.front() ? d.params.front()->type : nullptr;
            int sz = param_type ? param_type->size() : 0;
            if (!(sz == 1 || sz == 2 || sz == 4)) {
                diag_.error(d.loc,
                            "[[z88dk::fastcall]] requires an 8-, 16-, or 32-bit parameter");
            }
        }
    }
    if (d.body) d.body->accept(*this);
}

// ----- entry point ---------------------------------------------------

int sema::check(const translation_unit &tu) {
    for (auto &d : tu.decls)
        if (d) const_cast<decl&>(*d).accept(*this);
    return diag_.error_count();
}

} // namespace xcc
