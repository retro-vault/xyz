/*
 * stdalign.h
 *
 * Standard C alignment convenience header for the xcc Z80 target.
 *
 * In pre-C23 modes this header provides the traditional alignas/alignof macro
 * aliases for the core language operators. In C23 and later, those names are
 * keywords, so the header intentionally stays empty.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDALIGN_H
#define _STDALIGN_H

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#  define alignas _Alignas
#  define alignof _Alignof
#  define __alignas_is_defined 1
#  define __alignof_is_defined 1
#endif

#endif /* _STDALIGN_H */
