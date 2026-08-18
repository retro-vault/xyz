/*
 * stdatomic.h
 *
 * Standard C23 atomic support for the xcc Z80 target.
 *
 * The current libc provides 8-bit and 16-bit atomic operations through
 * DI/EI-wrapped runtime helpers. Memory-order arguments are accepted for API
 * compatibility but collapse to the same single-core behavior on this target.
 * Wider atomic object types are declared, but the current library only ships
 * helper code for 1-byte and 2-byte operations.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDATOMIC_H
#define _STDATOMIC_H

#define __STDC_VERSION_STDATOMIC_H__ 202311L

#include <stddef.h>
#include <stdint.h>
#include <uchar.h>

/* Memory-order enumeration. */
typedef enum {
    memory_order_relaxed = 0,
    memory_order_consume = 1,
    memory_order_acquire = 2,
    memory_order_release = 3,
    memory_order_acq_rel = 4,
    memory_order_seq_cst = 5
} memory_order;

/* Common atomic scalar aliases. */

typedef unsigned char  atomic_flag;
typedef unsigned char  atomic_bool;
typedef char           atomic_char;
typedef unsigned char  atomic_uchar;
typedef signed char    atomic_schar;
typedef short          atomic_short;
typedef unsigned short atomic_ushort;
typedef int            atomic_int;
typedef unsigned int   atomic_uint;
typedef long           atomic_long;
typedef unsigned long  atomic_ulong;
typedef long long      atomic_llong;
typedef unsigned long long atomic_ullong;
typedef wchar_t        atomic_wchar_t;
typedef char16_t       atomic_char16_t;
typedef char32_t       atomic_char32_t;
typedef ptrdiff_t      atomic_ptrdiff_t;
typedef size_t         atomic_size_t;
typedef intptr_t       atomic_intptr_t;
typedef uintptr_t      atomic_uintptr_t;
typedef intmax_t       atomic_intmax_t;
typedef uintmax_t      atomic_uintmax_t;

/* Runtime entry points for the currently supported widths. */

[[sdcc::sdccall(1)]] unsigned char  _atomic_load_1(unsigned char *ptr);
[[sdcc::sdccall(1)]] unsigned short _atomic_load_2(unsigned short *ptr);
[[sdcc::sdccall(1)]] void _atomic_store_1(unsigned char *ptr, unsigned char val);
[[sdcc::sdccall(1)]] void _atomic_store_2(unsigned short *ptr, unsigned short val);
[[sdcc::sdccall(1)]] unsigned char _atomic_exchange_1(
    unsigned char *ptr, unsigned char val);
[[sdcc::sdccall(1)]] unsigned short _atomic_exchange_2(
    unsigned short *ptr, unsigned short val);
[[sdcc::sdccall(1)]] int _atomic_compare_exchange_1(
    unsigned char *ptr, unsigned char expected, unsigned char desired);
[[sdcc::sdccall(1)]] int _atomic_compare_exchange_2(
    unsigned short *ptr, unsigned short expected, unsigned short desired);
[[sdcc::sdccall(1)]] unsigned char _atomic_fetch_add_1(
    unsigned char *ptr, unsigned char val);
[[sdcc::sdccall(1)]] unsigned short _atomic_fetch_add_2(
    unsigned short *ptr, unsigned short val);
[[sdcc::sdccall(1)]] unsigned char _atomic_fetch_sub_1(
    unsigned char *ptr, unsigned char val);
[[sdcc::sdccall(1)]] unsigned short _atomic_fetch_sub_2(
    unsigned short *ptr, unsigned short val);
[[sdcc::sdccall(1)]] unsigned char _atomic_fetch_and_1(
    unsigned char *ptr, unsigned char val);
[[sdcc::sdccall(1)]] unsigned short _atomic_fetch_and_2(
    unsigned short *ptr, unsigned short val);
[[sdcc::sdccall(1)]] unsigned char _atomic_fetch_or_1(
    unsigned char *ptr, unsigned char val);
[[sdcc::sdccall(1)]] unsigned short _atomic_fetch_or_2(
    unsigned short *ptr, unsigned short val);
[[sdcc::sdccall(1)]] unsigned char _atomic_fetch_xor_1(
    unsigned char *ptr, unsigned char val);
[[sdcc::sdccall(1)]] unsigned short _atomic_fetch_xor_2(
    unsigned short *ptr, unsigned short val);
[[sdcc::sdccall(1)]] unsigned char _atomic_flag_test_set(unsigned char *flag);
[[sdcc::sdccall(1)]] void _atomic_flag_clear(unsigned char *flag);
[[sdcc::sdccall(1)]] void _atomic_width_not_supported(void);

/* Initialization helpers. */
#define ATOMIC_VAR_INIT(val)    (val)
#define ATOMIC_FLAG_INIT        0

#define atomic_init(obj, val)   (*(obj) = (val))

#define __atomic_unsupported_value(obj) (_atomic_width_not_supported(), *(obj))
#define __atomic_unsupported_void()     ((void)_atomic_width_not_supported())

/* Type-dispatched atomic operations for the supported widths. */

#define atomic_load(obj) _Generic(*(obj),                               \
    char:           (char)_atomic_load_1((unsigned char *)(obj)),      \
    signed char:    (signed char)_atomic_load_1((unsigned char *)(obj)),\
    unsigned char:  _atomic_load_1((unsigned char *)(obj)),            \
    short:          (short)_atomic_load_2((unsigned short *)(obj)),    \
    unsigned short: _atomic_load_2((unsigned short *)(obj)),           \
    int:            (int)_atomic_load_2((unsigned short *)(obj)),      \
    unsigned int:   _atomic_load_2((unsigned short *)(obj)),           \
    default:        __atomic_unsupported_value(obj)                     \
)

#define atomic_load_explicit(obj, order) atomic_load(obj)

#define atomic_store(obj, val) _Generic(*(obj),                             \
    char:           _atomic_store_1((unsigned char *)(obj),  (unsigned char)(val)),  \
    signed char:    _atomic_store_1((unsigned char *)(obj),  (unsigned char)(val)),  \
    unsigned char:  _atomic_store_1((unsigned char *)(obj),  (unsigned char)(val)),  \
    short:          _atomic_store_2((unsigned short *)(obj), (unsigned short)(val)), \
    unsigned short: _atomic_store_2((unsigned short *)(obj), (unsigned short)(val)), \
    int:            _atomic_store_2((unsigned short *)(obj), (unsigned short)(val)), \
    unsigned int:   _atomic_store_2((unsigned short *)(obj), (unsigned short)(val)), \
    default:        __atomic_unsupported_void()                                        \
)

#define atomic_store_explicit(obj, val, order) atomic_store(obj, val)

#define atomic_exchange(obj, val) _Generic(*(obj),                              \
    char:           (char)_atomic_exchange_1((unsigned char *)(obj),  (unsigned char)(val)),   \
    signed char:    (signed char)_atomic_exchange_1((unsigned char *)(obj), (unsigned char)(val)), \
    unsigned char:  _atomic_exchange_1((unsigned char *)(obj),  (unsigned char)(val)),         \
    short:          (short)_atomic_exchange_2((unsigned short *)(obj),(unsigned short)(val)),  \
    unsigned short: _atomic_exchange_2((unsigned short *)(obj),(unsigned short)(val)),         \
    int:            (int)_atomic_exchange_2((unsigned short *)(obj),  (unsigned short)(val)),  \
    unsigned int:   _atomic_exchange_2((unsigned short *)(obj),(unsigned short)(val)),         \
    default:        __atomic_unsupported_value(obj)                                               \
)

#define atomic_exchange_explicit(obj, val, order) atomic_exchange(obj, val)

/* Compare-exchange is always strong with the current DI/EI wrappers. */
#define atomic_compare_exchange_strong(obj, exp, des) _Generic(*(obj),      \
    unsigned char: _atomic_compare_exchange_1((unsigned char *)(obj),      \
        (unsigned char)*(exp), (unsigned char)(des)),                        \
    char:          _atomic_compare_exchange_1((unsigned char *)(obj),      \
        (unsigned char)*(exp), (unsigned char)(des)),                        \
    unsigned short: _atomic_compare_exchange_2((unsigned short *)(obj),    \
        (unsigned short)*(exp), (unsigned short)(des)),                      \
    int: _atomic_compare_exchange_2((unsigned short *)(obj),               \
        (unsigned short)*(exp), (unsigned short)(des)),                      \
    default: (_atomic_width_not_supported(), 0)                             \
)

#define atomic_compare_exchange_weak(obj, exp, des) \
    atomic_compare_exchange_strong(obj, exp, des)

#define atomic_compare_exchange_strong_explicit(obj, exp, des, s, f) \
    atomic_compare_exchange_strong(obj, exp, des)
#define atomic_compare_exchange_weak_explicit(obj, exp, des, s, f) \
    atomic_compare_exchange_strong(obj, exp, des)

#define atomic_fetch_add(obj, val) _Generic(*(obj),                         \
    unsigned char: _atomic_fetch_add_1((unsigned char *)(obj),(unsigned char)(val)),       \
    char:          (char)_atomic_fetch_add_1((unsigned char *)(obj),(unsigned char)(val)), \
    unsigned short: _atomic_fetch_add_2((unsigned short *)(obj),(unsigned short)(val)),    \
    int:           (int)_atomic_fetch_add_2((unsigned short *)(obj),(unsigned short)(val)),\
    unsigned int:  _atomic_fetch_add_2((unsigned short *)(obj),(unsigned short)(val)),     \
    default:       __atomic_unsupported_value(obj)                                            \
)
#define atomic_fetch_add_explicit(obj, val, order) atomic_fetch_add(obj, val)

#define atomic_fetch_sub(obj, val) _Generic(*(obj),                         \
    unsigned char: _atomic_fetch_sub_1((unsigned char *)(obj),(unsigned char)(val)),       \
    char:          (char)_atomic_fetch_sub_1((unsigned char *)(obj),(unsigned char)(val)), \
    unsigned short: _atomic_fetch_sub_2((unsigned short *)(obj),(unsigned short)(val)),    \
    int:           (int)_atomic_fetch_sub_2((unsigned short *)(obj),(unsigned short)(val)),\
    unsigned int:  _atomic_fetch_sub_2((unsigned short *)(obj),(unsigned short)(val)),     \
    default:       __atomic_unsupported_value(obj)                                            \
)
#define atomic_fetch_sub_explicit(obj, val, order) atomic_fetch_sub(obj, val)

#define atomic_fetch_and(obj, val) _Generic(*(obj),                         \
    unsigned char: _atomic_fetch_and_1((unsigned char *)(obj),(unsigned char)(val)),       \
    char:          (char)_atomic_fetch_and_1((unsigned char *)(obj),(unsigned char)(val)), \
    unsigned short: _atomic_fetch_and_2((unsigned short *)(obj),(unsigned short)(val)),    \
    int:           (int)_atomic_fetch_and_2((unsigned short *)(obj),(unsigned short)(val)),\
    unsigned int:  _atomic_fetch_and_2((unsigned short *)(obj),(unsigned short)(val)),     \
    default:       __atomic_unsupported_value(obj)                                            \
)
#define atomic_fetch_and_explicit(obj, val, order) atomic_fetch_and(obj, val)

#define atomic_fetch_or(obj, val) _Generic(*(obj),                          \
    unsigned char: _atomic_fetch_or_1((unsigned char *)(obj),(unsigned char)(val)),        \
    char:          (char)_atomic_fetch_or_1((unsigned char *)(obj),(unsigned char)(val)),  \
    unsigned short: _atomic_fetch_or_2((unsigned short *)(obj),(unsigned short)(val)),     \
    int:           (int)_atomic_fetch_or_2((unsigned short *)(obj),(unsigned short)(val)), \
    unsigned int:  _atomic_fetch_or_2((unsigned short *)(obj),(unsigned short)(val)),      \
    default:       __atomic_unsupported_value(obj)                                            \
)
#define atomic_fetch_or_explicit(obj, val, order) atomic_fetch_or(obj, val)

#define atomic_fetch_xor(obj, val) _Generic(*(obj),                         \
    unsigned char: _atomic_fetch_xor_1((unsigned char *)(obj),(unsigned char)(val)),       \
    char:          (char)_atomic_fetch_xor_1((unsigned char *)(obj),(unsigned char)(val)), \
    unsigned short: _atomic_fetch_xor_2((unsigned short *)(obj),(unsigned short)(val)),    \
    int:           (int)_atomic_fetch_xor_2((unsigned short *)(obj),(unsigned short)(val)),\
    unsigned int:  _atomic_fetch_xor_2((unsigned short *)(obj),(unsigned short)(val)),     \
    default:       __atomic_unsupported_value(obj)                                            \
)
#define atomic_fetch_xor_explicit(obj, val, order) atomic_fetch_xor(obj, val)

/* Atomic-flag helpers. */
#define atomic_flag_test_and_set(flag)          _atomic_flag_test_set(flag)
#define atomic_flag_test_and_set_explicit(f, o) _atomic_flag_test_set(f)
#define atomic_flag_clear(flag)                 _atomic_flag_clear(flag)
#define atomic_flag_clear_explicit(f, o)        _atomic_flag_clear(f)

/* Fences collapse to no-ops on the current single-core target. */
#define atomic_thread_fence(order)   ((void)(order))
#define atomic_signal_fence(order)   ((void)(order))
#define atomic_is_lock_free(obj)     ((sizeof(*(obj)) <= 2U) ? 1 : 0)

/* Lock-freedom constants. */
#define ATOMIC_BOOL_LOCK_FREE      1
#define ATOMIC_CHAR_LOCK_FREE      1
#define ATOMIC_CHAR8_T_LOCK_FREE   1
#define ATOMIC_SHORT_LOCK_FREE     1
#define ATOMIC_INT_LOCK_FREE       1
#define ATOMIC_LONG_LOCK_FREE      0
#define ATOMIC_LLONG_LOCK_FREE     0
#define ATOMIC_POINTER_LOCK_FREE   1
#define ATOMIC_WCHAR_T_LOCK_FREE   1
#define ATOMIC_CHAR16_T_LOCK_FREE  1
#define ATOMIC_CHAR32_T_LOCK_FREE  0

typedef _Atomic unsigned char atomic_char8_t;

#endif /* _STDATOMIC_H */
