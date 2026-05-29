/*
 * stddef.h — C11/C23 common definitions for the xcc Z80 target.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef _STDDEF_H
#define _STDDEF_H

/* Null pointer constant */
#ifndef NULL
#  define NULL ((void *)0)
#endif

/* Signed size type */
typedef int           ptrdiff_t;

/* Unsigned size type (16-bit on Z80) */
typedef unsigned int  size_t;

/* Wide character type */
typedef int           wchar_t;

/* Offset of a struct member */
#define offsetof(type, member)  ((size_t)(&((type *)0)->member))

/* C23: unreachable() — marks a code path as never reached.
 * Backed by __builtin_unreachable() which emits HALT on Z80.
 */
#define unreachable()  __builtin_unreachable()

/* C23: __STDC_VERSION_STDDEF_H__ feature-test macro */
#define __STDC_VERSION_STDDEF_H__  202311L

#endif /* _STDDEF_H */
