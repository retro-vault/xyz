/*
 * stdbool.h
 *
 * Standard C boolean convenience definitions for the xcc Z80 target.
 *
 * In C23, bool/true/false are language keywords and this header is effectively
 * reduced to the legacy feature macro. In pre-C23 modes, the standard macro
 * spellings are provided in the usual way.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDBOOL_H
#define _STDBOOL_H

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#  define bool  _Bool
#  define true  1
#  define false 0
#endif

/* Required by the C standard, obsolescent in C23. */
#define __bool_true_false_are_defined 1

#endif /* _STDBOOL_H */
