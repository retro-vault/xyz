#ifndef _STDINT_H
#define _STDINT_H

typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef long               int32_t;
typedef unsigned long      uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;

typedef int                intptr_t;
typedef unsigned int       uintptr_t;

typedef long               intmax_t;
typedef unsigned long      uintmax_t;

/* Least-width types */
typedef signed char        int_least8_t;
typedef unsigned char      uint_least8_t;
typedef short              int_least16_t;
typedef unsigned short     uint_least16_t;
typedef long               int_least32_t;
typedef unsigned long      uint_least32_t;
typedef long long          int_least64_t;
typedef unsigned long long uint_least64_t;

/* Fast types (same as least on Z80) */
typedef signed char        int_fast8_t;
typedef unsigned char      uint_fast8_t;
typedef short              int_fast16_t;
typedef unsigned short     uint_fast16_t;
typedef long               int_fast32_t;
typedef unsigned long      uint_fast32_t;
typedef long long          int_fast64_t;
typedef unsigned long long uint_fast64_t;

#define INT8_MIN   (-128)
#define INT8_MAX   127
#define UINT8_MAX  255
#define INT16_MIN  (-32768)
#define INT16_MAX  32767
#define UINT16_MAX 65535
#define INT32_MIN  (-2147483648L)
#define INT32_MAX  2147483647L
#define UINT32_MAX 4294967295UL

#define INT8_C(x)   (x)
#define UINT8_C(x)  (x)       /* uint_least8_t = uchar, promotes to int */
#define INT16_C(x)  (x)
#define UINT16_C(x) (x ## U)
#define INT32_C(x)  (x ## L)
#define UINT32_C(x) (x ## UL)
#define INT64_C(x)  (x ## LL)
#define UINT64_C(x) (x ## ULL)
#define INTMAX_C(x)  (x ## L)
#define UINTMAX_C(x) (x ## UL)

#define INT64_MIN  (-9223372036854775807LL - 1)
#define INT64_MAX  9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL
#define INTPTR_MIN (-2147483648)
#define INTPTR_MAX 2147483647
#define UINTPTR_MAX 4294967295U
#define INTMAX_MIN  INT32_MIN
#define INTMAX_MAX  INT32_MAX
#define UINTMAX_MAX UINT32_MAX

#define INT_LEAST8_MIN   INT8_MIN
#define INT_LEAST8_MAX   INT8_MAX
#define UINT_LEAST8_MAX  UINT8_MAX
#define INT_LEAST16_MIN  INT16_MIN
#define INT_LEAST16_MAX  INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define INT_LEAST32_MIN  INT32_MIN
#define INT_LEAST32_MAX  INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define INT_LEAST64_MIN  INT64_MIN
#define INT_LEAST64_MAX  INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX

#define INT_FAST8_MIN   INT8_MIN
#define INT_FAST8_MAX   INT8_MAX
#define UINT_FAST8_MAX  UINT8_MAX
#define INT_FAST16_MIN  INT16_MIN
#define INT_FAST16_MAX  INT16_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define INT_FAST32_MIN  INT32_MIN
#define INT_FAST32_MAX  INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define INT_FAST64_MIN  INT64_MIN
#define INT_FAST64_MAX  INT64_MAX
#define UINT_FAST64_MAX UINT64_MAX

#define PTRDIFF_MIN INT16_MIN
#define PTRDIFF_MAX INT16_MAX
#define SIZE_MAX    UINT16_MAX
#define SIG_ATOMIC_MIN INT8_MIN
#define SIG_ATOMIC_MAX INT8_MAX
#define WCHAR_MIN   0
#define WCHAR_MAX   255
#define WINT_MIN    0
#define WINT_MAX    65535

#endif
