//
// irgen_init.cpp — aggregate initialiser lowering for the xcc IR pass.
//
// Covers: visit(compound_literal_expr), visit(init_list_expr), gen_init_list.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"
#include "frontend/const_eval.h"

#include <functional>

namespace xcc {

namespace {

ir_module::global_var::init_elem init_elem(int64_t value, int size,
                                           std::string label = {}) {
    ir_module::global_var::init_elem elem;
    elem.value = value;
    elem.size = size;
    elem.label = std::move(label);
    return elem;
}

bool is_char_array_type(type_ptr ty) {
    if (!ty || ty->kind != type_kind::ARRAY || !ty->base)
        return false;
    type_kind elem = ty->base->unqual()->kind;
    return elem == type_kind::CHAR ||
           elem == type_kind::UCHAR ||
           elem == type_kind::CHAR8T;
}

void append_string_bytes(const string_literal_expr &str, type_ptr array_type,
                         std::vector<ir_module::global_var::init_elem> &out) {
    int n = array_type ? array_type->size() : 0;
    int init_bytes = static_cast<int>(str.value.size()) + 1;
    if (n > 0 && init_bytes > n)
        init_bytes = n;
    for (int i = 0; i < init_bytes; ++i) {
        int ch = 0;
        if (i < static_cast<int>(str.value.size()))
            ch = static_cast<unsigned char>(str.value[i]);
        out.push_back(init_elem(ch, 1));
    }
    for (int i = init_bytes; i < n; ++i)
        out.push_back(init_elem(0, 1));
}

int64_t narrow_static_int(int64_t value, type_ptr ty) {
    if (!ty)
        return value;
    if (ty->kind == type_kind::BOOL)
        return value != 0 ? 1 : 0;
    int bytes = ty->size();
    if (bytes <= 0 || bytes >= 8)
        return value;
    uint64_t mask = (uint64_t{1} << (bytes * 8)) - 1;
    return static_cast<int64_t>(static_cast<uint64_t>(value) & mask);
}

void collect_static_init(expr *init, type_ptr target, ir_module &mod,
                         int &next_lbl,
                         std::vector<ir_module::global_var::init_elem> &out) {
    if (!init || !target)
        return;

    if (auto *str = dynamic_cast<string_literal_expr*>(init);
        str && is_char_array_type(target)) {
        append_string_bytes(*str, target, out);
        return;
    }

    if (auto *il = dynamic_cast<init_list_expr*>(init)) {
        if (is_char_array_type(target) && il->elements.size() == 1) {
            if (auto *str = dynamic_cast<string_literal_expr*>(
                    il->elements[0].value.get())) {
                append_string_bytes(*str, target, out);
                return;
            }
        }

        if (target->kind == type_kind::ARRAY && target->base) {
            type_ptr elem_t = target->base;
            int64_t next_idx = 0;
            int64_t emitted = 0;
            for (auto &e : il->elements) {
                int64_t idx = e.array_index ? *e.array_index : next_idx;
                while (emitted < idx) {
                    out.push_back(init_elem(0, elem_t->size()));
                    ++emitted;
                }
                collect_static_init(e.value.get(), elem_t, mod, next_lbl, out);
                ++emitted;
                next_idx = idx + 1;
            }
            while (target->array_size > 0 && emitted < target->array_size) {
                out.push_back(init_elem(0, elem_t->size()));
                ++emitted;
            }
            return;
        }

        if ((target->kind == type_kind::STRUCT ||
             target->kind == type_kind::UNION) &&
            !target->fields.empty()) {
            size_t seq_idx = 0;
            for (auto &e : il->elements) {
                const struct_field *fld = nullptr;
                if (e.field_name) {
                    for (size_t fi = 0; fi < target->fields.size(); ++fi) {
                        if (target->fields[fi].name == *e.field_name) {
                            fld = &target->fields[fi];
                            seq_idx = fi + 1;
                            break;
                        }
                    }
                }
                if (!fld && seq_idx < target->fields.size())
                    fld = &target->fields[seq_idx++];
                if (fld)
                    collect_static_init(e.value.get(), fld->type, mod, next_lbl, out);
            }
            return;
        }

        if (!il->elements.empty())
            collect_static_init(il->elements[0].value.get(), target, mod, next_lbl, out);
        return;
    }

    if (target->is_integer()) {
        if (auto cv = const_expr_evaluator::evaluate(init)) {
            out.push_back(init_elem(narrow_static_int(*cv, target),
                                    target->size()));
            return;
        }
    }

    if (auto *lit = dynamic_cast<int_literal_expr*>(init)) {
        out.push_back(init_elem(narrow_static_int(lit->value, target),
                                target->size()));
    } else if (auto *lit = dynamic_cast<char_literal_expr*>(init)) {
        out.push_back(init_elem(narrow_static_int(lit->value, target),
                                target->size()));
    } else if (auto *flit = dynamic_cast<float_literal_expr*>(init)) {
        out.push_back(init_elem(encode_float_constant(flit->value, target),
                                target->size()));
    } else if (auto *str = dynamic_cast<string_literal_expr*>(init);
               str && target->kind == type_kind::POINTER) {
        std::string lbl = "__str_" + std::to_string(next_lbl++);
        ir_module::global_var gv;
        gv.name       = lbl;
        gv.type       = type::make_array(type::make_char(),
                        static_cast<int>(str->value.size()) + 1);
        gv.str_init   = str->value;
        gv.char_width = str->char_width;
        gv.has_init   = true;
        mod.string_literals.push_back(std::move(gv));
        out.push_back(init_elem(0, target->size(), lbl));
    } else {
        out.push_back(init_elem(0, target->size()));
    }
}

} // namespace

void ir_gen::visit(compound_literal_expr &e) {
    if (e.sym && e.init) {
        if (e.sym->is_global) {
            ir_module::global_var gv;
            gv.name = !e.sym->asm_name.empty() ? e.sym->asm_name : e.sym->name;
            gv.type = e.type;
            gv.has_init = true;
            gv.is_static = true;
            collect_static_init(e.init.get(), e.type, *mod_, next_lbl_, gv.init_vals);
            mod_->globals.push_back(std::move(gv));
        } else if (auto *il = dynamic_cast<init_list_expr*>(e.init.get())) {
            gen_init_list(*e.sym, e.type, *il);
        } else {
            emit_assign(sym_to_operand(*e.sym, e.type), gen_expr(*e.init));
        }
    }

    if (e.sym && e.type && e.type->is_array() && e.type->base) {
        operand obj = sym_to_operand(*e.sym, e.type);
        expr_result_ = emit_unop(icode_op::ADDRESS_OF, obj,
                                 type::make_pointer(e.type->base));
        return;
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

    auto coerce_init_store = [&](operand value, const type_ptr &target) -> operand {
        if (!target)
            return value;
        if (!value.type) {
            value.type = target;
            return value;
        }
        bool same_type =
            value.type->kind == target->kind &&
            value.type->size() == target->size() &&
            value.type->is_unsigned() == target->is_unsigned();
        if (same_type) {
            value.type = target;
            return value;
        }
        value = coerce_const_operand(value, target);
        if (value.kind == operand_kind::INT_CONST ||
            value.kind == operand_kind::FLOAT_CONST)
            return value;
        return emit_unop(icode_op::CAST, value, target);
    };

    operand base_sym = sym_to_operand(sym, type);
    operand base_ptr = new_temp(type::make_pointer(type));
    { icode ic; ic.op = icode_op::ADDRESS_OF; ic.result = base_ptr; ic.left = base_sym; emit(ic); }

    auto ptr_at = [&](const operand &ptr, int64_t offset,
                      const type_ptr &pointee) -> operand {
        if (offset == 0)
            return ptr;
        operand out = new_temp(type::make_pointer(pointee));
        operand off = operand::make_int(offset, type::make_int());
        icode ic; ic.op = icode_op::ADD; ic.result = out; ic.left = ptr; ic.right = off; emit(ic);
        return out;
    };

    std::function<void(const operand&, const type_ptr&, init_list_expr&)> emit_init_at;
    emit_init_at = [&](const operand &dst_ptr, const type_ptr &dst_type,
                       init_list_expr &list) {
        if (!dst_type)
            return;

        if (dst_type->kind == type_kind::ARRAY && dst_type->base) {
            type_ptr elem_t  = dst_type->base;
            int      elem_sz = elem_t->size();

            if (elem_sz == 1 && list.elements.size() == 1) {
                if (auto *str = dynamic_cast<string_literal_expr*>(
                        list.elements[0].value.get())) {
                    int n = dst_type->size();
                    int init_bytes = static_cast<int>(str->value.size()) + 1;
                    if (init_bytes > n)
                        init_bytes = n;
                    for (int i = 0; i < init_bytes; ++i) {
                        int ch = 0;
                        if (i < static_cast<int>(str->value.size()))
                            ch = static_cast<unsigned char>(str->value[i]);
                        operand elem_ptr = ptr_at(dst_ptr, i, elem_t);
                        icode ic;
                        ic.op = icode_op::SET_VALUE_AT;
                        ic.result = elem_ptr;
                        ic.left = operand::make_int(ch, elem_t);
                        emit(ic);
                    }
                    return;
                }
            }

            int64_t  cur_idx = 0;
            for (auto &e : list.elements) {
                if (e.array_index) cur_idx = *e.array_index;
                operand elem_ptr = ptr_at(dst_ptr, cur_idx * elem_sz, elem_t);
                if (auto *sub = dynamic_cast<init_list_expr*>(e.value.get());
                    sub && (elem_t->kind == type_kind::ARRAY ||
                            elem_t->kind == type_kind::STRUCT ||
                            elem_t->kind == type_kind::UNION)) {
                    emit_init_at(elem_ptr, elem_t, *sub);
                } else {
                    operand val = coerce_init_store(gen_expr(*e.value), elem_t);
                    icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = elem_ptr; ic.left = val; emit(ic);
                }
                ++cur_idx;
            }
        } else if ((dst_type->kind == type_kind::STRUCT ||
                    dst_type->kind == type_kind::UNION) &&
                   !dst_type->fields.empty()) {
            size_t seq_idx = 0;
            for (auto &e : list.elements) {
                const struct_field *fld = nullptr;
                if (e.field_name) {
                    for (size_t fi = 0; fi < dst_type->fields.size(); ++fi) {
                        if (dst_type->fields[fi].name == *e.field_name) {
                            fld = &dst_type->fields[fi];
                            seq_idx = fi + 1;
                            break;
                        }
                    }
                }
                if (!fld && seq_idx < dst_type->fields.size())
                    fld = &dst_type->fields[seq_idx++];
                if (!fld)
                    continue;

                operand fld_ptr = ptr_at(dst_ptr, fld->offset, fld->type);
                if (auto *sub = dynamic_cast<init_list_expr*>(e.value.get());
                    sub && (fld->type->kind == type_kind::ARRAY ||
                            fld->type->kind == type_kind::STRUCT ||
                            fld->type->kind == type_kind::UNION)) {
                    emit_init_at(fld_ptr, fld->type, *sub);
                } else {
                    operand val = coerce_init_store(gen_expr(*e.value), fld->type);
                    icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = fld_ptr; ic.left = val; emit(ic);
                }
            }
        } else if (!list.elements.empty()) {
            operand src = coerce_init_store(gen_expr(*list.elements[0].value), dst_type);
            icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = dst_ptr; ic.left = src; emit(ic);
        }
    };

    emit_init_at(base_ptr, type, il);
}

} // namespace xcc
