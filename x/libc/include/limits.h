/*
 * limits.h
 *
 * Standard C23 integer limits for the xcc Z80 target.
 *
 * Target data model:
 *   char      1 byte (target-default signedness)
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

#define __STDC_VERSION_LIMITS_H__ 202311L

/* Bits per byte. */
#define CHAR_BIT        8

/* Signed and unsigned char limits. */
#define SCHAR_MIN       (-128)
#define SCHAR_MAX       127
#define UCHAR_MAX       255U

/* Plain char follows the compiler target default. */
#ifdef __CHAR_UNSIGNED__
#define CHAR_MIN        0
#define CHAR_MAX        UCHAR_MAX
#else
#define CHAR_MIN        SCHAR_MIN
#define CHAR_MAX        SCHAR_MAX
#endif

/* short and unsigned short limits. */
#define SHRT_MIN        (-32768)
#define SHRT_MAX        32767
#define USHRT_MAX       65535U

/* int and unsigned int limits. */
#define INT_MIN         (-32768)
#define INT_MAX         32767
#define UINT_MAX        65535U

/* long and unsigned long limits. */
#define LONG_MIN        (-2147483648L)
#define LONG_MAX        2147483647L
#define ULONG_MAX       4294967295UL

/* long long and unsigned long long limits. */
#define LLONG_MIN       (-9223372036854775807LL - 1LL)
#define LLONG_MAX       9223372036854775807LL
#define ULLONG_MAX      18446744073709551615ULL

/* The target uses single-byte execution characters. */
#define MB_LEN_MAX      1

/* C23 representation-width macros. */
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
#define BITINT_MAXWIDTH  64

#endif /* _LIMITS_H */
