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

// A bit-field wholly contained in one byte can be read or updated through
// that byte without touching the rest of its declared allocation unit.  This
// is both a smaller transaction and the natural Z80 lowering.  Volatile
// fields retain their declared access width, and crossing fields retain the
// complete unit because either case can make the access width observable.
static bool byte_contained_bitfield(const struct_field &field) {
    if (field.bit_width <= 0 || !field.type || field.type->is_volatile)
        return false;
    const int first_byte = field.bit_offset / 8;
    const int last_byte =
        (field.bit_offset + field.bit_width - 1) / 8;
    return first_byte == last_byte;
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
    type_ptr st = e.owner_type;
    if (!st) {
        st = e.object->type;
        if (e.is_arrow && st && st->is_ptr()) st = st->base;
    }
    if (!st) return nullptr;
    for (auto &f : st->fields)
        if (f.name == e.member) return &f;
    return nullptr;
}

operand ir_gen::gen_lvalue_write(expr &lhs, operand src,
                                 const operand *captured_address) {
    // Reusing an assignment's RHS value must not re-evaluate an observable
    // source, regardless of whether the destination is a symbol or indirect.
    if (src.is_symbol() &&
        (src.is_sfr || (src.type && src.type->is_volatile))) {
        operand value = new_temp(src.type ? src.type->unqual()
                                          : type::make_int());
        emit_assign(value, src);
        src = value;
    }
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
            value.type->is_unsigned() == target->is_unsigned() &&
            (target->kind != type_kind::BITINT ||
             value.type->bitint_width == target->bitint_width);
        if (same_type) {
            // A SYMBOL still denotes a memory access: arithmetic/store
            // coercion must retain the source object's volatile qualifier.
            if (!value.is_symbol() || !value.type->is_volatile)
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
            if (dst.is_sfr || (dst.type && dst.type->is_volatile)) {
                // An assignment (including prefix ++/--) yields the value
                // stored, not another evaluation of the destination object.
                // Snapshot symbolic sources before writing: the source may
                // alias the destination, and may itself be volatile or an SFR.
                if (src.is_symbol()) {
                    operand value = new_temp(dst.type ? dst.type->unqual()
                                                      : type::make_int());
                    emit_assign(value, src);
                    src = value;
                } else if (src.type) {
                    src.type = src.type->unqual();
                }
                emit_assign(dst, src);
                return src;
            }
            emit_assign(dst, src);
            return dst;
        }
    }
    if (auto *deref = dynamic_cast<unary_expr*>(&lhs)) {
        if (deref->op == unary_op::DEREF) {
            operand ptr = captured_address ? *captured_address
                                           : gen_expr(*deref->operand);
            src = coerce_for_store(src, deref->type ? deref->type : lhs.type);
            icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = ptr; ic.left = src; emit(ic);
            return src;
        }
    }
    if (auto *mem = dynamic_cast<member_expr*>(&lhs)) {
        const struct_field *fld = find_member(*mem);
        operand ptr = captured_address ? *captured_address
                                       : gen_member_ptr(*mem);
        if (fld && fld->bit_width >= 0) {
            // Apply the declared integer conversion before inserting the
            // field's bits.  A wide or floating RHS must not reach the
            // integer mask/store with its original representation.
            src = coerce_for_store(src, fld->type ? fld->type->unqual()
                                                  : type::make_int());
            if (src.is_symbol()) {
                operand value = new_temp(src.type ? src.type->unqual()
                                                  : type::make_int());
                emit_assign(value, src);
                src = value;
            }
            type_ptr unit_type = fld->type ? fld->type : type::make_int();
            int bit_offset = fld->bit_offset;
            if (byte_contained_bitfield(*fld)) {
                const int byte_offset = bit_offset / 8;
                unit_type = type::make_uchar();
                type_ptr byte_ptr_type = elem_ptr_like(ptr.type, unit_type);
                if (byte_offset != 0) {
                    ptr = emit_binop(
                        icode_op::ADD, ptr,
                        operand::make_int(byte_offset, type::make_int()),
                        byte_ptr_type);
                } else {
                    ptr.type = byte_ptr_type;
                }
                bit_offset %= 8;
            }
            const bool retain_insert =
                !(unit_type && unit_type->is_volatile) &&
                !(ptr.type && ptr.type->is_far_ptr());
            if (retain_insert) {
                icode ic;
                ic.op = icode_op::SET_VALUE_AT;
                ic.result = ptr;
                ic.left = src;
                ic.bit_width = fld->bit_width;
                ic.bit_offset = bit_offset;
                ic.bit_storage_bytes = unit_type->size();
                emit(ic);
            } else {
                // Volatile fields retain the declared load/store width and
                // far fields continue through the banked memory helpers.
                int64_t mask = static_cast<int64_t>(
                    bitfield_mask_bits(fld->bit_width));
                operand m_src = emit_binop(
                    icode_op::BAND, src,
                    operand::make_int(mask, type::make_int()), unit_type);
                operand s_src = m_src;
                if (bit_offset > 0)
                    s_src = emit_binop(
                        icode_op::SHL, m_src,
                        operand::make_int(bit_offset, type::make_int()),
                        unit_type);
                operand cur = emit_unop(
                    icode_op::GET_VALUE_AT, ptr, unit_type);
                int64_t clr_mask = ~(mask << bit_offset);
                operand cleared = emit_binop(
                    icode_op::BAND, cur,
                    operand::make_int(clr_mask, type::make_int()), unit_type);
                operand combined = emit_binop(
                    icode_op::BOR, cleared, s_src, unit_type);
                icode ic;
                ic.op = icode_op::SET_VALUE_AT;
                ic.result = ptr;
                ic.left = combined;
                emit(ic);
            }
        } else {
            src = coerce_for_store(src, fld ? fld->type : (mem->type ? mem->type : lhs.type));
            icode ic; ic.op = icode_op::SET_VALUE_AT; ic.result = ptr; ic.left = src; emit(ic);
        }
        if (fld && fld->bit_width >= 0) {
            // The assignment expression has the field's value after its
            // width conversion.  In particular, ++ on an unsigned bit-field
            // wraps at the field width even when its declared type is int.
            type_ptr value_type = fld->type ? fld->type->unqual()
                                            : type::make_int();
            const int64_t mask = static_cast<int64_t>(
                bitfield_mask_bits(fld->bit_width));
            src = emit_binop(icode_op::BAND, src,
                              operand::make_int(mask, value_type), value_type);
            if (!value_type->is_unsigned() && fld->bit_width > 0 &&
                fld->bit_width < value_type->size() * 8) {
                operand sign = operand::make_int(
                    static_cast<int64_t>(uint64_t{1} << (fld->bit_width - 1)),
                    value_type);
                src = emit_binop(icode_op::BXOR, src, sign, value_type);
                src = emit_binop(icode_op::SUB, src, sign, value_type);
            }
        }
        return src;
    }
    if (auto *idx = dynamic_cast<index_expr*>(&lhs)) {
        if (captured_address) {
            src = coerce_for_store(src, idx->type ? idx->type : type::make_int());
            icode ic; ic.op = icode_op::SET_VALUE_AT;
            ic.result = *captured_address; ic.left = src; emit(ic);
            return src;
        }
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
        if (index.type && index.type->kind == type_kind::BITINT) {
            type_ptr carrier = index.type->size() <= 2
                ? (index.type->is_unsigned() ? type::make_uint() : type::make_int())
                : (index.type->is_unsigned() ? type::make_ulong() : type::make_long());
            index = emit_unop(icode_op::CAST, index, carrier);
        } else if (index.type && index.type->is_integer() &&
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
    if (index.type && index.type->kind == type_kind::BITINT) {
        type_ptr carrier = index.type->size() <= 2
            ? (index.type->is_unsigned() ? type::make_uint() : type::make_int())
            : (index.type->is_unsigned() ? type::make_ulong() : type::make_long());
        index = emit_unop(icode_op::CAST, index, carrier);
    } else if (index.type && index.type->is_integer() &&
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
    type_ptr struct_type = e.owner_type;
    if (!struct_type) {
        struct_type = e.object->type;
        if (e.is_arrow && struct_type && struct_type->is_ptr())
            struct_type = struct_type->base;
    }

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
        if (ptr.is_symbol() && ptr.type && ptr.type->is_volatile)
            field_ptr_type->is_volatile = true;
        ptr.type = field_ptr_type;
    }
    return ptr;
}

operand ir_gen::gen_lvalue_addr(expr &e, type_ptr ptr_t) {
    if (dynamic_cast<string_literal_expr *>(&e)) {
        // The literal visitor already produces its static storage address.
        // Address-of changes the pointee from element to complete array;
        // taking the address of the temporary pointer would name a spill.
        operand address = gen_expr(e);
        address.type = ptr_t;
        return address;
    }
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
        if (index.type && index.type->kind == type_kind::BITINT) {
            type_ptr carrier = index.type->size() <= 2
                ? (index.type->is_unsigned() ? type::make_uint() : type::make_int())
                : (index.type->is_unsigned() ? type::make_ulong() : type::make_long());
            index = emit_unop(icode_op::CAST, index, carrier);
        } else if (index.type && index.type->is_integer() &&
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

operand ir_gen::gen_lvalue_read_once(expr &lhs, operand &captured_address) {
    captured_address = operand::make_none();
    if (dynamic_cast<ident_expr *>(&lhs))
        return gen_expr(lhs);
    captured_address = gen_lvalue_addr(
        lhs, type::make_pointer(lhs.type ? lhs.type : type::make_int()));
    // A pointer identifier is itself a memory operand.  Capture its current
    // value before a compound assignment's RHS can change it or a volatile
    // pointer can yield a different address on its next read.
    if (captured_address.is_symbol()) {
        operand pointer = new_temp(captured_address.type
                                       ? captured_address.type->unqual()
                                       : type::make_pointer(type::make_int()));
        emit_assign(pointer, captured_address);
        captured_address = pointer;
    }
    if (auto *member = dynamic_cast<member_expr *>(&lhs))
        return gen_member_value_at(*member, captured_address);
    return emit_unop(icode_op::GET_VALUE_AT, captured_address,
                     lhs.type ? lhs.type : type::make_int());
}

operand ir_gen::gen_member_value_at(member_expr &e, operand ptr) {
    const struct_field *fld = find_member(e);
    type_ptr fld_type = e.type ? e.type : type::make_int();

    if (fld_type && fld_type->is_array() && fld_type->base) {
        ptr.type = type::make_pointer(fld_type->base);
        return ptr;
    }

    type_ptr access_type = fld_type;
    int bit_offset = fld ? fld->bit_offset : 0;
    if (fld && fld->bit_width >= 0 && byte_contained_bitfield(*fld)) {
        const int byte_offset = bit_offset / 8;
        access_type = type::make_uchar();
        type_ptr byte_ptr_type = elem_ptr_like(ptr.type, access_type);
        if (byte_offset != 0) {
            ptr = emit_binop(
                icode_op::ADD, ptr,
                operand::make_int(byte_offset, type::make_int()),
                byte_ptr_type);
        } else {
            ptr.type = byte_ptr_type;
        }
        bit_offset %= 8;
    }

    operand loaded = emit_unop(icode_op::GET_VALUE_AT, ptr, access_type);

    if (fld && fld->bit_width >= 0) {
        if (bit_offset > 0) {
            operand off_op = operand::make_int(bit_offset, type::make_int());
            loaded = emit_binop(icode_op::SHR, loaded, off_op, access_type);
        }
        int64_t mask =
            static_cast<int64_t>(bitfield_mask_bits(fld->bit_width));
        operand mask_op = operand::make_int(mask, type::make_int());
        loaded = emit_binop(icode_op::BAND, loaded, mask_op, access_type);
        if (access_type != fld_type)
            loaded = emit_unop(icode_op::CAST, loaded, fld_type);
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
    return loaded;
}

void ir_gen::visit(member_expr &e) {
    expr_result_ = gen_member_value_at(e, gen_member_ptr(e));
}

} // namespace xcc
