//
// stdatomic.h — C11 <stdatomic.h> for the xcc Z80 compiler.
//
// On a single-core preemptive Z80 system all atomic operations are
// serialised with DI/EI in the runtime stubs below.  Memory-order
// parameters are accepted but ignored (there is only one core).
//
// OS integration: override __tls_base in your kernel; the stub in
// lib/runtime/ returns NULL.  Every thread must initialise its TLS
// block from __tls_template before running.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#ifndef _STDATOMIC_H
#define _STDATOMIC_H

// ----- memory_order --------------------------------------------------
typedef enum {
    memory_order_relaxed = 0,
    memory_order_consume = 1,
    memory_order_acquire = 2,
    memory_order_release = 3,
    memory_order_acq_rel = 4,
    memory_order_seq_cst = 5
} memory_order;

// ----- atomic types --------------------------------------------------
// On Z80 all atomic types are plain integral types; the DI/EI wrappers
// in the runtime stubs provide the required mutual exclusion.

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

// ----- runtime stubs -------------------------------------------------
// Declared here; defined in lib/runtime/.

extern unsigned char  __atomic_load_1(unsigned char *ptr);
extern unsigned short __atomic_load_2(unsigned short *ptr);
extern void __atomic_store_1(unsigned char  *ptr, unsigned char  val);
extern void __atomic_store_2(unsigned short *ptr, unsigned short val);
extern unsigned char  __atomic_exchange_1(unsigned char  *ptr, unsigned char  val);
extern unsigned short __atomic_exchange_2(unsigned short *ptr, unsigned short val);
extern int __atomic_compare_exchange_1(unsigned char  *ptr, unsigned char  expected, unsigned char  desired);
extern int __atomic_compare_exchange_2(unsigned short *ptr, unsigned short expected, unsigned short desired);
extern unsigned char  __atomic_fetch_add_1(unsigned char  *ptr, unsigned char  val);
extern unsigned short __atomic_fetch_add_2(unsigned short *ptr, unsigned short val);
extern unsigned char  __atomic_fetch_sub_1(unsigned char  *ptr, unsigned char  val);
extern unsigned short __atomic_fetch_sub_2(unsigned short *ptr, unsigned short val);
extern unsigned char  __atomic_fetch_and_1(unsigned char  *ptr, unsigned char  val);
extern unsigned short __atomic_fetch_and_2(unsigned short *ptr, unsigned short val);
extern unsigned char  __atomic_fetch_or_1(unsigned char  *ptr, unsigned char  val);
extern unsigned short __atomic_fetch_or_2(unsigned short *ptr, unsigned short val);
extern unsigned char  __atomic_fetch_xor_1(unsigned char  *ptr, unsigned char  val);
extern unsigned short __atomic_fetch_xor_2(unsigned short *ptr, unsigned short val);
extern unsigned char  __atomic_flag_test_set(unsigned char *flag);
extern void           __atomic_flag_clear(unsigned char *flag);

// ----- ATOMIC_VAR_INIT / ATOMIC_FLAG_INIT ----------------------------
#define ATOMIC_VAR_INIT(val)    (val)
#define ATOMIC_FLAG_INIT        0

// ----- atomic_init ---------------------------------------------------
#define atomic_init(obj, val)   (*(obj) = (val))

// ----- size-selecting load/store/exchange helpers --------------------
// _Generic selects on the dereferenced type so 1-byte types use _1
// variants and 2-byte types use _2 variants.

#define atomic_load(obj) _Generic(*(obj),                               \
    char:           (char)__atomic_load_1((unsigned char *)(obj)),      \
    signed char:    (signed char)__atomic_load_1((unsigned char *)(obj)),\
    unsigned char:  __atomic_load_1((unsigned char *)(obj)),            \
    short:          (short)__atomic_load_2((unsigned short *)(obj)),    \
    unsigned short: __atomic_load_2((unsigned short *)(obj)),           \
    int:            (int)__atomic_load_2((unsigned short *)(obj)),      \
    unsigned int:   __atomic_load_2((unsigned short *)(obj)),           \
    default:        __atomic_load_2((unsigned short *)(obj))            \
)

#define atomic_load_explicit(obj, order) atomic_load(obj)

#define atomic_store(obj, val) _Generic(*(obj),                             \
    char:           __atomic_store_1((unsigned char *)(obj),  (unsigned char)(val)),  \
    signed char:    __atomic_store_1((unsigned char *)(obj),  (unsigned char)(val)),  \
    unsigned char:  __atomic_store_1((unsigned char *)(obj),  (unsigned char)(val)),  \
    short:          __atomic_store_2((unsigned short *)(obj), (unsigned short)(val)), \
    unsigned short: __atomic_store_2((unsigned short *)(obj), (unsigned short)(val)), \
    int:            __atomic_store_2((unsigned short *)(obj), (unsigned short)(val)), \
    unsigned int:   __atomic_store_2((unsigned short *)(obj), (unsigned short)(val)), \
    default:        __atomic_store_2((unsigned short *)(obj), (unsigned short)(val))  \
)

#define atomic_store_explicit(obj, val, order) atomic_store(obj, val)

#define atomic_exchange(obj, val) _Generic(*(obj),                              \
    char:           (char)__atomic_exchange_1((unsigned char *)(obj),  (unsigned char)(val)),   \
    signed char:    (signed char)__atomic_exchange_1((unsigned char *)(obj), (unsigned char)(val)), \
    unsigned char:  __atomic_exchange_1((unsigned char *)(obj),  (unsigned char)(val)),         \
    short:          (short)__atomic_exchange_2((unsigned short *)(obj),(unsigned short)(val)),  \
    unsigned short: __atomic_exchange_2((unsigned short *)(obj),(unsigned short)(val)),         \
    int:            (int)__atomic_exchange_2((unsigned short *)(obj),  (unsigned short)(val)),  \
    unsigned int:   __atomic_exchange_2((unsigned short *)(obj),(unsigned short)(val)),         \
    default:        __atomic_exchange_2((unsigned short *)(obj),(unsigned short)(val))          \
)

#define atomic_exchange_explicit(obj, val, order) atomic_exchange(obj, val)

// compare_exchange: on Z80/di/ei, the exchange is always "strong" (no spurious failures).
#define atomic_compare_exchange_strong(obj, exp, des) _Generic(*(obj),      \
    unsigned char: __atomic_compare_exchange_1((unsigned char *)(obj),      \
        (unsigned char)*(exp), (unsigned char)(des)),                        \
    char:          __atomic_compare_exchange_1((unsigned char *)(obj),      \
        (unsigned char)*(exp), (unsigned char)(des)),                        \
    unsigned short: __atomic_compare_exchange_2((unsigned short *)(obj),    \
        (unsigned short)*(exp), (unsigned short)(des)),                      \
    int: __atomic_compare_exchange_2((unsigned short *)(obj),               \
        (unsigned short)*(exp), (unsigned short)(des)),                      \
    default: __atomic_compare_exchange_2((unsigned short *)(obj),           \
        (unsigned short)*(exp), (unsigned short)(des))                       \
)

#define atomic_compare_exchange_weak(obj, exp, des) \
    atomic_compare_exchange_strong(obj, exp, des)

#define atomic_compare_exchange_strong_explicit(obj, exp, des, s, f) \
    atomic_compare_exchange_strong(obj, exp, des)
#define atomic_compare_exchange_weak_explicit(obj, exp, des, s, f) \
    atomic_compare_exchange_strong(obj, exp, des)

// fetch-modify macros
#define atomic_fetch_add(obj, val) _Generic(*(obj),                         \
    unsigned char: __atomic_fetch_add_1((unsigned char *)(obj),(unsigned char)(val)),       \
    char:          (char)__atomic_fetch_add_1((unsigned char *)(obj),(unsigned char)(val)), \
    unsigned short: __atomic_fetch_add_2((unsigned short *)(obj),(unsigned short)(val)),    \
    int:           (int)__atomic_fetch_add_2((unsigned short *)(obj),(unsigned short)(val)),\
    unsigned int:  __atomic_fetch_add_2((unsigned short *)(obj),(unsigned short)(val)),     \
    default:       __atomic_fetch_add_2((unsigned short *)(obj),(unsigned short)(val))      \
)
#define atomic_fetch_add_explicit(obj, val, order) atomic_fetch_add(obj, val)

#define atomic_fetch_sub(obj, val) _Generic(*(obj),                         \
    unsigned char: __atomic_fetch_sub_1((unsigned char *)(obj),(unsigned char)(val)),       \
    char:          (char)__atomic_fetch_sub_1((unsigned char *)(obj),(unsigned char)(val)), \
    unsigned short: __atomic_fetch_sub_2((unsigned short *)(obj),(unsigned short)(val)),    \
    int:           (int)__atomic_fetch_sub_2((unsigned short *)(obj),(unsigned short)(val)),\
    unsigned int:  __atomic_fetch_sub_2((unsigned short *)(obj),(unsigned short)(val)),     \
    default:       __atomic_fetch_sub_2((unsigned short *)(obj),(unsigned short)(val))      \
)
#define atomic_fetch_sub_explicit(obj, val, order) atomic_fetch_sub(obj, val)

#define atomic_fetch_and(obj, val) _Generic(*(obj),                         \
    unsigned char: __atomic_fetch_and_1((unsigned char *)(obj),(unsigned char)(val)),       \
    char:          (char)__atomic_fetch_and_1((unsigned char *)(obj),(unsigned char)(val)), \
    unsigned short: __atomic_fetch_and_2((unsigned short *)(obj),(unsigned short)(val)),    \
    int:           (int)__atomic_fetch_and_2((unsigned short *)(obj),(unsigned short)(val)),\
    unsigned int:  __atomic_fetch_and_2((unsigned short *)(obj),(unsigned short)(val)),     \
    default:       __atomic_fetch_and_2((unsigned short *)(obj),(unsigned short)(val))      \
)
#define atomic_fetch_and_explicit(obj, val, order) atomic_fetch_and(obj, val)

#define atomic_fetch_or(obj, val) _Generic(*(obj),                          \
    unsigned char: __atomic_fetch_or_1((unsigned char *)(obj),(unsigned char)(val)),        \
    char:          (char)__atomic_fetch_or_1((unsigned char *)(obj),(unsigned char)(val)),  \
    unsigned short: __atomic_fetch_or_2((unsigned short *)(obj),(unsigned short)(val)),     \
    int:           (int)__atomic_fetch_or_2((unsigned short *)(obj),(unsigned short)(val)), \
    unsigned int:  __atomic_fetch_or_2((unsigned short *)(obj),(unsigned short)(val)),      \
    default:       __atomic_fetch_or_2((unsigned short *)(obj),(unsigned short)(val))       \
)
#define atomic_fetch_or_explicit(obj, val, order) atomic_fetch_or(obj, val)

#define atomic_fetch_xor(obj, val) _Generic(*(obj),                         \
    unsigned char: __atomic_fetch_xor_1((unsigned char *)(obj),(unsigned char)(val)),       \
    char:          (char)__atomic_fetch_xor_1((unsigned char *)(obj),(unsigned char)(val)), \
    unsigned short: __atomic_fetch_xor_2((unsigned short *)(obj),(unsigned short)(val)),    \
    int:           (int)__atomic_fetch_xor_2((unsigned short *)(obj),(unsigned short)(val)),\
    unsigned int:  __atomic_fetch_xor_2((unsigned short *)(obj),(unsigned short)(val)),     \
    default:       __atomic_fetch_xor_2((unsigned short *)(obj),(unsigned short)(val))      \
)
#define atomic_fetch_xor_explicit(obj, val, order) atomic_fetch_xor(obj, val)

// ----- atomic_flag operations ----------------------------------------
#define atomic_flag_test_and_set(flag)          __atomic_flag_test_set(flag)
#define atomic_flag_test_and_set_explicit(f, o) __atomic_flag_test_set(f)
#define atomic_flag_clear(flag)                 __atomic_flag_clear(flag)
#define atomic_flag_clear_explicit(f, o)        __atomic_flag_clear(f)

// ----- no-op fence (single-core Z80 has no reordering) ---------------
#define atomic_thread_fence(order)   ((void)(order))
#define atomic_signal_fence(order)   ((void)(order))
#define atomic_is_lock_free(obj)     1

// ----- ATOMIC_*_LOCK_FREE constants ----------------------------------
#define ATOMIC_BOOL_LOCK_FREE     1
#define ATOMIC_CHAR_LOCK_FREE     1
#define ATOMIC_SHORT_LOCK_FREE    1
#define ATOMIC_INT_LOCK_FREE      1
#define ATOMIC_LONG_LOCK_FREE     0  // 32-bit needs 4 bytes; not atomic on Z80 without DI/EI
#define ATOMIC_POINTER_LOCK_FREE  1

#endif // _STDATOMIC_H
