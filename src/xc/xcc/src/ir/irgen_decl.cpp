//
// irgen_decl.cpp — declaration visitors for the xcc IR lowering pass.
//
// Covers: typedef_decl, struct_decl, param_decl, var_decl, func_decl.
// Global aggregate initialiser collection (collect_global_inits) lives
// here because it is only needed when lowering global var_decl nodes.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"
#include <cassert>

namespace xcc {

// ----- Global aggregate init helpers ---------------------------------

static void collect_global_inits(const init_list_expr &il, type_ptr agg_type,
                                  std::vector<ir_module::global_var::init_elem> &out)
{
    type_ptr elem_type;
    const std::vector<struct_field> *fields = nullptr;

    if (agg_type) {
        if (agg_type->kind == type_kind::ARRAY && agg_type->base)
            elem_type = agg_type->base;
        else if (agg_type->kind == type_kind::STRUCT ||
                 agg_type->kind == type_kind::UNION)
            fields = &agg_type->fields;
    }

    for (size_t i = 0; i < il.elements.size(); ++i) {
        auto &e = il.elements[i];
        const expr *elem = e.value.get();
        type_ptr et = elem_type;
        if (fields) {
            if (e.field_name) {
                for (size_t fi = 0; fi < fields->size(); ++fi)
                    if ((*fields)[fi].name == *e.field_name) { et = (*fields)[fi].type; break; }
            } else if (i < fields->size()) {
                et = (*fields)[i].type;
            }
        }

        if (auto *sub = dynamic_cast<const init_list_expr*>(elem)) {
            collect_global_inits(*sub, et, out);
        } else if (auto *lit = dynamic_cast<const int_literal_expr*>(elem)) {
            int sz = et ? et->size() : 2;
            out.push_back({lit->value, sz});
        } else {
            int sz = et ? et->size() : 2;
            out.push_back({0, sz});
        }
    }
}

// ----- Declaration visitors ------------------------------------------

void ir_gen::visit(typedef_decl &) {}
void ir_gen::visit(struct_decl  &) {}
void ir_gen::visit(param_decl   &) {}

void ir_gen::visit(var_decl &vd) {
    if (!vd.sym) return;

    if (vd.vla_size && cur_fn_) {
        operand count = gen_expr(*vd.vla_size);
        int elem_sz = vd.type->is_ptr() ? vd.type->base->size() : 1;
        operand bytes;
        if (elem_sz == 1) {
            bytes = count;
        } else {
            bytes = new_temp(type::make_uint());
            icode ic;
            ic.op     = icode_op::MUL;
            ic.result = bytes;
            ic.left   = count;
            ic.right  = operand::make_int(elem_sz, type::make_uint());
            emit(ic);
        }
        if (vd.sym->vla_size_sym) {
            emit_assign(sym_to_operand(*vd.sym->vla_size_sym, type::make_uint()), bytes);
        }
        operand ptr_res = new_temp(vd.type);
        icode ic2;
        ic2.op     = icode_op::ALLOCA;
        ic2.result = ptr_res;
        ic2.left   = bytes;
        emit(ic2);
        emit_assign(sym_to_operand(*vd.sym, vd.type), ptr_res);
        return;
    }

    if (vd.sym->is_global) {
        if (vd.sym->storage == storage_class::EXTERN) return;

        ir_module::global_var gv;
        gv.name      = vd.name;
        gv.type      = vd.type;
        gv.has_init  = vd.init != nullptr;
        gv.is_tls    = vd.sym->is_tls;
        gv.is_static = (vd.sym->storage == storage_class::STATIC);
        if (vd.init) {
            if (auto *il = dynamic_cast<init_list_expr*>(vd.init.get())) {
                collect_global_inits(*il, vd.type, gv.init_vals);
            } else if (auto *lit = dynamic_cast<int_literal_expr*>(vd.init.get())) {
                gv.init_val = lit->value;
            }
        }
        mod_->globals.push_back(std::move(gv));
        return;
    }

    if (vd.init && cur_fn_) {
        if (auto *il = dynamic_cast<init_list_expr*>(vd.init.get())) {
            gen_init_list(*vd.sym, vd.type, *il);
        } else {
            operand dst = sym_to_operand(*vd.sym, vd.type);
            operand src = gen_expr(*vd.init);
            emit_assign(dst, src);
        }
    }
}

void ir_gen::visit(func_decl &fd) {
    gen_func(fd);
}

void ir_gen::gen_func(func_decl &fd) {
    if (!fd.body) return;

    ir_function fn;
    fn.name        = fd.name;
    fn.is_global   = (fd.storage != storage_class::STATIC);
    fn.ret_type    = fd.type ? fd.type->ret : type::make_void();
    fn.local_bytes = fd.local_bytes;
    fn.num_params  = static_cast<int>(fd.params.size());

    mod_->functions.push_back(std::move(fn));
    cur_fn_ = &mod_->functions.back();

    {
        icode ic;
        ic.op          = icode_op::FUNCTION;
        ic.func_name   = fd.name;
        ic.num_params  = static_cast<int>(fd.params.size());
        ic.local_bytes = fd.local_bytes;
        emit(ic);
    }

    for (int i = 0; i < static_cast<int>(fd.params.size()); ++i) {
        auto &p = fd.params[i];
        if (!p->sym) continue;
        operand dst = sym_to_operand(*p->sym, p->type);
        icode ic;
        ic.op     = icode_op::RECEIVE;
        ic.result = dst;
        ic.argreg = i;
        emit(ic);
    }

    gen_stmt(*fd.body);

    {
        icode ic;
        ic.op        = icode_op::ENDFUNCTION;
        ic.func_name = fd.name;
        emit(ic);
    }

    cur_fn_ = nullptr;
}

} // namespace xcc
