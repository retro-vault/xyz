        ;; transf_core.s
        ;;
        ;; Shared single-precision transcendental helpers for the xcc Z80 libc.
        ;;
        ;; __libc_expf_core implements expf(x) via range reduction
        ;;   x = q*ln(2) + r,  q = round(x / ln(2)), |r| <= ln(2)/2
        ;; and a short Taylor polynomial for exp(r), then scales by 2^q.
        ;;
        ;; __libc_logf_core implements logf(x) by splitting x = m * 2^e with
        ;; frexpf(), nudging m into [sqrt(1/2), sqrt(2)), and evaluating
        ;;   log(m) = 2*s*(1 + z/3 + z^2/5 + z^3/7),
        ;;   s = (m - 1) / (m + 1), z = s^2
        ;; on a compact interval.
        ;;
        ;; These helpers are intentionally non-reentrant and reuse a small
        ;; shared scratch area, just like the rest of the handwritten math
        ;; slice in this libc.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module transf_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_expf_core
        .globl  __libc_logf_core

        .globl  ___libc_fpclassifyf
        .globl  ___libc_signbitf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsdiv
        .globl  ___fscmp
        .globl  ___fs2slong
        .globl  ___sint2fs
        .globl  _roundf
        .globl  _frexpf
        .globl  _ldexpf

        .area   _DATA
__tf_x:   .ds 4
__tf_y:   .ds 4
__tf_u:   .ds 4
__tf_v:   .ds 4
__tf_qf:  .ds 4
__tf_qi:  .ds 4
__tf_exp: .ds 2

        .area   _CODE

__tf_load_x:
        ld      de,(__tf_x)
        ld      hl,(__tf_x + 2)
        ret

__tf_load_y:
        ld      de,(__tf_y)
        ld      hl,(__tf_y + 2)
        ret

__tf_load_u:
        ld      de,(__tf_u)
        ld      hl,(__tf_u + 2)
        ret

__tf_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ret

__tf_ret_pinf:
        ld      hl,#0x7f80
        ld      de,#0x0000
        ret

__tf_ret_ninf:
        ld      hl,#0xff80
        ld      de,#0x0000
        ret

__tf_ret_zero:
        ld      hl,#0x0000
        ld      de,#0x0000
        ret

__tf_ret_one:
        ld      hl,#0x3f80
        ld      de,#0x0000
        ret

__tf_neg_hlde:
        ld      a,h
        xor     #0x80
        ld      h,a
        ret

        ;; __libc_expf_core
        ;; input:  HL:DE = x
        ;; output: HL:DE = expf(x)
__libc_expf_core::
        ld      (__tf_x),de
        ld      (__tf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; FP_NAN
        jp      z,__tf_ret_nan
        cp      #1                      ; FP_INFINITE
        jp      z,tf_exp_inf
        cp      #2                      ; FP_ZERO
        jp      z,__tf_ret_one

        ;; Guard the integer scaling path with coarse finite thresholds.
        ld      hl,#0x42b0              ; +88.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_x
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jr      z,tf_exp_chk_lo
        ld      a,d
        or      e
        jp      nz,__tf_ret_pinf        ; x > 88 -> overflow
tf_exp_chk_lo:
        ld      hl,#0xc2d0              ; -104.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_x
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jp      z,__tf_ret_zero         ; x < -104 -> underflow
        ld      a,d
        or      e
        jp      z,__tf_ret_zero         ; x == -104 -> underflow

        ;; qf = roundf(x / ln(2))
        ld      hl,#0x3fb8              ; 1 / ln(2)
        push    hl
        ld      hl,#0xaa3b
        push    hl
        call    __tf_load_x
        call    ___fsmul
        pop     bc
        pop     bc
        call    _roundf
        ld      (__tf_qf),de
        ld      (__tf_qf + 2),hl
        call    ___fs2slong
        ld      (__tf_qi),de
        ld      (__tf_qi + 2),hl

        ;; r = x - qf * ln(2)
        ld      hl,(__tf_qf + 2)
        push    hl
        ld      hl,(__tf_qf)
        push    hl
        ld      de,#0x7218              ; ln(2)
        ld      hl,#0x3f31
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_y),de
        ld      (__tf_y + 2),hl
        ld      hl,(__tf_y + 2)
        push    hl
        ld      hl,(__tf_y)
        push    hl
        call    __tf_load_x
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__tf_x),de             ; reuse X scratch for reduced r
        ld      (__tf_x + 2),hl

        ;; Horner form for:
        ;;   exp(r) ≈ 1 + r*(1 + r*(1/2 + r*(1/6 + r*(1/24 + r*(1/120)))))
        ld      de,#0x8889              ; 1/120
        ld      hl,#0x3c08
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ld      hl,(__tf_x + 2)
        push    hl
        ld      hl,(__tf_x)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,#0x3d2a              ; +1/24
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ld      hl,(__tf_x + 2)
        push    hl
        ld      hl,(__tf_x)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,#0x3e2a              ; +1/6
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ld      hl,(__tf_x + 2)
        push    hl
        ld      hl,(__tf_x)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,#0x3f00              ; +1/2
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ld      hl,(__tf_x + 2)
        push    hl
        ld      hl,(__tf_x)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,#0x3f80              ; +1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ld      hl,(__tf_x + 2)
        push    hl
        ld      hl,(__tf_x)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,#0x3f80              ; +1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc

        ;; scale the polynomial by 2^q
        ld      de,(__tf_qi)            ; low 16 bits are enough after guards
        push    de
        call    _ldexpf
        pop     bc
        ret

tf_exp_inf:
        call    __tf_load_x
        call    ___libc_signbitf
        ld      a,d
        or      e
        jp      nz,__tf_ret_zero
        jp      __tf_ret_pinf

        ;; __libc_logf_core
        ;; input:  HL:DE = x
        ;; output: HL:DE = logf(x)
__libc_logf_core::
        ld      (__tf_x),de
        ld      (__tf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; FP_NAN
        jp      z,__tf_ret_nan
        cp      #1                      ; FP_INFINITE
        jp      z,__tf_ret_pinf
        cp      #2                      ; FP_ZERO
        jp      z,__tf_ret_ninf

        ;; Negative finite inputs are domain errors.
        call    __tf_load_x
        call    ___libc_signbitf
        ld      a,d
        or      e
        jp      nz,__tf_ret_nan

        ;; m = frexpf(x, &e)
        ld      bc,#__tf_exp
        push    bc
        call    __tf_load_x
        call    _frexpf
        pop     bc
        ld      (__tf_x),de
        ld      (__tf_x + 2),hl

        ;; Ensure m lies in [sqrt(1/2), sqrt(2)) for the atanh series.
        ld      hl,#0x3f35              ; sqrt(1/2)
        push    hl
        ld      hl,#0x04f3
        push    hl
        call    __tf_load_x
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jr      nz,tf_log_ratio
        ld      hl,#0x4000              ; * 2
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_x
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_x),de
        ld      (__tf_x + 2),hl
        ld      hl,(__tf_exp)
        dec     hl
        ld      (__tf_exp),hl

tf_log_ratio:
        ;; s = (m - 1) / (m + 1)
        ld      hl,#0x3f80              ; - 1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_x
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl         ; numerator

        ld      hl,#0x3f80              ; + 1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_x
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tf_y),de
        ld      (__tf_y + 2),hl         ; denominator

        ld      hl,(__tf_y + 2)
        push    hl
        ld      hl,(__tf_y)
        push    hl
        call    __tf_load_u
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__tf_x),de
        ld      (__tf_x + 2),hl         ; s

        ;; z = s*s
        ld      hl,(__tf_x + 2)
        push    hl
        ld      hl,(__tf_x)
        push    hl
        call    __tf_load_x
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_y),de
        ld      (__tf_y + 2),hl

        ;; Horner form for:
        ;;   1 + z/3 + z^2/5 + z^3/7
        ld      de,#0x4925              ; 1/7
        ld      hl,#0x3e12
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ld      hl,(__tf_y + 2)
        push    hl
        ld      hl,(__tf_y)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,#0x3e4c              ; +1/5
        push    hl
        ld      hl,#0xcccd
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ld      hl,(__tf_y + 2)
        push    hl
        ld      hl,(__tf_y)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,#0x3eaa              ; +1/3
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ld      hl,(__tf_y + 2)
        push    hl
        ld      hl,(__tf_y)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,#0x3f80              ; +1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc

        ;; log(m) = 2 * s * poly
        ld      (__tf_v),de
        ld      (__tf_v + 2),hl
        ld      hl,(__tf_x + 2)
        push    hl
        ld      hl,(__tf_x)
        push    hl
        call    __tf_load_v
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl
        ld      hl,(__tf_u + 2)
        push    hl
        ld      hl,(__tf_u)
        push    hl
        call    __tf_load_u
        call    ___fsadd                ; 2 * s * poly
        pop     bc
        pop     bc
        ld      (__tf_u),de
        ld      (__tf_u + 2),hl

        ;; e*ln(2)
        ld      hl,(__tf_exp)
        call    ___sint2fs
        ld      (__tf_v),de
        ld      (__tf_v + 2),hl
        ld      hl,#0x3f31              ; ln(2)
        push    hl
        ld      hl,#0x7218
        push    hl
        call    __tf_load_v
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__tf_v),de
        ld      (__tf_v + 2),hl

        ;; log(x) = e*ln(2) + log(m)
        ld      hl,(__tf_u + 2)
        push    hl
        ld      hl,(__tf_u)
        push    hl
        call    __tf_load_v
        call    ___fsadd
        pop     bc
        pop     bc
        ret

__tf_load_v:
        ld      de,(__tf_v)
        ld      hl,(__tf_v + 2)
        ret
