/*
 * limits.h
 *
 * standard C header file
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 02.05.2021   tstih
 *
 */
#ifndef __LIMITS_H__
#define __LIMITS_H__

#define SSIZE_MAX   256                 /* max value for size_t */

#define CHAR_BIT    8                   /* number of bits in byte */
#define SCHAR_MIN   -128
#define SCHAR_MAX   +127
#define UCHAR_MAX   255
#define CHAR_MIN    -128
#define CHAR_MAX    +127
#define SHRT_MIN    -128
#define SHRT_MAX    +127
#define USHRT_MAX   255
#define INT_MIN     -32768
#define INT_MAX     +32767
#define UINT_MAX    65535
#define LONG_MIN    -2147483648
#define LONG_MAX    +2147483647
#define ULONG_MAX   4294967295

#endif /* __LIMITS_H__ */