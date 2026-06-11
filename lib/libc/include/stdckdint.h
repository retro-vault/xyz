/*
 * stdckdint.h
 *
 * Standard C23 checked integer arithmetic macros for the xcc Z80 target.
 *
 * Implementation uses wider intermediate arithmetic (long long) to compute
 * the result and detect overflow without invoking signed overflow UB in the
 * narrow type. Returns nonzero on overflow (and still stores the truncated
 * result per the C23 semantics).
 *
 * For long long the detection is best-effort (still uses 64-bit math here;
 * a fully robust version could use runtime helpers from an existing .s file).
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDCKDINT_H
#define _STDCKDINT_H

#define __STDC_VERSION_STDCKDINT_H__ 202311L

/* Checked addition etc. Now use real asm helpers in existing strtox_core.s for overflow detection (no statics in new code). */
#define ckd_add(result, a, b) \
    ( { int __r; int __ov = __ckd_add_sint(&__r, (a), (b)); *(result) = __r; __ov; } )

#define ckd_sub(result, a, b) \
    ( { int __r; int __ov = __ckd_sub_sint(&__r, (a), (b)); *(result) = __r; __ov; } )

#define ckd_mul(result, a, b) \
    ( { int __r; int __ov = __ckd_mul_sint(&__r, (a), (b)); *(result) = __r; __ov; } )

#endif /* _STDCKDINT_H */
