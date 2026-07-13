/*
 * stdbit.h
 *
 * Standard C23 bit-utility macros for the xcc Z80 target.
 *
 * These helpers are implemented as _Generic dispatch over the target's
 * unsigned integer types. They are target-independent and operate on the
 * promoted value widths defined by this toolchain.
 *
 * Semantics follow the C23 stdbit interfaces as documented by GCC's
 * implementation.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDBIT_H
#define _STDBIT_H

#define __STDC_VERSION_STDBIT_H__ 202311L
#define __STDC_ENDIAN_LITTLE__ 1234
#define __STDC_ENDIAN_BIG__    4321
#define __STDC_ENDIAN_NATIVE__ __STDC_ENDIAN_LITTLE__

static inline unsigned int __stdbit_count_ones_ull(unsigned long long value) {
    unsigned int count = 0;
    while (value != 0ULL) {
        count += (unsigned int)(value & 1ULL);
        value >>= 1;
    }
    return count;
}

static inline unsigned int __stdbit_leading_zeros_ull(unsigned long long value, unsigned int bits) {
    unsigned int count = 0;
    unsigned long long mask;
    if (bits == 0U) {
        return 0U;
    }
    mask = 1ULL << (bits - 1U);
    while ((mask != 0ULL) && ((value & mask) == 0ULL)) {
        ++count;
        mask >>= 1;
    }
    return count;
}

static inline unsigned int __stdbit_leading_ones_ull(unsigned long long value, unsigned int bits) {
    unsigned int count = 0;
    unsigned long long mask;
    if (bits == 0U) {
        return 0U;
    }
    mask = 1ULL << (bits - 1U);
    while ((mask != 0ULL) && ((value & mask) != 0ULL)) {
        ++count;
        mask >>= 1;
    }
    return count;
}

static inline unsigned int __stdbit_trailing_zeros_ull(unsigned long long value, unsigned int bits) {
    unsigned int count = 0;
    unsigned long long mask = 1ULL;
    while ((count < bits) && ((value & mask) == 0ULL)) {
        ++count;
        mask <<= 1;
    }
    return count;
}

static inline unsigned int __stdbit_trailing_ones_ull(unsigned long long value, unsigned int bits) {
    unsigned int count = 0;
    unsigned long long mask = 1ULL;
    while ((count < bits) && ((value & mask) != 0ULL)) {
        ++count;
        mask <<= 1;
    }
    return count;
}

static inline unsigned int __stdbit_bit_width_ull(unsigned long long value) {
    unsigned int width = 0;
    while (value != 0ULL) {
        ++width;
        value >>= 1;
    }
    return width;
}

static inline unsigned long long __stdbit_bit_floor_ull(unsigned long long value) {
    unsigned long long floor = 0ULL;
    while (value != 0ULL) {
        floor = value;
        value &= (value - 1ULL);
    }
    return floor;
}

static inline unsigned long long __stdbit_bit_ceil_ull(unsigned long long value, unsigned int bits) {
    unsigned int width;
    if (value <= 1ULL) {
        return 1ULL;
    }
    width = __stdbit_bit_width_ull((unsigned long long)(value - 1ULL));
    if (width >= bits) {
        return 0ULL;
    }
    return 1ULL << width;
}

#define __STDBIT_DECLARE(width_name, type_name, type_bits) \
static inline unsigned int __stdbit_leading_zeros_##width_name(type_name value) { \
    return __stdbit_leading_zeros_ull((unsigned long long)value, (type_bits)); \
} \
static inline unsigned int __stdbit_leading_ones_##width_name(type_name value) { \
    return __stdbit_leading_ones_ull((unsigned long long)value, (type_bits)); \
} \
static inline unsigned int __stdbit_trailing_zeros_##width_name(type_name value) { \
    return __stdbit_trailing_zeros_ull((unsigned long long)value, (type_bits)); \
} \
static inline unsigned int __stdbit_trailing_ones_##width_name(type_name value) { \
    return __stdbit_trailing_ones_ull((unsigned long long)value, (type_bits)); \
} \
static inline unsigned int __stdbit_first_leading_zero_##width_name(type_name value) { \
    return (value == (type_name)~(type_name)0) ? 0U : (1U + __stdbit_leading_ones_##width_name(value)); \
} \
static inline unsigned int __stdbit_first_leading_one_##width_name(type_name value) { \
    return (value == (type_name)0) ? 0U : (1U + __stdbit_leading_zeros_##width_name(value)); \
} \
static inline unsigned int __stdbit_first_trailing_zero_##width_name(type_name value) { \
    return (value == (type_name)~(type_name)0) ? 0U : (1U + __stdbit_trailing_ones_##width_name(value)); \
} \
static inline unsigned int __stdbit_first_trailing_one_##width_name(type_name value) { \
    return (value == (type_name)0) ? 0U : (1U + __stdbit_trailing_zeros_##width_name(value)); \
} \
static inline unsigned int __stdbit_count_zeros_##width_name(type_name value) { \
    return (type_bits) - __stdbit_count_ones_ull((unsigned long long)value); \
} \
static inline unsigned int __stdbit_count_ones_##width_name(type_name value) { \
    return __stdbit_count_ones_ull((unsigned long long)value); \
} \
static inline int __stdbit_has_single_bit_##width_name(type_name value) { \
    return (value != (type_name)0) && ((value & (type_name)(value - 1)) == (type_name)0); \
} \
static inline unsigned int __stdbit_bit_width_##width_name(type_name value) { \
    return __stdbit_bit_width_ull((unsigned long long)value); \
} \
static inline type_name __stdbit_bit_floor_##width_name(type_name value) { \
    return (type_name)__stdbit_bit_floor_ull((unsigned long long)value); \
} \
static inline type_name __stdbit_bit_ceil_##width_name(type_name value) { \
    return (type_name)__stdbit_bit_ceil_ull((unsigned long long)value, (type_bits)); \
} \
static inline type_name __stdbit_rotate_left_##width_name(type_name value, unsigned int count) { \
    count %= (type_bits); \
    if (count == 0U) { \
        return value; \
    } \
    return (type_name)((type_name)(value << count) | (type_name)(value >> ((type_bits) - count))); \
} \
static inline type_name __stdbit_rotate_right_##width_name(type_name value, unsigned int count) { \
    count %= (type_bits); \
    if (count == 0U) { \
        return value; \
    } \
    return (type_name)((type_name)(value >> count) | (type_name)(value << ((type_bits) - count))); \
}

__STDBIT_DECLARE(uc,  unsigned char,      8U)
__STDBIT_DECLARE(us,  unsigned short,    16U)
__STDBIT_DECLARE(ui,  unsigned int,      16U)
__STDBIT_DECLARE(ul,  unsigned long,     32U)
__STDBIT_DECLARE(ull, unsigned long long, 64U)

#define __STDBIT_DISPATCH(name, value) _Generic((value), \
    unsigned char: __stdbit_##name##_uc, \
    unsigned short: __stdbit_##name##_us, \
    unsigned int: __stdbit_##name##_ui, \
    unsigned long: __stdbit_##name##_ul, \
    unsigned long long: __stdbit_##name##_ull \
)(value)

#define __STDBIT_DISPATCH2(name, value, arg) _Generic((value), \
    unsigned char: __stdbit_##name##_uc, \
    unsigned short: __stdbit_##name##_us, \
    unsigned int: __stdbit_##name##_ui, \
    unsigned long: __stdbit_##name##_ul, \
    unsigned long long: __stdbit_##name##_ull \
)(value, arg)

#define stdc_leading_zeros(value)        __STDBIT_DISPATCH(leading_zeros, (value))
#define stdc_leading_ones(value)         __STDBIT_DISPATCH(leading_ones, (value))
#define stdc_trailing_zeros(value)       __STDBIT_DISPATCH(trailing_zeros, (value))
#define stdc_trailing_ones(value)        __STDBIT_DISPATCH(trailing_ones, (value))
#define stdc_first_leading_zero(value)   __STDBIT_DISPATCH(first_leading_zero, (value))
#define stdc_first_leading_one(value)    __STDBIT_DISPATCH(first_leading_one, (value))
#define stdc_first_trailing_zero(value)  __STDBIT_DISPATCH(first_trailing_zero, (value))
#define stdc_first_trailing_one(value)   __STDBIT_DISPATCH(first_trailing_one, (value))
#define stdc_count_zeros(value)          __STDBIT_DISPATCH(count_zeros, (value))
#define stdc_count_ones(value)           __STDBIT_DISPATCH(count_ones, (value))
#define stdc_has_single_bit(value)       __STDBIT_DISPATCH(has_single_bit, (value))
#define stdc_bit_width(value)            __STDBIT_DISPATCH(bit_width, (value))
#define stdc_bit_floor(value)            __STDBIT_DISPATCH(bit_floor, (value))
#define stdc_bit_ceil(value)             __STDBIT_DISPATCH(bit_ceil, (value))
#define stdc_rotate_left(value, count)   __STDBIT_DISPATCH2(rotate_left, (value), (count))
#define stdc_rotate_right(value, count)  __STDBIT_DISPATCH2(rotate_right, (value), (count))

#endif /* _STDBIT_H */
