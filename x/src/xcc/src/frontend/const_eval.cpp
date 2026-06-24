//
// const_eval.cpp — compile-time constant expression evaluator for xcc.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/const_eval.h"
#include "frontend/ast.h"
#include <cmath>
#include <limits>

namespace xcc {

static std::optional<int64_t> apply_integer_cast(int64_t v, const type *t) {
    if (!t || !t->is_integer()) return std::nullopt;
    int bits = t->size() * 8;
    if (bits <= 0 || bits > 64) return std::nullopt;
    // Mask to the lower `bits` bits.
    uint64_t mask = (bits == 64) ? ~0ULL : ((1ULL << bits) - 1);
    uint64_t u = (uint64_t)v & mask;
    if (t->is_unsigned()) return (int64_t)u;
    // Sign-extend: if high bit is set, negative.
    uint64_t sign_bit = 1ULL << (bits - 1);
    if (u & sign_bit) return (int64_t)(u | ~mask);
    return (int64_t)u;
}

// Evaluate a floating-point constant expression.  Returns nullopt if not
// reducible at compile time.
static std::optional<double> evaluate_fp(const expr *e) {
    if (!e) return std::nullopt;
    if (auto *fl = dynamic_cast<const float_literal_expr *>(e)) return fl->value;
    if (auto *il = dynamic_cast<const int_literal_expr   *>(e)) return (double)il->value;
    if (auto *ch = dynamic_cast<const char_literal_expr  *>(e)) return (double)ch->value;
    if (auto *c = dynamic_cast<const cast_expr *>(e)) {
        auto v = evaluate_fp(c->operand.get());
        if (v) return *v;
        auto iv = const_expr_evaluator::evaluate(c->operand.get());
        if (iv) return (double)*iv;
        return std::nullopt;
    }
    if (auto *u = dynamic_cast<const unary_expr *>(e)) {
        auto v = evaluate_fp(u->operand.get());
        if (!v) return std::nullopt;
        if (u->op == unary_op::NEG) return -*v;
        return std::nullopt;
    }
    if (auto *bin = dynamic_cast<const binary_expr *>(e)) {
        auto l = evaluate_fp(bin->left.get());
        auto r = evaluate_fp(bin->right.get());
        if (!l || !r) return std::nullopt;
        switch (bin->op) {
        case bin_op::ADD: return *l + *r;
        case bin_op::SUB: return *l - *r;
        case bin_op::MUL: return *l * *r;
        case bin_op::DIV: return *r != 0.0 ? std::optional<double>(*l / *r) : std::nullopt;
        default: return std::nullopt;
        }
    }
    return std::nullopt;
}

static const struct_field *find_named_field(const type *agg,
                                            const std::string &member) {
    if (!agg)
        return nullptr;
    for (const auto &field : agg->fields) {
        if (field.name == member)
            return &field;
    }
    return nullptr;
}

static int slot_index_for_offset(const type *agg, int offset) {
    if (!agg)
        return -1;
    int slot = -1;
    int last_offset = std::numeric_limits<int>::min();
    for (const auto &field : agg->fields) {
        if (field.name.empty())
            continue;
        if (slot < 0 || field.offset != last_offset) {
            ++slot;
            last_offset = field.offset;
        }
        if (field.offset == offset)
            return slot;
    }
    return -1;
}

static std::optional<int64_t> evaluate_aggregate_member(type_ptr agg_type,
                                                        const expr *init,
                                                        const std::string &member);

static std::optional<int64_t> evaluate_member_value(const struct_field *field,
                                                    const expr *value) {
    if (!field || !value)
        return std::nullopt;
    auto v = const_expr_evaluator::evaluate(value);
    if (!v)
        return std::nullopt;
    if (field->type && field->type->is_integer())
        return apply_integer_cast(*v, field->type.get());
    return v;
}

static std::optional<int64_t> search_designated_member(const init_list_expr *il,
                                                       const struct_field *target) {
    if (!il || !target)
        return std::nullopt;
    for (const auto &elem : il->elements) {
        if (elem.field_name && *elem.field_name == target->name) {
            if (auto v = evaluate_member_value(target, elem.value.get()))
                return v;
        }
        if (auto *nested = dynamic_cast<const init_list_expr *>(elem.value.get())) {
            if (auto v = search_designated_member(nested, target))
                return v;
        }
    }
    return std::nullopt;
}

static std::optional<int64_t> evaluate_aggregate_member(type_ptr agg_type,
                                                        const expr *init,
                                                        const std::string &member) {
    if (!agg_type || !init)
        return std::nullopt;

    if (auto *cl = dynamic_cast<const compound_literal_expr *>(init)) {
        return evaluate_aggregate_member(cl->type ? cl->type : agg_type,
                                         cl->init.get(), member);
    }

    const auto *agg = agg_type.get();
    if (!agg || (agg->kind != type_kind::STRUCT && agg->kind != type_kind::UNION))
        return std::nullopt;

    const struct_field *target = find_named_field(agg, member);
    if (!target)
        return std::nullopt;

    auto *il = dynamic_cast<const init_list_expr *>(init);
    if (!il)
        return std::nullopt;

    if (auto v = search_designated_member(il, target))
        return v;

    const int target_slot = slot_index_for_offset(agg, target->offset);
    if (target_slot < 0)
        return std::nullopt;

    int current_slot = 0;
    for (const auto &elem : il->elements) {
        if (elem.field_name) {
            const struct_field *designated = find_named_field(agg, *elem.field_name);
            if (!designated)
                continue;
            int designated_slot = slot_index_for_offset(agg, designated->offset);
            current_slot = (agg->kind == type_kind::UNION || designated_slot < 0)
                             ? 0
                             : designated_slot + 1;
            continue;
        }

        if (current_slot == target_slot) {
            if (auto v = evaluate_member_value(target, elem.value.get()))
                return v;
            if (auto *nested = dynamic_cast<const init_list_expr *>(elem.value.get())) {
                if (auto v = search_designated_member(nested, target))
                    return v;
            }
        }

        if (agg->kind != type_kind::UNION)
            ++current_slot;
    }

    return std::nullopt;
}

std::optional<int64_t> const_expr_evaluator::evaluate(const expr *e) {
    if (!e) return std::nullopt;

    if (auto *lit = dynamic_cast<const int_literal_expr  *>(e)) return lit->value;
    if (auto *ch  = dynamic_cast<const char_literal_expr *>(e)) return ch->value;

    if (auto *sz = dynamic_cast<const sizeof_expr *>(e)) {
        if (sz->is_countof) {
            if (sz->sizeof_type &&
                sz->sizeof_type->kind == type_kind::ARRAY &&
                sz->sizeof_type->array_size >= 0) {
                return sz->sizeof_type->array_size;
            }
            if (sz->sizeof_expr_op && sz->sizeof_expr_op->type &&
                sz->sizeof_expr_op->type->kind == type_kind::ARRAY) {
                return sz->sizeof_expr_op->type->array_size;
            }
            return std::nullopt;
        }
        if (sz->sizeof_type)
            return sz->is_alignof ? sz->sizeof_type->align() : sz->sizeof_type->size();
        if (sz->sizeof_expr_op && sz->sizeof_expr_op->type)
            return sz->is_alignof ? (sz->align_override > 0 ? sz->align_override
                                                            : sz->sizeof_expr_op->type->align())
                                  : sz->sizeof_expr_op->type->size();
        return std::nullopt;
    }

    if (auto *bin = dynamic_cast<const binary_expr *>(e)) {
        auto l = evaluate(bin->left.get());
        auto r = evaluate(bin->right.get());
        if (l && r) {
            switch (bin->op) {
            case bin_op::ADD:  return *l + *r;
            case bin_op::SUB:  return *l - *r;
            case bin_op::MUL:  return *l * *r;
            case bin_op::DIV:  return *r ? std::optional<int64_t>(*l / *r) : std::nullopt;
            case bin_op::MOD:  return *r ? std::optional<int64_t>(*l % *r) : std::nullopt;
            case bin_op::AND:  return *l & *r;
            case bin_op::OR:   return *l | *r;
            case bin_op::XOR:  return *l ^ *r;
            case bin_op::SHL:  return *l << *r;
            case bin_op::SHR:  return *l >> *r;
            case bin_op::EQ:   return (*l == *r) ? 1 : 0;
            case bin_op::NE:   return (*l != *r) ? 1 : 0;
            case bin_op::LT:   return (*l <  *r) ? 1 : 0;
            case bin_op::LE:   return (*l <= *r) ? 1 : 0;
            case bin_op::GT:   return (*l >  *r) ? 1 : 0;
            case bin_op::GE:   return (*l >= *r) ? 1 : 0;
            case bin_op::LAND: return (*l && *r) ? 1 : 0;
            case bin_op::LOR:  return (*l || *r) ? 1 : 0;
            default: return std::nullopt;
            }
        }
        // Fall back to floating-point evaluation for comparison/arithmetic ops.
        auto fl = evaluate_fp(bin->left.get());
        auto fr = evaluate_fp(bin->right.get());
        if (fl && fr) {
            switch (bin->op) {
            case bin_op::ADD:  { double v = *fl + *fr; return (int64_t)v; }
            case bin_op::SUB:  { double v = *fl - *fr; return (int64_t)v; }
            case bin_op::MUL:  { double v = *fl * *fr; return (int64_t)v; }
            case bin_op::DIV:  return *fr != 0.0 ? std::optional<int64_t>((int64_t)(*fl / *fr)) : std::nullopt;
            case bin_op::EQ:   return (*fl == *fr) ? 1 : 0;
            case bin_op::NE:   return (*fl != *fr) ? 1 : 0;
            case bin_op::LT:   return (*fl <  *fr) ? 1 : 0;
            case bin_op::LE:   return (*fl <= *fr) ? 1 : 0;
            case bin_op::GT:   return (*fl >  *fr) ? 1 : 0;
            case bin_op::GE:   return (*fl >= *fr) ? 1 : 0;
            default: return std::nullopt;
            }
        }
        return std::nullopt;
    }

    if (auto *c = dynamic_cast<const cast_expr *>(e)) {
        auto v = evaluate(c->operand.get());
        if (v) {
            if (c->target_type && c->target_type->is_integer())
                return apply_integer_cast(*v, c->target_type.get());
            // Float target type: int-to-float cast.  Return int64 with the
            // float value; caller will use evaluate_fp() for float contexts.
            return v;
        }
        // Try float operand → integer result (e.g. (int)(1.5f))
        auto fv = evaluate_fp(c->operand.get());
        if (fv && c->target_type && c->target_type->is_integer())
            return apply_integer_cast((int64_t)*fv, c->target_type.get());
        return std::nullopt;
    }

    if (auto *u = dynamic_cast<const unary_expr *>(e)) {
        auto v = evaluate(u->operand.get());
        if (!v) return std::nullopt;
        switch (u->op) {
        case unary_op::NEG:  return -*v;
        case unary_op::BNOT: return ~*v;
        case unary_op::NOT:  return !*v ? 1 : 0;
        default: return std::nullopt;
        }
    }

    if (auto *idx = dynamic_cast<const index_expr *>(e)) {
        auto i = evaluate(idx->index.get());
        auto *s = dynamic_cast<const string_literal_expr *>(idx->base.get());
        if (!i || !s || *i < 0)
            return std::nullopt;
        size_t pos = static_cast<size_t>(*i);
        if (pos >= s->value.size())
            return std::nullopt;

        int64_t value = static_cast<unsigned char>(s->value[pos]);
        if (idx->type && !idx->type->is_unsigned() && value >= 0x80)
            value = static_cast<int64_t>(static_cast<int8_t>(value));
        return value;
    }

    if (auto *cl = dynamic_cast<const compound_literal_expr *>(e)) {
        if (!cl->init)
            return std::nullopt;
        auto v = evaluate(cl->init.get());
        if (!v)
            return std::nullopt;
        if (cl->type && cl->type->is_integer())
            return apply_integer_cast(*v, cl->type.get());
        return v;
    }

    if (auto *mem = dynamic_cast<const member_expr *>(e)) {
        if (auto *id = dynamic_cast<const ident_expr *>(mem->object.get())) {
            if (id->sym && id->sym->init_expr &&
                id->sym->type &&
                (id->sym->type->kind == type_kind::STRUCT ||
                 id->sym->type->kind == type_kind::UNION)) {
                if (auto v = evaluate_aggregate_member(id->sym->type,
                                                       id->sym->init_expr,
                                                       mem->member)) {
                    if (mem->type && mem->type->is_integer())
                        return apply_integer_cast(*v, mem->type.get());
                    return v;
                }
            }
        }

        if (auto *cl = dynamic_cast<const compound_literal_expr *>(mem->object.get())) {
            if (auto v = evaluate_aggregate_member(cl->type, cl->init.get(),
                                                   mem->member)) {
                if (mem->type && mem->type->is_integer())
                    return apply_integer_cast(*v, mem->type.get());
                return v;
            }
        }
    }

    if (auto *id = dynamic_cast<const ident_expr *>(e)) {
        if (id->sym && id->sym->kind == sym_kind::ENUM_CONST)
            return id->sym->enum_val;
        // Constant folding for const-qualified integer variables (GCC/Clang extension).
        if (id->sym && id->sym->const_val && id->sym->type &&
            id->sym->type->is_const && id->sym->type->is_integer())
            return id->sym->const_val;
        return std::nullopt;
    }

    // Constant-foldable builtins.
    if (auto *call = dynamic_cast<const call_expr *>(e)) {
        if (auto *id = dynamic_cast<const ident_expr *>(call->callee.get())) {
            const std::string &nm = id->name;
            if ((nm == "__builtin_strcmp" || nm == "strcmp") && call->args.size() == 2) {
                auto *s1 = dynamic_cast<const string_literal_expr *>(call->args[0].get());
                auto *s2 = dynamic_cast<const string_literal_expr *>(call->args[1].get());
                if (s1 && s2) return (int64_t)s1->value.compare(s2->value);
            }
            // Popcount family.
            if ((nm == "__builtin_popcount"  || nm == "__builtin_popcountl"  ||
                 nm == "__builtin_popcountll") && call->args.size() == 1) {
                auto v = evaluate(call->args[0].get());
                if (v) return (int64_t)__builtin_popcountll((uint64_t)*v);
            }
            // Parity family.
            if ((nm == "__builtin_parity"   || nm == "__builtin_parityl"   ||
                 nm == "__builtin_parityll") && call->args.size() == 1) {
                auto v = evaluate(call->args[0].get());
                if (v) return (int64_t)(__builtin_popcountll((uint64_t)*v) & 1);
            }
            // Clz / ctz / ffs.
            if ((nm == "__builtin_clz"  || nm == "__builtin_clzl"  ||
                 nm == "__builtin_clzll") && call->args.size() == 1) {
                auto v = evaluate(call->args[0].get());
                if (v && *v != 0) return (int64_t)__builtin_clzll((uint64_t)*v);
            }
            if ((nm == "__builtin_ctz"  || nm == "__builtin_ctzl"  ||
                 nm == "__builtin_ctzll") && call->args.size() == 1) {
                auto v = evaluate(call->args[0].get());
                if (v && *v != 0) return (int64_t)__builtin_ctzll((uint64_t)*v);
            }
            if (nm == "__builtin_ffs" || nm == "__builtin_ffsl" ||
                nm == "__builtin_ffsll") {
                if (call->args.size() == 1) {
                    auto v = evaluate(call->args[0].get());
                    if (v) return (int64_t)__builtin_ffsll(*v);
                }
            }
            // __builtin_constant_p — returns 1 if argument is a constant.
            if (nm == "__builtin_constant_p" && call->args.size() == 1) {
                auto v = evaluate(call->args[0].get());
                return v ? (int64_t)1 : (int64_t)0;
            }
            // __builtin_expect — return first arg (for constexpr purposes).
            if (nm == "__builtin_expect" && call->args.size() == 2) {
                return evaluate(call->args[0].get());
            }
            // __builtin_fpclassify(nan, inf, norm, subnorm, zero, x)
            if (nm == "__builtin_fpclassify" && call->args.size() == 6) {
                auto xv = evaluate_fp(call->args[5].get());
                if (xv) {
                    int idx;
                    if (std::isnan(*xv))         idx = 0; // FP_NAN
                    else if (std::isinf(*xv))    idx = 1; // FP_INFINITE
                    else if (*xv == 0.0)         idx = 4; // FP_ZERO
                    else if (std::fpclassify(*xv) == FP_SUBNORMAL) idx = 3; // FP_SUBNORMAL
                    else                          idx = 2; // FP_NORMAL
                    return evaluate(call->args[idx].get());
                }
            }
            // __builtin_isinf_sign, __builtin_isfinite, __builtin_isnormal, __builtin_isnan
            if (nm == "__builtin_isinf_sign" && call->args.size() == 1) {
                auto xv = evaluate_fp(call->args[0].get());
                if (xv) return (int64_t)(std::isinf(*xv) ? (*xv > 0 ? 1 : -1) : 0);
            }
            if (nm == "__builtin_isfinite" && call->args.size() == 1) {
                auto xv = evaluate_fp(call->args[0].get());
                if (xv) return (int64_t)(std::isfinite(*xv) ? 1 : 0);
            }
            if (nm == "__builtin_isnormal" && call->args.size() == 1) {
                auto xv = evaluate_fp(call->args[0].get());
                if (xv) return (int64_t)(std::isnormal(*xv) ? 1 : 0);
            }
            if (nm == "__builtin_isnan" && call->args.size() == 1) {
                auto xv = evaluate_fp(call->args[0].get());
                if (xv) return (int64_t)(std::isnan(*xv) ? 1 : 0);
            }
        }
        return std::nullopt;
    }

    if (auto *tern = dynamic_cast<const conditional_expr *>(e)) {
        auto cond = evaluate(tern->cond.get());
        if (!cond) return std::nullopt;
        return *cond ? evaluate(tern->then_expr.get())
                     : evaluate(tern->else_expr.get());
    }

    return std::nullopt;
}

} // namespace xcc
