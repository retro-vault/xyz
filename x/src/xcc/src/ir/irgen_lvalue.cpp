//
// irgen_lvalue.cpp — lvalue read/write and member access for the xcc IR pass.
//
// Covers: gen_lvalue_write, visit(index_expr), gen_member_ptr,
// visit(member_expr), and the find_member helper.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"

namespace xcc {

namespace {

uint64_t bitfield_mask_bits(int width) {
    if (width <= 0)
        return 0;
    if (width >= 64)
        return ~uint64_t{0};
    return (uint64_t{1} << width) - 1;
}

// Build a pointer-to-elem type that inherits far-ness from the base
// pointer/array, so far pointer arithmetic (p+i, p[i], p->f) keeps the
// result far and routes its dereference through the far trampoline.
static type_ptr elem_ptr_like(const type_ptr &base, type_ptr elem) {
    if (base && base->is_far_ptr())
        return type::make_far_pointer(std::move(elem));
    return type::make_pointer(std::move(elem));
}

static bool is_subscriptable_type(const type_ptr &ty) {
    type_ptr t = ty ? ty->unqual() : nullptr;
    return t && (t->is_ptr() || t->is_array());
}

static void normalize_subscript_operands(operand &base, operand &index) {
    if (!is_subscriptable_type(base.type) && is_subscriptable_type(index.type))
        std::swap(base, index);
}

} // namespace

static const struct_field *find_member(const member_expr &e) {
    type_ptr st = e.object->type;
    if (e.is_arrow && st && st->is_ptr()) st = st->base;
    if (!st) return nullptr;
    for (auto &f : st->fields)
        if (f.name == e.member) return &f;
    return nullptr;
}

operand ir_gen::gen_lvalue_write(expr &lhs, operand src) {
    auto coerce_for_store = [&](operand value, const type_ptr &target) -> operand {
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

    if (auto *id = dynamic_cast<ident_expr*>(&lhs)) {
        if (id->sym) {
            operand dst = sym_to_operand(*id->sym, id->type);
            src = coerce_for_store(src, dst.type);
            emit_assign(dst, src);
            return dst;
        }
    }
    if (auto *deref = dynamic_cast<unary_expr*>(&lhs)) {
        if (deref->op == unary_op::DEREF) {
            operand ptr = gen_expr(*deref->operand);
            src = coerce_for_store(src, deref->type ? deref->type : lhs.type);
            icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = ptr; ic.left = src; emit(ic);
            return src;
        }
    }
    if (auto *mem = dynamic_cast<member_expr*>(&lhs)) {
        const struct_field *fld = find_member(*mem);
        operand ptr = gen_member_ptr(*mem);
        if (fld && fld->bit_width >= 0) {
            type_ptr unit_type = fld->type ? fld->type : type::make_int();
            int64_t  mask      =
                static_cast<int64_t>(bitfield_mask_bits(fld->bit_width));
            operand m_src = emit_binop(icode_op::BAND, src,
                                       operand::make_int(mask, type::make_int()), unit_type);
            operand s_src = m_src;
            if (fld->bit_offset > 0)
                s_src = emit_binop(icode_op::SHL, m_src,
                                   operand::make_int(fld->bit_offset, type::make_int()), unit_type);
            operand cur     = emit_unop(icode_op::GET_VALUE_AT, ptr, unit_type);
            int64_t clr_mask = ~(mask << fld->bit_offset);
            operand cleared  = emit_binop(icode_op::BAND, cur,
                                          operand::make_int(clr_mask, type::make_int()), unit_type);
            operand combined = emit_binop(icode_op::BOR, cleared, s_src, unit_type);
            icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = ptr; ic.left = combined; emit(ic);
        } else {
            src = coerce_for_store(src, fld ? fld->type : (mem->type ? mem->type : lhs.type));
            icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = ptr; ic.left = src; emit(ic);
        }
        return src;
    }
    if (auto *idx = dynamic_cast<index_expr*>(&lhs)) {
        operand base  = gen_expr(*idx->base);
        operand index = gen_expr(*idx->index);
        if (base.type && base.type->is_array() && base.type->base) {
            // Array expressions decay to element pointers before index arithmetic.
            base = emit_unop(icode_op::ADDRESS_OF, base,
                             type::make_pointer(base.type->base));
        }
        if (index.type && index.type->is_array() && index.type->base) {
            index = emit_unop(icode_op::ADDRESS_OF, index,
                              type::make_pointer(index.type->base));
        }
        normalize_subscript_operands(base, index);
        if (index.type && index.type->is_integer() &&
            index.type->size() < type::make_int()->size()) {
            index = emit_unop(icode_op::CAST, index, type::make_int());
        }
        type_ptr elem_type = idx->type ? idx->type : type::make_int();
        int elem_sz = elem_type->size();
        if (elem_sz > 1) {
            operand scale = operand::make_int(elem_sz, type::make_int());
            index = emit_binop(icode_op::MUL, index, scale, index.type);
        }
        operand addr = emit_binop(icode_op::ADD, base, index, elem_ptr_like(base.type, elem_type));
        src = coerce_for_store(src, elem_type);
        icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = addr; ic.left = src; emit(ic);
        return src;
    }
    return src;
}

void ir_gen::visit(index_expr &e) {
    operand base  = gen_expr(*e.base);
    operand index = gen_expr(*e.index);
    if (base.type && base.type->is_array() && base.type->base) {
        // Array expressions decay to element pointers before index arithmetic.
        base = emit_unop(icode_op::ADDRESS_OF, base,
                         type::make_pointer(base.type->base));
    }
    if (index.type && index.type->is_array() && index.type->base) {
        index = emit_unop(icode_op::ADDRESS_OF, index,
                          type::make_pointer(index.type->base));
    }
    normalize_subscript_operands(base, index);
    if (index.type && index.type->is_integer() &&
        index.type->size() < type::make_int()->size()) {
        index = emit_unop(icode_op::CAST, index, type::make_int());
    }

    type_ptr elem_type = e.type ? e.type : type::make_int();
    int elem_sz = elem_type->size();
    if (elem_sz > 1) {
        operand scale = operand::make_int(elem_sz, type::make_int());
        index = emit_binop(icode_op::MUL, index, scale, index.type);
    }

    operand ptr = emit_binop(icode_op::ADD, base, index, elem_ptr_like(base.type, elem_type));
    if (elem_type && elem_type->is_array() && elem_type->base) {
        // A subscript of a pointer-to-array yields an array lvalue.  In value
        // context that lvalue decays to a pointer to the first element at the
        // same address; do not load a temporary row object.
        ptr.type = elem_ptr_like(base.type, elem_type->base);
        expr_result_ = ptr;
        return;
    }
    expr_result_ = emit_unop(icode_op::GET_VALUE_AT, ptr, elem_type);
}

operand ir_gen::gen_member_ptr(member_expr &e) {
    type_ptr struct_type = e.object->type;
    if (e.is_arrow && struct_type && struct_type->is_ptr())
        struct_type = struct_type->base;

    int      field_offset = 0;
    type_ptr field_type   = e.type ? e.type : type::make_int();
    if (struct_type) {
        for (auto &f : struct_type->fields) {
            if (f.name == e.member) {
                field_offset = f.offset;
                field_type   = f.type;
                break;
            }
        }
    }

    type_ptr struct_ptr_type =
        type::make_pointer(struct_type ? struct_type : field_type);
    operand ptr;
    if (e.is_arrow) {
        ptr = gen_expr(*e.object);
    } else {
        ptr = gen_lvalue_addr(*e.object, struct_ptr_type);
    }

    // A far struct pointer yields far field pointers, so the member access
    // dereferences through the far trampoline.
    type_ptr field_ptr_type = elem_ptr_like(ptr.type, field_type);

    if (field_offset != 0) {
        operand off = operand::make_int(field_offset, type::make_int());
        ptr = emit_binop(icode_op::ADD, ptr, off, field_ptr_type);
    } else {
        ptr.type = field_ptr_type;
    }
    return ptr;
}

operand ir_gen::gen_lvalue_addr(expr &e, type_ptr ptr_t) {
    if (auto *id = dynamic_cast<ident_expr*>(&e)) {
        if (id->sym) {
            operand obj = sym_to_operand(*id->sym, id->type);
            return emit_unop(icode_op::ADDRESS_OF, obj, ptr_t);
        }
    }

    if (auto *deref = dynamic_cast<unary_expr*>(&e)) {
        if (deref->op == unary_op::DEREF)
            return gen_expr(*deref->operand);
    }

    if (auto *idx = dynamic_cast<index_expr*>(&e)) {
        operand base = gen_expr(*idx->base);
        operand index = gen_expr(*idx->index);
        if (base.type && base.type->is_array() && base.type->base) {
            base = emit_unop(icode_op::ADDRESS_OF, base,
                             type::make_pointer(base.type->base));
        }
        if (index.type && index.type->is_array() && index.type->base) {
            index = emit_unop(icode_op::ADDRESS_OF, index,
                              type::make_pointer(index.type->base));
        }
        normalize_subscript_operands(base, index);
        if (index.type && index.type->is_integer() &&
            index.type->size() < type::make_int()->size()) {
            index = emit_unop(icode_op::CAST, index, type::make_int());
        }

        type_ptr elem_type = e.type ? e.type : type::make_int();
        int elem_sz = elem_type->size();
        if (elem_sz > 1) {
            operand scale = operand::make_int(elem_sz, type::make_int());
            index = emit_binop(icode_op::MUL, index, scale, index.type);
        }

        type_ptr res_t = (base.type && base.type->is_far_ptr())
                             ? type::make_far_pointer(elem_type)
                             : (ptr_t ? ptr_t : type::make_pointer(elem_type));
        return emit_binop(icode_op::ADD, base, index, res_t);
    }

    if (auto *mem = dynamic_cast<member_expr*>(&e))
        return gen_member_ptr(*mem);

    operand obj = gen_expr(e);
    return emit_unop(icode_op::ADDRESS_OF, obj, ptr_t);
}

void ir_gen::visit(member_expr &e) {
    const struct_field *fld = find_member(e);
    operand ptr      = gen_member_ptr(e);
    type_ptr fld_type = e.type ? e.type : type::make_int();

    if (fld_type && fld_type->is_array() && fld_type->base) {
        ptr.type = type::make_pointer(fld_type->base);
        expr_result_ = ptr;
        return;
    }

    operand loaded   = emit_unop(icode_op::GET_VALUE_AT, ptr, fld_type);

    if (fld && fld->bit_width >= 0) {
        if (fld->bit_offset > 0) {
            operand off_op = operand::make_int(fld->bit_offset, type::make_int());
            loaded = emit_binop(icode_op::SHR, loaded, off_op, fld_type);
        }
        int64_t mask =
            static_cast<int64_t>(bitfield_mask_bits(fld->bit_width));
        operand mask_op = operand::make_int(mask, type::make_int());
        loaded = emit_binop(icode_op::BAND, loaded, mask_op, fld_type);
        if (fld_type && fld_type->is_integer() &&
            !fld_type->is_unsigned() &&
            fld->bit_width > 0 &&
            fld->bit_width < fld_type->size() * 8) {
            const int64_t sign_bit =
                static_cast<int64_t>(uint64_t{1} << (fld->bit_width - 1));
            operand sign_op = operand::make_int(sign_bit, fld_type);
            loaded = emit_binop(icode_op::BXOR, loaded, sign_op, fld_type);
            loaded = emit_binop(icode_op::SUB, loaded, sign_op, fld_type);
        }
    }
    expr_result_ = loaded;
}

} // namespace xcc
