//
// types.cpp — C11 type system: sizes, predicates, arithmetic conversions.
//
// Implements type::size() and type::align() using the xcc Z80 target type
// sizes, the is_integer / is_unsigned / is_arith / is_scalar predicates,
// the to_string() diagnostic formatter, and the two arithmetic conversion
// helpers (integer_promote, usual_arith_conv) following C11 §6.3.1.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/types.h"
#include <stdexcept>
#include <sstream>

namespace xcc {

bool type::is_integer() const {
    switch (kind) {
    case type_kind::BOOL:  case type_kind::CHAR:   case type_kind::UCHAR:
    case type_kind::SHORT: case type_kind::USHORT:
    case type_kind::INT:   case type_kind::UINT:
    case type_kind::LONG:  case type_kind::ULONG:
    case type_kind::LLONG: case type_kind::ULLONG:
    case type_kind::ENUM:
        return true;
    default:
        return false;
    }
}

bool type::is_unsigned() const {
    switch (kind) {
    case type_kind::BOOL:  case type_kind::UCHAR:  case type_kind::USHORT:
    case type_kind::UINT:  case type_kind::ULONG:  case type_kind::ULLONG:
        return true;
    default:
        return false;
    }
}

bool type::is_arith() const {
    return is_integer() ||
           kind == type_kind::FLOAT  ||
           kind == type_kind::DOUBLE ||
           kind == type_kind::COMPLEX;
}

bool type::is_scalar() const {
    return is_arith() || kind == type_kind::POINTER || kind == type_kind::ENUM;
}

// Z80 type sizes
int type::size() const {
    switch (kind) {
    case type_kind::VOID:    return 0;
    case type_kind::BOOL:    return 1;
    case type_kind::CHAR:    return 1;
    case type_kind::UCHAR:   return 1;
    case type_kind::SHORT:   return 2;
    case type_kind::USHORT:  return 2;
    case type_kind::INT:     return 2;
    case type_kind::UINT:    return 2;
    case type_kind::LONG:    return 4;
    case type_kind::ULONG:   return 4;
    case type_kind::LLONG:   return 8;
    case type_kind::ULLONG:  return 8;
    case type_kind::FLOAT:   return 4;
    case type_kind::DOUBLE:  return 4; // double == float on Z80 (no FPU)
    case type_kind::COMPLEX: return 8; // re+im, each a 4-byte soft-float
    case type_kind::POINTER: return 2;
    case type_kind::ENUM:    return 2; // enum -> int -> 2 bytes
    case type_kind::ARRAY: {
        if (!base) return 0;
        return array_size * base->size();
    }
    case type_kind::STRUCT:
    case type_kind::UNION: {
        if (!complete) return 0;
        int total = 0;
        for (auto &f : fields) {
            int end = f.offset + f.type->size();
            if (end > total) total = end;
        }
        return total;
    }
    case type_kind::FUNCTION:
        return 2; // pointer-sized (function never directly sized)
    }
    return 0;
}

int type::align() const {
    // Z80 has no alignment requirements beyond 1 byte for most things,
    // but we align 2+ byte types to 2 for potential performance benefit.
    int sz = size();
    if (sz == 0) return 1;
    if (sz == 1) return 1;
    return 2;
}

std::string type::to_string() const {
    std::ostringstream os;
    if (is_const)    os << "const ";
    if (is_volatile) os << "volatile ";
    if (is_restrict) os << "restrict ";
    switch (kind) {
    case type_kind::VOID:    os << "void"; break;
    case type_kind::BOOL:    os << "_Bool"; break;
    case type_kind::CHAR:    os << "char"; break;
    case type_kind::UCHAR:   os << "unsigned char"; break;
    case type_kind::SHORT:   os << "short"; break;
    case type_kind::USHORT:  os << "unsigned short"; break;
    case type_kind::INT:     os << "int"; break;
    case type_kind::UINT:    os << "unsigned int"; break;
    case type_kind::LONG:    os << "long"; break;
    case type_kind::ULONG:   os << "unsigned long"; break;
    case type_kind::LLONG:   os << "long long"; break;
    case type_kind::ULLONG:  os << "unsigned long long"; break;
    case type_kind::FLOAT:   os << "float"; break;
    case type_kind::DOUBLE:  os << "double"; break;
    case type_kind::COMPLEX: os << "float _Complex"; break;
    case type_kind::POINTER: os << base->to_string() << "*"; break;
    case type_kind::ARRAY:   os << base->to_string() << "[" << array_size << "]"; break;
    case type_kind::FUNCTION:
        os << ret->to_string() << "(";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) os << ", ";
            os << params[i]->to_string();
        }
        if (variadic) os << ", ...";
        os << ")";
        break;
    case type_kind::STRUCT: os << "struct " << tag; break;
    case type_kind::UNION:  os << "union "  << tag; break;
    case type_kind::ENUM:   os << "enum";           break;
    }
    return os.str();
}

// ----- Integer rank (C11 §6.3.1.1) -----------------------------------
static int int_rank(type_kind k) {
    switch (k) {
    case type_kind::BOOL:    return 1;
    case type_kind::CHAR:
    case type_kind::UCHAR:   return 2;
    case type_kind::SHORT:
    case type_kind::USHORT:  return 3;
    case type_kind::INT:
    case type_kind::UINT:
    case type_kind::ENUM:    return 4;
    case type_kind::LONG:
    case type_kind::ULONG:   return 5;
    case type_kind::LLONG:
    case type_kind::ULLONG:  return 6;
    default:                return 0;
    }
}

type_ptr integer_promote(type_ptr t) {
    // If rank < int: promote to int (or uint if int can't hold all values)
    if (t->is_integer() && int_rank(t->kind) < int_rank(type_kind::INT)) {
        if (t->size() <= type::make_int()->size())
            return type::make_int();
        return type::make_uint();
    }
    return t;
}

type_ptr usual_arith_conv(type_ptr a, type_ptr b) {
    // Both must be arithmetic
    a = integer_promote(a);
    b = integer_promote(b);

    // Same kind -> done
    if (a->kind == b->kind) return a;

    // complex takes precedence over float/double
    if (a->kind == type_kind::COMPLEX || b->kind == type_kind::COMPLEX)
        return type::make_complex();
    // float/double take precedence
    if (a->kind == type_kind::DOUBLE || b->kind == type_kind::DOUBLE)
        return type::make_double();
    if (a->kind == type_kind::FLOAT  || b->kind == type_kind::FLOAT)
        return type::make_float();

    // Both integer: higher rank wins
    int ra = int_rank(a->kind), rb = int_rank(b->kind);
    if (ra == rb) {
        // same rank: unsigned wins
        return a->is_unsigned() ? a : b;
    }
    type_ptr higher = (ra > rb) ? a : b;
    type_ptr lower  = (ra > rb) ? b : a;

    if (higher->is_unsigned()) return higher;

    // Signed higher: if it can represent all values of lower, use signed
    if (higher->size() > lower->size()) return higher;

    // Otherwise unsigned version of higher rank
    switch (higher->kind) {
    case type_kind::INT:   return type::make_uint();
    case type_kind::LONG:  return type::make_ulong();
    case type_kind::LLONG: return type::make_ullong();
    default:              return higher;
    }
}

} // namespace xcc
