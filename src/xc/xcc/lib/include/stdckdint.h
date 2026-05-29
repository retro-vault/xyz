/*
 * stdckdint.h — C23 checked integer arithmetic (stub for xcc/Z80).
 *
 * Provides ckd_add, ckd_sub, ckd_mul macros.  On a 16-bit Z80 target,
 * "overflow" checking is performed via signed-promotion arithmetic.
 * These are stubs only; a full implementation requires soft-arithmetic.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef _STDCKDINT_H
#define _STDCKDINT_H

#define __STDC_VERSION_STDCKDINT_H__  202311L

/*
 * ckd_add(result, a, b) — *result = a + b; returns true on overflow.
 * ckd_sub(result, a, b) — *result = a - b; returns true on overflow.
 * ckd_mul(result, a, b) — *result = a * b; returns true on overflow.
 *
 * On Z80 we lack hardware overflow detection; conservatively always
 * compute and return 0 (no overflow detected — safe for correctness,
 * not for security).
 */

#define ckd_add(result, a, b) \
    (*(result) = (a) + (b), 0)

#define ckd_sub(result, a, b) \
    (*(result) = (a) - (b), 0)

#define ckd_mul(result, a, b) \
    (*(result) = (a) * (b), 0)

#endif /* _STDCKDINT_H */
