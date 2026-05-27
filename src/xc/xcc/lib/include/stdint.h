//
// stdint.h — C11 <stdint.h> for the xcc Z80 compiler.
//
// Type sizes on the xcc Z80 target:
//   char / unsigned char     1 byte
//   short / int              2 bytes
//   long                     4 bytes
//   long long                8 bytes
//   pointer                  2 bytes  (16-bit flat address space)
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#ifndef _STDINT_H
#define _STDINT_H

// ----- Exact-width integer types -------------------------------------

typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef long               int32_t;
typedef unsigned long      uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;

// ----- Least-width integer types -------------------------------------

typedef int8_t             int_least8_t;
typedef uint8_t            uint_least8_t;
typedef int16_t            int_least16_t;
typedef uint16_t           uint_least16_t;
typedef int32_t            int_least32_t;
typedef uint32_t           uint_least32_t;
typedef int64_t            int_least64_t;
typedef uint64_t           uint_least64_t;

// ----- Fastest minimum-width integer types ---------------------------
// On Z80, int (2 bytes) is the native word; 8-bit is also native via A.

typedef int8_t             int_fast8_t;
typedef uint8_t            uint_fast8_t;
typedef int16_t            int_fast16_t;
typedef uint16_t           uint_fast16_t;
typedef int32_t            int_fast32_t;
typedef uint32_t           uint_fast32_t;
typedef int64_t            int_fast64_t;
typedef uint64_t           uint_fast64_t;

// ----- Integer types capable of holding a pointer --------------------

typedef short              intptr_t;
typedef unsigned short     uintptr_t;

// ----- Greatest-width integer types ----------------------------------

typedef long long          intmax_t;
typedef unsigned long long uintmax_t;

// ----- Limits of exact-width types -----------------------------------

#define INT8_MIN    (-128)
#define INT8_MAX    (127)
#define UINT8_MAX   (255U)

#define INT16_MIN   (-32768)
#define INT16_MAX   (32767)
#define UINT16_MAX  (65535U)

#define INT32_MIN   (-2147483648L)
#define INT32_MAX   (2147483647L)
#define UINT32_MAX  (4294967295UL)

#define INT64_MIN   (-9223372036854775808LL)
#define INT64_MAX   (9223372036854775807LL)
#define UINT64_MAX  (18446744073709551615ULL)

// ----- Limits of least-width types -----------------------------------

#define INT_LEAST8_MIN    INT8_MIN
#define INT_LEAST8_MAX    INT8_MAX
#define UINT_LEAST8_MAX   UINT8_MAX

#define INT_LEAST16_MIN   INT16_MIN
#define INT_LEAST16_MAX   INT16_MAX
#define UINT_LEAST16_MAX  UINT16_MAX

#define INT_LEAST32_MIN   INT32_MIN
#define INT_LEAST32_MAX   INT32_MAX
#define UINT_LEAST32_MAX  UINT32_MAX

#define INT_LEAST64_MIN   INT64_MIN
#define INT_LEAST64_MAX   INT64_MAX
#define UINT_LEAST64_MAX  UINT64_MAX

// ----- Limits of fastest minimum-width types -------------------------

#define INT_FAST8_MIN     INT8_MIN
#define INT_FAST8_MAX     INT8_MAX
#define UINT_FAST8_MAX    UINT8_MAX

#define INT_FAST16_MIN    INT16_MIN
#define INT_FAST16_MAX    INT16_MAX
#define UINT_FAST16_MAX   UINT16_MAX

#define INT_FAST32_MIN    INT32_MIN
#define INT_FAST32_MAX    INT32_MAX
#define UINT_FAST32_MAX   UINT32_MAX

#define INT_FAST64_MIN    INT64_MIN
#define INT_FAST64_MAX    INT64_MAX
#define UINT_FAST64_MAX   UINT64_MAX

// ----- Limits of pointer-holding types -------------------------------

#define INTPTR_MIN    INT16_MIN
#define INTPTR_MAX    INT16_MAX
#define UINTPTR_MAX   UINT16_MAX

// ----- Limits of greatest-width types --------------------------------

#define INTMAX_MIN    INT64_MIN
#define INTMAX_MAX    INT64_MAX
#define UINTMAX_MAX   UINT64_MAX

// ----- Limits of other integer types (from <limits.h>) ---------------

#define PTRDIFF_MIN   INT16_MIN
#define PTRDIFF_MAX   INT16_MAX

#define SIZE_MAX      UINT16_MAX

#define SIG_ATOMIC_MIN INT8_MIN
#define SIG_ATOMIC_MAX INT8_MAX

#define WCHAR_MIN     INT16_MIN
#define WCHAR_MAX     INT16_MAX

#define WINT_MIN      0
#define WINT_MAX      UINT16_MAX

// ----- Macros for integer constants ----------------------------------

#define INT8_C(v)    ((int8_t)(v))
#define UINT8_C(v)   ((uint8_t)(v ## U))
#define INT16_C(v)   ((int16_t)(v))
#define UINT16_C(v)  ((uint16_t)(v ## U))
#define INT32_C(v)   (v ## L)
#define UINT32_C(v)  (v ## UL)
#define INT64_C(v)   (v ## LL)
#define UINT64_C(v)  (v ## ULL)

#define INTMAX_C(v)  INT64_C(v)
#define UINTMAX_C(v) UINT64_C(v)

#endif // _STDINT_H
