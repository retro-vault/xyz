// Common conditional-expression type after array/function value conversion.
#pragma once
#include "frontend/types.h"

namespace xcc {

inline type_ptr conditional_value_type(type_ptr t) {
    if (t && t->is_array() && t->base) {
        auto element = std::make_shared<type>(*t->base);
        element->is_const |= t->is_const;
        element->is_volatile |= t->is_volatile;
        element->is_restrict |= t->is_restrict;
        element->is_atomic |= t->is_atomic;
        return type::make_pointer(element);
    }
    if (t && t->is_func())
        return type::make_pointer(t);
    return t;
}

inline type_ptr conditional_common_type(type_ptr t, type_ptr f) {
    t = conditional_value_type(t);
    f = conditional_value_type(f);
    if (!t || !f)
        return t ? t : f;
    if (t->is_arith() && f->is_arith())
        return usual_arith_conv(t->unqual(), f->unqual());
    if (t->is_ptr() && f->is_ptr()) {
        auto result = t->unqual();
        // Preserve a bank byte whenever either selected arm can supply one.
        result->is_far = t->is_far || f->is_far;
        if (t->base && f->base) {
            type_ptr base = t->base;
            if (f->base->kind == type_kind::VOID && !t->base->is_func())
                base = f->base;
            auto qualified = base->unqual();
            qualified->is_const = t->base->is_const || f->base->is_const;
            qualified->is_volatile = t->base->is_volatile || f->base->is_volatile;
            qualified->is_restrict = t->base->is_restrict || f->base->is_restrict;
            qualified->is_atomic = t->base->is_atomic || f->base->is_atomic;
            result->base = qualified;
        }
        return result;
    }
    // In a valid integer/pointer combination, the integer arm denotes a
    // null pointer constant.
    if (t->is_ptr() && f->is_integer())
        return t->unqual();
    if (t->is_integer() && f->is_ptr())
        return f->unqual();
    return t->unqual();
}

} // namespace xcc
