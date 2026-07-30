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

#include <limits.h>

#define __STDC_VERSION_STDCKDINT_H__ 202311L

/*
 * xcc currently implements C int as 16 bits and long as 32 bits, so a long
 * can represent every int addition, subtraction, and multiplication result.
 * Keep the operations in that wider type: this avoids signed-overflow UB and
 * also avoids exposing private assembler entry points through a public header.
 */
#define ckd_add(result, a, b) \
    ({ long __w = (long)(a) + (long)(b); \
       *(result) = (int)__w; \
       (__w > (long)INT_MAX || __w < (long)INT_MIN); })

#define ckd_sub(result, a, b) \
    ({ long __w = (long)(a) - (long)(b); \
       *(result) = (int)__w; \
       (__w > (long)INT_MAX || __w < (long)INT_MIN); })

#define ckd_mul(result, a, b) \
    ({ long __w = (long)(a) * (long)(b); \
       *(result) = (int)__w; \
       (__w > (long)INT_MAX || __w < (long)INT_MIN); })

#endif /* _STDCKDINT_H */
