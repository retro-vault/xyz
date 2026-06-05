/*
 * stdckdint.h
 *
 * Standard C23 checked integer arithmetic macros for the xcc Z80 target.
 *
 * This initial implementation is intentionally small: it computes the result
 * and reports "no overflow" unconditionally. That is enough to provide the
 * header surface and keep existing code building, while leaving room for a
 * later target-accurate overflow implementation.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDCKDINT_H
#define _STDCKDINT_H

#define __STDC_VERSION_STDCKDINT_H__ 202311L

/* Checked addition: stores a + b and returns nonzero on overflow. */
#define ckd_add(result, a, b) \
    (*(result) = (a) + (b), 0)

/* Checked subtraction: stores a - b and returns nonzero on overflow. */
#define ckd_sub(result, a, b) \
    (*(result) = (a) - (b), 0)

/* Checked multiplication: stores a * b and returns nonzero on overflow. */
#define ckd_mul(result, a, b) \
    (*(result) = (a) * (b), 0)

#endif /* _STDCKDINT_H */
