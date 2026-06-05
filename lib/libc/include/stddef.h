/*
 * stddef.h
 *
 * Standard C23 common definitions for the xcc Z80 target.
 *
 * On this target, pointers and size_t are 16-bit quantities. wchar_t is also
 * represented as a 16-bit signed integer until a wider wide-character runtime
 * is introduced.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDDEF_H
#define _STDDEF_H

#define __STDC_VERSION_STDDEF_H__ 202311L

/* Null pointer constant. */
#ifndef NULL
#  define NULL ((void *)0)
#endif

/* Core object and pointer-related types. */
typedef int ptrdiff_t;
typedef unsigned int size_t;
typedef int wchar_t;

/* Compute the byte offset of a member within a structure type. */
#define offsetof(type, member) ((size_t)(&((type *)0)->member))

/* Compiler-facing utility hooks. */
#define unreachable() __builtin_unreachable()

#endif /* _STDDEF_H */
