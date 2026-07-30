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

#include <algorithm>
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
           elem == type_kind::SCHAR ||
           elem == type_kind::UCHAR ||
           elem == type_kind::CHAR8T;
}

bool is_char_pointer_type(type_ptr ty) {
    if (!ty || ty->kind != type_kind::POINTER || !ty->base)
        return false;
    type_kind elem = ty->base->unqual()->kind;
    return elem == type_kind::CHAR ||
           elem == type_kind::SCHAR ||
           elem == type_kind::UCHAR ||
           elem == type_kind::CHAR8T;
}

const string_literal_expr *unwrap_string_literal(expr *init) {
    while (init) {
        if (auto *str = dynamic_cast<string_literal_expr*>(init))
            return str;
        auto *cast = dynamic_cast<cast_expr*>(init);
        if (!cast || !cast->operand)
            break;
        init = cast->operand.get();
    }
    return nullptr;
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

uint64_t bitfield_mask_bits(int width) {
    if (width <= 0)
        return 0;
    if (width >= 64)
        return ~uint64_t{0};
    return (uint64_t{1} << width) - 1;
}

struct aggregate_init_slot {
    struct_field field;
};

void collect_flat_init_slots(type_ptr target, int base_offset,
                             std::vector<aggregate_init_slot> &out) {
    if (!target)
        return;

    if (target->kind == type_kind::ARRAY && target->base) {
        const int elem_size = target->base->size();
        for (int i = 0; i < target->array_size; ++i) {
            collect_flat_init_slots(
                target->base, base_offset + i * elem_size, out);
        }
        return;
    }

    if ((target->kind == type_kind::STRUCT ||
         target->kind == type_kind::UNION) &&
        !target->fields.empty()) {
        std::vector<std::pair<int, int>> occupied;
        for (const auto &source_field : target->fields) {
            struct_field field = source_field;
            field.offset += base_offset;
            const int field_size =
                field.type ? std::max(field.type->size(), 1) : 1;
            const int begin = field.offset;
            const int end = begin + field_size;

            bool overlaps_prior_member = false;
            if (field.bit_width < 0) {
                for (const auto &[prior_begin, prior_end] : occupied) {
                    if (begin < prior_end && prior_begin < end) {
                        overlaps_prior_member = true;
                        break;
                    }
                }
            }
            if (overlaps_prior_member)
                continue;

            if (field.bit_width < 0)
                occupied.emplace_back(begin, end);

            if (field.bit_width < 0 && field.type &&
                (field.type->kind == type_kind::ARRAY ||
                 field.type->kind == type_kind::STRUCT ||
                 field.type->kind == type_kind::UNION)) {
                collect_flat_init_slots(field.type, field.offset, out);
            } else {
                out.push_back({field});
            }
        }
        return;
    }

    struct_field field;
    field.type = target;
    field.offset = base_offset;
    out.push_back({field});
}

bool is_flat_aggregate_initializer(const init_list_expr &list) {
    for (const auto &elem : list.elements) {
        if (elem.field_name || elem.array_index ||
            dynamic_cast<init_list_expr *>(elem.value.get())) {
            return false;
        }
    }
    return true;
}

void collect_static_init(expr *init, type_ptr target, ir_module &mod,
                         int &next_lbl,
                         std::vector<ir_module::global_var::init_elem> &out) {
    if (!init || !target)
        return;

    if (const auto *str = unwrap_string_literal(init);
        str && is_char_array_type(target)) {
        append_string_bytes(*str, target, out);
        return;
    }

    if (auto *il = dynamic_cast<init_list_expr*>(init)) {
        if (is_char_array_type(target) && il->elements.size() == 1) {
            if (const auto *str =
                    unwrap_string_literal(il->elements[0].value.get())) {
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
            if (is_flat_aggregate_initializer(*il)) {
                std::vector<aggregate_init_slot> slots;
                collect_flat_init_slots(target, 0, slots);
                if (std::none_of(
                        slots.begin(), slots.end(),
                        [](const aggregate_init_slot &slot) {
                            return slot.field.bit_width >= 0;
                        })) {
                    int cursor = 0;
                    const size_t count =
                        std::min(il->elements.size(), slots.size());
                    for (size_t i = 0; i < count; ++i) {
                        const auto &slot = slots[i].field;
                        while (cursor < slot.offset) {
                            out.push_back(init_elem(0, 1));
                            ++cursor;
                        }
                        std::vector<ir_module::global_var::init_elem> value;
                        collect_static_init(
                            il->elements[i].value.get(), slot.type,
                            mod, next_lbl, value);
                        for (auto &elem : value)
                            out.push_back(std::move(elem));
                        cursor = slot.offset + (slot.type ? slot.type->size() : 0);
                    }
                    while (cursor < target->size()) {
                        out.push_back(init_elem(0, 1));
                        ++cursor;
                    }
                    return;
                }
            }

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
        if (auto cv = const_expr_evaluator::evaluate_integer_conversion(
                init, target)) {
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
    } else if (const auto *str = unwrap_string_literal(init);
               str && is_char_pointer_type(target)) {
        std::string lbl = "__xcc_str_" + std::to_string(next_lbl++);
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

void ir_gen::gen_init_list(const symbol &sym, type_ptr type, init_list_expr &il,
                           const operand *base_override) {
    if (!type) return;

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

    operand base_ptr;
    if (base_override) {
        base_ptr = *base_override;
        base_ptr.type = type::make_pointer(type);
    } else {
        operand base_sym = sym_to_operand(sym, type);
        base_ptr = new_temp(type::make_pointer(type));
        icode ic;
        ic.op = icode_op::ADDRESS_OF;
        ic.result = base_ptr;
        ic.left = base_sym;
        emit(ic);
    }

    auto ptr_at = [&](const operand &ptr, int64_t offset,
                      const type_ptr &pointee) -> operand {
        if (offset == 0)
            return ptr;
        operand out = new_temp(type::make_pointer(pointee));
        operand off = operand::make_int(offset, type::make_int());
        icode ic; ic.op = icode_op::ADD; ic.result = out; ic.left = ptr; ic.right = off; emit(ic);
        return out;
    };

    auto zero_object = [&](const operand &dst_ptr, const type_ptr &dst_type) {
        if (!dst_type)
            return;
        const int size = dst_type->size();
        for (int i = 0; i < size; ++i) {
            operand byte_ptr = ptr_at(dst_ptr, i, type::make_uchar());
            icode ic;
            ic.op = icode_op::SET_VALUE_AT;
            ic.result = byte_ptr;
            ic.left = operand::make_int(0, type::make_uchar());
            emit(ic);
        }
    };

    auto emit_bitfield_store =
        [&](const operand &dst_ptr, const struct_field &field, operand value) {
            type_ptr unit_type = field.type ? field.type : type::make_int();
            operand field_ptr = ptr_at(dst_ptr, field.offset, unit_type);
            const int64_t mask =
                static_cast<int64_t>(bitfield_mask_bits(field.bit_width));

            value = coerce_init_store(value, field.type);
            operand masked = emit_binop(
                icode_op::BAND,
                value,
                operand::make_int(mask, type::make_int()),
                unit_type);
            if (field.bit_offset > 0) {
                masked = emit_binop(
                    icode_op::SHL,
                    masked,
                    operand::make_int(field.bit_offset, type::make_int()),
                    unit_type);
            }

            operand current = emit_unop(icode_op::GET_VALUE_AT, field_ptr, unit_type);
            const int64_t clear_mask = ~(mask << field.bit_offset);
            operand cleared = emit_binop(
                icode_op::BAND,
                current,
                operand::make_int(clear_mask, type::make_int()),
                unit_type);
            operand combined = emit_binop(icode_op::BOR, cleared, masked, unit_type);

            icode ic;
            ic.op = icode_op::SET_VALUE_AT;
            ic.result = field_ptr;
            ic.left = combined;
            emit(ic);
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
            if (is_flat_aggregate_initializer(list)) {
                std::vector<aggregate_init_slot> slots;
                collect_flat_init_slots(dst_type, 0, slots);
                const size_t count =
                    std::min(list.elements.size(), slots.size());
                for (size_t i = 0; i < count; ++i) {
                    const struct_field &field = slots[i].field;
                    operand value = gen_expr(*list.elements[i].value);
                    if (field.bit_width >= 0) {
                        emit_bitfield_store(dst_ptr, field, value);
                    } else {
                        operand field_ptr =
                            ptr_at(dst_ptr, field.offset, field.type);
                        value = coerce_init_store(value, field.type);
                        icode ic;
                        ic.op = icode_op::SET_VALUE_AT;
                        ic.result = field_ptr;
                        ic.left = value;
                        emit(ic);
                    }
                }
                return;
            }

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

                if (auto *sub = dynamic_cast<init_list_expr*>(e.value.get());
                    sub && (fld->type->kind == type_kind::ARRAY ||
                            fld->type->kind == type_kind::STRUCT ||
                            fld->type->kind == type_kind::UNION)) {
                    operand fld_ptr = ptr_at(dst_ptr, fld->offset, fld->type);
                    emit_init_at(fld_ptr, fld->type, *sub);
                } else if (fld->bit_width >= 0) {
                    emit_bitfield_store(dst_ptr, *fld, gen_expr(*e.value));
                } else {
                    operand fld_ptr = ptr_at(dst_ptr, fld->offset, fld->type);
                    operand val = coerce_init_store(gen_expr(*e.value), fld->type);
                    icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = fld_ptr; ic.left = val; emit(ic);
                }
                if (dst_type->kind == type_kind::UNION)
                    break;
            }
        } else if (!list.elements.empty()) {
            operand src = coerce_init_store(gen_expr(*list.elements[0].value), dst_type);
            icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = dst_ptr; ic.left = src; emit(ic);
        }
    };

    // Aggregate list-initialization zero-fills any members/elements not
    // explicitly mentioned before overlaying the provided initializers.
    zero_object(base_ptr, type);
    emit_init_at(base_ptr, type, il);
}

} // namespace xcc
