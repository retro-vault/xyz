//
// irgen_init.cpp — aggregate initialiser lowering for the xcc IR pass.
//
// Covers: visit(compound_literal_expr), visit(init_list_expr), gen_init_list.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"

namespace xcc {

void ir_gen::visit(compound_literal_expr &e) {
    if (e.sym && e.init) {
        if (auto *il = dynamic_cast<init_list_expr*>(e.init.get()))
            gen_init_list(*e.sym, e.type, *il);
        else
            emit_assign(sym_to_operand(*e.sym, e.type), gen_expr(*e.init));
    }
    expr_result_ = e.sym ? sym_to_operand(*e.sym, e.type)
                         : operand::make_int(0, e.type ? e.type : type::make_int());
}

void ir_gen::visit(init_list_expr &e) {
    if (!e.elements.empty())
        expr_result_ = gen_expr(*e.elements[0].value);
    else
        expr_result_ = operand::make_int(0, type::make_int());
}

void ir_gen::gen_init_list(const symbol &sym, type_ptr type, init_list_expr &il) {
    if (!type || il.elements.empty()) return;

    operand base_sym = sym_to_operand(sym, type);
    operand base_ptr = new_temp(type::make_pointer(type));
    { icode ic; ic.op = icode_op::ADDRESS_OF; ic.result = base_ptr; ic.left = base_sym; emit(ic); }

    if (type->kind == type_kind::ARRAY && type->base) {
        type_ptr elem_t  = type->base;
        int      elem_sz = elem_t->size();
        int64_t  cur_idx = 0;
        for (auto &e : il.elements) {
            if (e.array_index) cur_idx = *e.array_index;
            operand val = gen_expr(*e.value);
            if (cur_idx == 0) {
                icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = base_ptr; ic.left = val; emit(ic);
            } else {
                operand elem_ptr = new_temp(type::make_pointer(elem_t));
                operand off_op   = operand::make_int(cur_idx * elem_sz, type::make_int());
                { icode ic; ic.op = icode_op::ADD; ic.result = elem_ptr; ic.left = base_ptr; ic.right = off_op; emit(ic); }
                { icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = elem_ptr; ic.left = val; emit(ic); }
            }
            ++cur_idx;
        }
    } else if ((type->kind == type_kind::STRUCT || type->kind == type_kind::UNION)
               && !type->fields.empty()) {
        size_t seq_idx = 0;
        for (auto &e : il.elements) {
            const struct_field *fld = nullptr;
            if (e.field_name) {
                for (size_t fi = 0; fi < type->fields.size(); ++fi) {
                    if (type->fields[fi].name == *e.field_name) {
                        fld = &type->fields[fi];
                        seq_idx = fi + 1;
                        break;
                    }
                }
            }
            if (!fld && seq_idx < type->fields.size())
                fld = &type->fields[seq_idx++];
            if (!fld) continue;

            operand val = gen_expr(*e.value);
            if (fld->offset == 0) {
                icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = base_ptr; ic.left = val; emit(ic);
            } else {
                operand fld_ptr = new_temp(type::make_pointer(fld->type));
                operand off_op  = operand::make_int(fld->offset, type::make_int());
                { icode ic; ic.op = icode_op::ADD; ic.result = fld_ptr; ic.left = base_ptr; ic.right = off_op; emit(ic); }
                { icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = fld_ptr; ic.left = val; emit(ic); }
            }
        }
    } else {
        if (!il.elements.empty()) {
            operand src = gen_expr(*il.elements[0].value);
            emit_assign(base_sym, src);
        }
    }
}

} // namespace xcc
