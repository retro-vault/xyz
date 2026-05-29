/*
 * limits.h — C11/C23 integer limits for the xcc Z80 target.
 *
 * Z80 type sizes:
 *   char      1 byte (signed)
 *   short     2 bytes
 *   int       2 bytes
 *   long      4 bytes
 *   long long 8 bytes
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef _LIMITS_H
#define _LIMITS_H

/* Number of bits in a char */
#define CHAR_BIT        8

/* char */
#define SCHAR_MIN       (-128)
#define SCHAR_MAX       127
#define UCHAR_MAX       255U

/* char (same as schar on this target) */
#define CHAR_MIN        SCHAR_MIN
#define CHAR_MAX        SCHAR_MAX

/* short (2 bytes) */
#define SHRT_MIN        (-32768)
#define SHRT_MAX        32767
#define USHRT_MAX       65535U

/* int (2 bytes on Z80) */
#define INT_MIN         (-32768)
#define INT_MAX         32767
#define UINT_MAX        65535U

/* long (4 bytes) */
#define LONG_MIN        (-2147483648L)
#define LONG_MAX        2147483647L
#define ULONG_MAX       4294967295UL

/* long long (8 bytes) */
#define LLONG_MIN       (-9223372036854775808LL)
#define LLONG_MAX       9223372036854775807LL
#define ULLONG_MAX      18446744073709551615ULL

/* MB_LEN_MAX: max bytes in a multibyte char */
#define MB_LEN_MAX      1

/* C23: value-representation bit-width macros */
#define BOOL_WIDTH       1
#define CHAR_WIDTH       8
#define SCHAR_WIDTH      8
#define UCHAR_WIDTH      8
#define SHRT_WIDTH       16
#define USHRT_WIDTH      16
#define INT_WIDTH        16
#define UINT_WIDTH       16
#define LONG_WIDTH       32
#define ULONG_WIDTH      32
#define LLONG_WIDTH      64
#define ULLONG_WIDTH     64
#define BITINT_MAXWIDTH  64   /* xcc/Z80 _BitInt maximum */

#endif /* _LIMITS_H */
