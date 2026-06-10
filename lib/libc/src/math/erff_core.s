        ;; erff_core.s
        ;;
        ;; Shared single-precision error-function kernel for the xcc Z80 libc.
        ;;
        ;; Uses Winitzki's compact approximation
        ;;   erf(x) ≈ sign(x) * sqrt(1 - exp(-x² * ((4/π) + a x²)/(1 + a x²)))
        ;; with a = 0.147. It stays small, uses only the existing soft-float
        ;; helpers, and is accurate enough for libc-grade scalar math tests.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module erff_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_erff_core
        .globl  ___libc_fpclassifyf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsdiv
        .globl  _expf
        .globl  _sqrtf

        .area   _DATA
__erf_x:      .ds 4
__erf_x2:     .ds 4
__erf_t:      .ds 4
__erf_u:      .ds 4
__erf_sign:   .ds 1

        .area   _CODE

__erf_load_x:
        ld      de,(__erf_x)
        ld      hl,(__erf_x + 2)
        ret

__erf_load_x2:
        ld      de,(__erf_x2)
        ld      hl,(__erf_x2 + 2)
        ret

__erf_load_t:
        ld      de,(__erf_t)
        ld      hl,(__erf_t + 2)
        ret

__erf_load_u:
        ld      de,(__erf_u)
        ld      hl,(__erf_u + 2)
        ret

__libc_erff_core::
        ld      (__erf_x),de
        ld      (__erf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> preserve payload/sign.
        jp      z,__erf_ret_x
        cp      #1                      ; +/-Inf -> saturate to +/-1.
        jp      z,__erf_inf
        cp      #2                      ; Signed zero is already exact.
        jp      z,__erf_ret_x

        ld      a,h
        and     #0x80
        ld      (__erf_sign),a
        res     7,h
        ld      (__erf_x),de
        ld      (__erf_x + 2),hl

        ;; x2 = x * x
        ld      hl,(__erf_x + 2)
        push    hl
        ld      hl,(__erf_x)
        push    hl
        call    __erf_load_x
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__erf_x2),de
        ld      (__erf_x2 + 2),hl

        ;; t = a * x^2
        ld      hl,#0x3e16              ; a = 0.147
        push    hl
        ld      hl,#0x872b
        push    hl
        call    __erf_load_x2
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__erf_t),de
        ld      (__erf_t + 2),hl

        ;; u = x^2 * ((4/pi) + t)
        ld      hl,(__erf_t + 2)
        push    hl
        ld      hl,(__erf_t)
        push    hl
        ld      de,#0xf983
        ld      hl,#0x3fa2              ; 4/pi
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__erf_u),de
        ld      (__erf_u + 2),hl

        ld      hl,(__erf_u + 2)
        push    hl
        ld      hl,(__erf_u)
        push    hl
        call    __erf_load_x2
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__erf_u),de
        ld      (__erf_u + 2),hl

        ;; t = 1 + a*x^2
        ld      hl,(__erf_t + 2)
        push    hl
        ld      hl,(__erf_t)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__erf_t),de
        ld      (__erf_t + 2),hl

        ;; u = -u / t
        ld      hl,(__erf_t + 2)
        push    hl
        ld      hl,(__erf_t)
        push    hl
        call    __erf_load_u
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      a,h
        xor     #0x80
        ld      h,a

        ;; u = exp(u)
        call    _expf
        ld      (__erf_u),de
        ld      (__erf_u + 2),hl

        ;; u = sqrt(1 - u)
        ld      hl,(__erf_u + 2)
        push    hl
        ld      hl,(__erf_u)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        call    _sqrtf

        ;; Restore the original sign.
        ld      a,(__erf_sign)
        or      a
        ret     z
        set     7,h
        ret

__erf_inf:
        ld      de,#0x0000
        ld      a,(__erf_x + 3)
        and     #0x80
        or      #0x3f
        ld      h,a
        ld      l,#0x80                 ; +/-1.0f
        ret

__erf_ret_x:
        ld      de,(__erf_x)
        ld      hl,(__erf_x + 2)
        ret
