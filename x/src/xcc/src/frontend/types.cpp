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
#include <cmath>
#include <cstring>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <sstream>

namespace xcc {

namespace {

float_format g_float_format = float_format::IEEE32;
call_abi g_default_call_abi = call_abi::SDCCCALL1;

int64_t mask_to_bytes(int64_t value, int bytes) {
    if (bytes <= 0 || bytes >= 8)
        return value;
    const uint64_t mask = (uint64_t{1} << (bytes * 8)) - 1;
    return static_cast<int64_t>(static_cast<uint64_t>(value) & mask);
}

uint16_t round_to_even_u16(double value) {
    if (value <= 0.0)
        return 0;

    const double floored = std::floor(value);
    const double frac = value - floored;
    uint32_t rounded = static_cast<uint32_t>(floored);
    if (frac > 0.5) {
        ++rounded;
    } else if (frac == 0.5 && (rounded & 1u) != 0u) {
        ++rounded;
    }
    return static_cast<uint16_t>(rounded);
}

uint16_t encode_ieee16_raw(double value) {
    const bool sign = std::signbit(value);
    const uint16_t sign_bit = sign ? 0x8000u : 0x0000u;

    if (std::isnan(value))
        return static_cast<uint16_t>(sign_bit | 0x7e00u);
    if (std::isinf(value))
        return static_cast<uint16_t>(sign_bit | 0x7c00u);
    if (value == 0.0)
        return sign_bit;

    const double abs_value = std::fabs(value);
    int exponent = 0;
    const double fraction = std::frexp(abs_value, &exponent);
    int half_exponent = exponent - 1 + 15;

    if (half_exponent >= 31)
        return static_cast<uint16_t>(sign_bit | 0x7c00u);

    if (half_exponent <= 0) {
        if (half_exponent < -10)
            return sign_bit;

        uint16_t mantissa = round_to_even_u16(std::ldexp(abs_value, 24));
        if (mantissa == 0)
            return sign_bit;
        if (mantissa >= 1024u)
            return static_cast<uint16_t>(sign_bit | 0x0400u);
        return static_cast<uint16_t>(sign_bit | mantissa);
    }

    const double significand = std::ldexp(fraction, 1);
    uint16_t mantissa = round_to_even_u16((significand - 1.0) * 1024.0);
    if (mantissa >= 1024u) {
        mantissa = 0;
        ++half_exponent;
        if (half_exponent >= 31)
            return static_cast<uint16_t>(sign_bit | 0x7c00u);
    }

    return static_cast<uint16_t>(sign_bit |
                                 (static_cast<uint16_t>(half_exponent) << 10) |
                                 mantissa);
}

int64_t fixed_positive_infinity_raw(int bytes) {
    switch (bytes) {
    case 2:
        return 0x7fff;
    case 4:
        return 0x7fffffff;
    default:
        throw std::logic_error("unsupported fixed-float byte width");
    }
}

int64_t fixed_nan_raw(int bytes) {
    switch (bytes) {
    case 2:
        return 0x7ffe;
    case 4:
        return 0x7ffffffe;
    default:
        throw std::logic_error("unsupported fixed-float byte width");
    }
}

int64_t fixed_negative_infinity_raw(int bytes) {
    switch (bytes) {
    case 2:
        return -0x8000;
    case 4:
        return -0x80000000ll;
    default:
        throw std::logic_error("unsupported fixed-float byte width");
    }
}

int64_t fixed_max_finite_raw(int bytes) {
    return fixed_nan_raw(bytes) - 1;
}

int64_t fixed_min_finite_raw(int bytes) {
    return fixed_negative_infinity_raw(bytes) + 1;
}

} // namespace

void set_float_format(float_format format) {
    g_float_format = format;
}

float_format get_float_format() {
    return g_float_format;
}

void set_default_call_abi(call_abi abi) {
    switch (abi) {
    case call_abi::SDCCCALL0:
    case call_abi::SDCCCALL1:
    case call_abi::Z88DK_SMALLC:
    case call_abi::Z88DK_FASTCALL:
    case call_abi::Z88DK_CALLEE:
        g_default_call_abi = abi;
        break;
    default:
        g_default_call_abi = call_abi::SDCCCALL1;
        break;
    }
}

call_abi get_default_call_abi() {
    return g_default_call_abi;
}

const char *float_format_name(float_format format) {
    switch (format) {
    case float_format::IEEE32:     return "ieee32";
    case float_format::IEEE16:     return "ieee16";
    case float_format::FIXED8_8:   return "fixed8_8";
    case float_format::FIXED16_16: return "fixed16_16";
    case float_format::FIXED24_8:  return "fixed24_8";
    }
    return "ieee32";
}

int float_format_size(float_format format) {
    switch (format) {
    case float_format::IEEE32:     return 4;
    case float_format::IEEE16:     return 2;
    case float_format::FIXED8_8:   return 2;
    case float_format::FIXED16_16: return 4;
    case float_format::FIXED24_8:  return 4;
    }
    return 4;
}

int float_format_fraction_bits(float_format format) {
    switch (format) {
    case float_format::FIXED8_8:   return 8;
    case float_format::FIXED16_16: return 16;
    case float_format::FIXED24_8:  return 8;
    case float_format::IEEE16:
    case float_format::IEEE32:
        return 0;
    }
    return 0;
}

int64_t encode_float_constant(double value, type_ptr target_type) {
    if (!target_type || target_type->kind == type_kind::DOUBLE ||
        target_type->size() == 8) {
        uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        return static_cast<int64_t>(bits);
    }

    const float_format format =
        target_type->kind == type_kind::FLOAT ? get_float_format()
                                              : float_format::IEEE32;
    if (format == float_format::IEEE32) {
        uint32_t bits = 0;
        float narrowed = static_cast<float>(value);
        std::memcpy(&bits, &narrowed, sizeof(bits));
        return bits;
    }
    if (format == float_format::IEEE16)
        return encode_ieee16_raw(value);

    const int frac_bits = float_format_fraction_bits(format);
    const int bytes = float_format_size(format);
    if (std::isnan(value))
        return mask_to_bytes(fixed_nan_raw(bytes), bytes);
    if (std::isinf(value)) {
        const int64_t raw = value < 0.0 ? fixed_negative_infinity_raw(bytes)
                                        : fixed_positive_infinity_raw(bytes);
        return mask_to_bytes(raw, bytes);
    }

    const double scaled = std::ldexp(value, frac_bits);
    if (scaled >= static_cast<double>(std::numeric_limits<int64_t>::max()))
        return mask_to_bytes(fixed_max_finite_raw(bytes), bytes);
    if (scaled <= static_cast<double>(std::numeric_limits<int64_t>::min()))
        return mask_to_bytes(fixed_min_finite_raw(bytes), bytes);

    const int64_t raw = static_cast<int64_t>(std::llround(scaled));
    if (raw > fixed_max_finite_raw(bytes))
        return mask_to_bytes(fixed_max_finite_raw(bytes), bytes);
    if (raw < fixed_min_finite_raw(bytes))
        return mask_to_bytes(fixed_min_finite_raw(bytes), bytes);
    return mask_to_bytes(raw, bytes);
}

bool type::is_integer() const {
    switch (kind) {
    case type_kind::BOOL:   case type_kind::CHAR:   case type_kind::UCHAR:
    case type_kind::SHORT:  case type_kind::USHORT:
    case type_kind::INT:    case type_kind::UINT:
    case type_kind::LONG:   case type_kind::ULONG:
    case type_kind::LLONG:  case type_kind::ULLONG:
    case type_kind::ENUM:   case type_kind::BITINT:
    case type_kind::CHAR8T:
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
    case type_kind::BITINT:
        return bitint_unsigned;
    case type_kind::CHAR8T:
        return true; // char8_t is unsigned
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
    case type_kind::FLOAT:   return float_format_size(get_float_format());
    case type_kind::DOUBLE:  return 8;
    case type_kind::COMPLEX: return 8; // re+im, each a 4-byte soft-float
    case type_kind::POINTER: return is_far ? 3 : 2;
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
    case type_kind::CHAR8T:  return 1;  // unsigned char size
    case type_kind::FUNCTION:
        return 2; // pointer-sized (function never directly sized)
    case type_kind::BITINT: {
        // Round up to the nearest of 1, 2, or 4 bytes (max 32-bit on Z80).
        int bytes = (bitint_width + 7) / 8;
        if (bytes <= 1) return 1;
        if (bytes <= 2) return 2;
        return 4;
    }
    }
    return 0;
}

int type::align() const {
    // C23 §6.2.8: alignment of an array type equals alignment of its element.
    // This covers incomplete arrays (array_size == 0) where size() returns 0.
    if (kind == type_kind::ARRAY && base)
        return base->align();

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
    case type_kind::CHAR8T:  os << "char8_t"; break;
    case type_kind::BITINT:  os << (bitint_unsigned ? "unsigned " : "") << "_BitInt(" << bitint_width << ")"; break;
    case type_kind::POINTER: os << base->to_string() << (is_far ? "* far" : "*"); break;
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
        const int int_size = type::make_int()->size();
        if ((!t->is_unsigned() && t->size() <= int_size) ||
            (t->is_unsigned() && t->size() < int_size))
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
