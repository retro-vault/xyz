//
// irgen_stmt.cpp — statement visitors for the xcc IR lowering pass.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"

namespace xcc {

operand ir_gen::gen_cond_value(expr &e) {
    operand cond;
    if (auto *id = dynamic_cast<ident_expr *>(&e);
        id && id->sym && id->sym->kind != sym_kind::ENUM_CONST) {
        cond = sym_to_operand(*id->sym, id->sym->type);
    } else {
        cond = gen_expr(e);
    }

    if (cond.type && cond.type->is_integer() &&
        cond.type->size() < type::make_int()->size()) {
        cond = emit_unop(icode_op::CAST, cond, type::make_int());
    }
    return cond;
}

void ir_gen::emit_cond_branch(expr &e,
                              const std::string &true_lbl,
                              const std::string &false_lbl) {
    if (auto *bin = dynamic_cast<binary_expr *>(&e)) {
        if (bin->op == bin_op::LAND) {
            std::string rhs_lbl = new_label();
            emit_cond_branch(*bin->left, rhs_lbl, false_lbl);
            { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = rhs_lbl; emit(lbl); }
            emit_cond_branch(*bin->right, true_lbl, false_lbl);
            return;
        }
        if (bin->op == bin_op::LOR) {
            std::string rhs_lbl = new_label();
            emit_cond_branch(*bin->left, true_lbl, rhs_lbl);
            { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = rhs_lbl; emit(lbl); }
            emit_cond_branch(*bin->right, true_lbl, false_lbl);
            return;
        }
        if (bin->op == bin_op::COMMA) {
            gen_expr(*bin->left);
            emit_cond_branch(*bin->right, true_lbl, false_lbl);
            return;
        }
    }

    if (auto *un = dynamic_cast<unary_expr *>(&e)) {
        if (un->op == unary_op::NOT) {
            emit_cond_branch(*un->operand, false_lbl, true_lbl);
            return;
        }
    }

    if (auto *lit = dynamic_cast<int_literal_expr *>(&e)) {
        icode ic;
        ic.op = icode_op::GOTO;
        ic.label_name = lit->value ? true_lbl : false_lbl;
        emit(ic);
        return;
    }

    operand cond = gen_cond_value(e);
    icode ic;
    ic.op = icode_op::IFX;
    ic.left = cond;
    ic.true_lbl = true_lbl;
    ic.false_lbl = false_lbl;
    emit(ic);
}

void ir_gen::visit(compound_stmt &s) {
    for (auto &child : s.body)
        if (child) gen_stmt(*child);
}

void ir_gen::visit(expr_stmt &s) {
    if (s.expr) gen_expr(*s.expr);
}

void ir_gen::visit(decl_stmt &s) {
    for (auto &d : s.decls)
        if (d) gen_decl(*d);
}

void ir_gen::visit(return_stmt &s) {
    icode ic;
    ic.op = icode_op::RETURN;
    if (s.value) {
        ic.left = gen_expr(*s.value);
        if (cur_fn_ && cur_fn_->ret_type &&
            cur_fn_->ret_type->kind != type_kind::VOID &&
            ic.left.type != cur_fn_->ret_type) {
            ic.left = emit_unop(icode_op::CAST, ic.left, cur_fn_->ret_type);
        }
    }
    emit(ic);
}

void ir_gen::visit(if_stmt &s) {
    std::string then_lbl = new_label();
    std::string else_lbl = new_label();
    std::string end_lbl  = new_label();

    emit_cond_branch(*s.cond, then_lbl, s.else_body ? else_lbl : end_lbl);

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = then_lbl; emit(lbl); }
    gen_stmt(*s.then_body);
    { icode jmp; jmp.op = icode_op::GOTO; jmp.label_name = end_lbl; emit(jmp); }

    if (s.else_body) {
        { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = else_lbl; emit(lbl); }
        gen_stmt(*s.else_body);
    }

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = end_lbl; emit(lbl); }
}

void ir_gen::visit(while_stmt &s) {
    std::string cond_lbl = new_label();
    std::string body_lbl = new_label();
    std::string end_lbl  = new_label();

    auto saved_break = break_lbl_;
    auto saved_cont  = cont_lbl_;
    break_lbl_ = end_lbl;
    cont_lbl_  = cond_lbl;

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = cond_lbl; emit(lbl); }
    emit_cond_branch(*s.cond, body_lbl, end_lbl);

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = body_lbl; emit(lbl); }
    gen_stmt(*s.body);
    { icode jmp; jmp.op = icode_op::GOTO; jmp.label_name = cond_lbl; emit(jmp); }

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = end_lbl; emit(lbl); }

    break_lbl_ = saved_break;
    cont_lbl_  = saved_cont;
}

void ir_gen::visit(do_while_stmt &s) {
    std::string body_lbl = new_label();
    std::string cond_lbl = new_label();
    std::string end_lbl  = new_label();

    auto saved_break = break_lbl_;
    auto saved_cont  = cont_lbl_;
    break_lbl_ = end_lbl;
    cont_lbl_  = cond_lbl;

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = body_lbl; emit(lbl); }
    gen_stmt(*s.body);
    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = cond_lbl; emit(lbl); }
    emit_cond_branch(*s.cond, body_lbl, end_lbl);
    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = end_lbl; emit(lbl); }

    break_lbl_ = saved_break;
    cont_lbl_  = saved_cont;
}

void ir_gen::visit(for_stmt &s) {
    std::string cond_lbl = new_label();
    std::string body_lbl = new_label();
    std::string step_lbl = new_label();
    std::string end_lbl  = new_label();

    auto saved_break = break_lbl_;
    auto saved_cont  = cont_lbl_;
    break_lbl_ = end_lbl;
    cont_lbl_  = step_lbl;

    if (s.init) gen_stmt(*s.init);

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = cond_lbl; emit(lbl); }
    if (s.cond) {
        emit_cond_branch(*s.cond, body_lbl, end_lbl);
    }

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = body_lbl; emit(lbl); }
    gen_stmt(*s.body);

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = step_lbl; emit(lbl); }
    if (s.step) gen_expr(*s.step);
    { icode jmp; jmp.op = icode_op::GOTO; jmp.label_name = cond_lbl; emit(jmp); }

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = end_lbl; emit(lbl); }

    break_lbl_ = saved_break;
    cont_lbl_  = saved_cont;
}

void ir_gen::visit(break_stmt &) {
    icode ic; ic.op = icode_op::GOTO; ic.label_name = break_lbl_; emit(ic);
}

void ir_gen::visit(continue_stmt &) {
    icode ic; ic.op = icode_op::GOTO; ic.label_name = cont_lbl_; emit(ic);
}

void ir_gen::visit(goto_stmt &s) {
    icode ic; ic.op = icode_op::GOTO; ic.label_name = s.label; emit(ic);
}

void ir_gen::visit(label_stmt &s) {
    { icode ic; ic.op = icode_op::LABEL; ic.label_name = s.name; emit(ic); }
    if (s.body) gen_stmt(*s.body);
}

void ir_gen::visit(switch_stmt &s) {
    struct case_entry {
        int64_t     value;
        bool        is_default;
        std::string label;
        case_stmt  *node;
    };
    std::vector<case_entry> cases;

    auto *body_cs = dynamic_cast<compound_stmt*>(s.body.get());
    if (body_cs) {
        for (auto &st : body_cs->body) {
            auto *c = dynamic_cast<case_stmt*>(st.get());
            if (!c) continue;
            case_entry e;
            e.is_default = c->is_default;
            e.node  = c;
            e.label = new_label();
            if (!c->is_default && c->value) {
                operand v = gen_expr(*c->value);
                e.value = v.ival;
            } else {
                e.value = 0;
            }
            cases.push_back(e);
        }
    }

    operand cond = gen_expr(*s.cond);
    std::string end_lbl     = new_label();
    std::string default_lbl = end_lbl;

    for (auto &e : cases) {
        if (e.is_default) {
            default_lbl = e.label;
        } else {
            operand eq = emit_binop(icode_op::EQ, cond,
                                    operand::make_int(e.value, type::make_int()),
                                    type::make_int());
            { icode ic; ic.op = icode_op::IFX; ic.left = eq;
              ic.true_lbl = e.label; ic.false_lbl = ""; emit(ic); }
        }
    }
    { icode ic; ic.op = icode_op::GOTO; ic.label_name = default_lbl; emit(ic); }

    auto saved_break = break_lbl_;
    break_lbl_ = end_lbl;

    if (body_cs) {
        for (auto &st : body_cs->body) {
            auto *c = dynamic_cast<case_stmt*>(st.get());
            if (c) {
                for (auto &e : cases) {
                    if (e.node == c) {
                        icode lbl; lbl.op = icode_op::LABEL;
                        lbl.label_name = e.label; emit(lbl);
                        break;
                    }
                }
            } else {
                gen_stmt(*st);
            }
        }
    } else if (s.body) {
        gen_stmt(*s.body);
    }

    { icode ic; ic.op = icode_op::LABEL; ic.label_name = end_lbl; emit(ic); }
    break_lbl_ = saved_break;
}

void ir_gen::visit(case_stmt &s) {
    (void)s;
}

void ir_gen::visit(asm_stmt &s) {
    icode ic;
    ic.op       = icode_op::INLINE_ASM;
    ic.asm_text = s.text;
    emit(ic);
}

} // namespace xcc
