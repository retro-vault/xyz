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
        ;; Keep the helper structure, but move every temporary into one
        ;; IX-framed local block so concurrent calls do not alias each other.
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

TF_XLO  .equ -26
TF_XHI  .equ -24
TF_YLO  .equ -22
TF_YHI  .equ -20
TF_ULO  .equ -18
TF_UHI  .equ -16
TF_VLO  .equ -14
TF_VHI  .equ -12
TF_QFLO .equ -10
TF_QFHI .equ -8
TF_QILO .equ -6
TF_QIHI .equ -4
TF_EXP  .equ -2

        .area   _CODE

__tf_load_x:
        ld      e,TF_XLO(ix)
        ld      d,TF_XLO + 1(ix)
        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        ret

__tf_load_y:
        ld      e,TF_YLO(ix)
        ld      d,TF_YLO + 1(ix)
        ld      l,TF_YHI(ix)
        ld      h,TF_YHI + 1(ix)
        ret

__tf_load_u:
        ld      e,TF_ULO(ix)
        ld      d,TF_ULO + 1(ix)
        ld      l,TF_UHI(ix)
        ld      h,TF_UHI + 1(ix)
        ret

__tf_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        jp      __tf_leave

__tf_ret_pinf:
        ld      hl,#0x7f80
        ld      de,#0x0000
        jp      __tf_leave

__tf_ret_ninf:
        ld      hl,#0xff80
        ld      de,#0x0000
        jp      __tf_leave

__tf_ret_zero:
        ld      hl,#0x0000
        ld      de,#0x0000
        jp      __tf_leave

__tf_ret_one:
        ld      hl,#0x3f80
        ld      de,#0x0000
        jp      __tf_leave

__tf_neg_hlde:
        ld      a,h
        xor     #0x80
        ld      h,a
        ret

__tf_leave:
        ld      sp,ix
        pop     ix
        ret

        ;; __libc_expf_core
        ;; input:  HL:DE = x
        ;; output: HL:DE = expf(x)
__libc_expf_core::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l                     ; preserve x.high while allocating locals
        ld      b,h
        ld      hl,#-26
        add     hl,sp
        ld      sp,hl
        ld      TF_XLO(ix),e
        ld      TF_XLO + 1(ix),d
        ld      TF_XHI(ix),c
        ld      TF_XHI + 1(ix),b
        call    __tf_load_x
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
        ld      TF_QFLO(ix),e
        ld      TF_QFLO + 1(ix),d
        ld      TF_QFHI(ix),l
        ld      TF_QFHI + 1(ix),h
        call    ___fs2slong
        ld      TF_QILO(ix),e
        ld      TF_QILO + 1(ix),d
        ld      TF_QIHI(ix),l
        ld      TF_QIHI + 1(ix),h

        ;; r = x - qf * ln(2)
        ld      l,TF_QFHI(ix)
        ld      h,TF_QFHI + 1(ix)
        push    hl
        ld      l,TF_QFLO(ix)
        ld      h,TF_QFLO + 1(ix)
        push    hl
        ld      de,#0x7218              ; ln(2)
        ld      hl,#0x3f31
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_YLO(ix),e
        ld      TF_YLO + 1(ix),d
        ld      TF_YHI(ix),l
        ld      TF_YHI + 1(ix),h
        ld      l,TF_YHI(ix)
        ld      h,TF_YHI + 1(ix)
        push    hl
        ld      l,TF_YLO(ix)
        ld      h,TF_YLO + 1(ix)
        push    hl
        call    __tf_load_x
        call    ___fssub
        pop     bc
        pop     bc
        ld      TF_XLO(ix),e            ; reuse X scratch for reduced r
        ld      TF_XLO + 1(ix),d
        ld      TF_XHI(ix),l
        ld      TF_XHI + 1(ix),h

        ;; Horner form for:
        ;;   exp(r) ≈ 1 + r*(1 + r*(1/2 + r*(1/6 + r*(1/24 + r*(1/120)))))
        ld      de,#0x8889              ; 1/120
        ld      hl,#0x3c08
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        push    hl
        ld      l,TF_XLO(ix)
        ld      h,TF_XLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      hl,#0x3d2a              ; +1/24
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        push    hl
        ld      l,TF_XLO(ix)
        ld      h,TF_XLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      hl,#0x3e2a              ; +1/6
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        push    hl
        ld      l,TF_XLO(ix)
        ld      h,TF_XLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      hl,#0x3f00              ; +1/2
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        push    hl
        ld      l,TF_XLO(ix)
        ld      h,TF_XLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      hl,#0x3f80              ; +1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        push    hl
        ld      l,TF_XLO(ix)
        ld      h,TF_XLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      hl,#0x3f80              ; +1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc

        ;; scale the polynomial by 2^q
        ld      e,TF_QILO(ix)           ; low 16 bits are enough after guards
        ld      d,TF_QILO + 1(ix)
        push    de
        call    _ldexpf
        pop     bc
        jp      __tf_leave

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
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-26
        add     hl,sp
        ld      sp,hl
        ld      TF_XLO(ix),e
        ld      TF_XLO + 1(ix),d
        ld      TF_XHI(ix),c
        ld      TF_XHI + 1(ix),b
        call    __tf_load_x
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
        push    ix
        pop     hl
        ld      de,#TF_EXP
        add     hl,de
        push    hl
        pop     bc
        push    bc
        call    __tf_load_x
        call    _frexpf
        pop     bc
        ld      TF_XLO(ix),e
        ld      TF_XLO + 1(ix),d
        ld      TF_XHI(ix),l
        ld      TF_XHI + 1(ix),h

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
        ld      TF_XLO(ix),e
        ld      TF_XLO + 1(ix),d
        ld      TF_XHI(ix),l
        ld      TF_XHI + 1(ix),h
        ld      l,TF_EXP(ix)
        ld      h,TF_EXP + 1(ix)
        dec     hl
        ld      TF_EXP(ix),l
        ld      TF_EXP + 1(ix),h

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
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h        ; numerator

        ld      hl,#0x3f80              ; + 1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_x
        call    ___fsadd
        pop     bc
        pop     bc
        ld      TF_YLO(ix),e
        ld      TF_YLO + 1(ix),d
        ld      TF_YHI(ix),l
        ld      TF_YHI + 1(ix),h        ; denominator

        ld      l,TF_YHI(ix)
        ld      h,TF_YHI + 1(ix)
        push    hl
        ld      l,TF_YLO(ix)
        ld      h,TF_YLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      TF_XLO(ix),e
        ld      TF_XLO + 1(ix),d
        ld      TF_XHI(ix),l
        ld      TF_XHI + 1(ix),h        ; s

        ;; z = s*s
        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        push    hl
        ld      l,TF_XLO(ix)
        ld      h,TF_XLO + 1(ix)
        push    hl
        call    __tf_load_x
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_YLO(ix),e
        ld      TF_YLO + 1(ix),d
        ld      TF_YHI(ix),l
        ld      TF_YHI + 1(ix),h

        ;; Horner form for:
        ;;   1 + z/3 + z^2/5 + z^3/7
        ld      de,#0x4925              ; 1/7
        ld      hl,#0x3e12
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ld      l,TF_YHI(ix)
        ld      h,TF_YHI + 1(ix)
        push    hl
        ld      l,TF_YLO(ix)
        ld      h,TF_YLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      hl,#0x3e4c              ; +1/5
        push    hl
        ld      hl,#0xcccd
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ld      l,TF_YHI(ix)
        ld      h,TF_YHI + 1(ix)
        push    hl
        ld      l,TF_YLO(ix)
        ld      h,TF_YLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      hl,#0x3eaa              ; +1/3
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ld      l,TF_YHI(ix)
        ld      h,TF_YHI + 1(ix)
        push    hl
        ld      l,TF_YLO(ix)
        ld      h,TF_YLO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      hl,#0x3f80              ; +1
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __tf_load_u
        call    ___fsadd
        pop     bc
        pop     bc

        ;; log(m) = 2 * s * poly
        ld      TF_VLO(ix),e
        ld      TF_VLO + 1(ix),d
        ld      TF_VHI(ix),l
        ld      TF_VHI + 1(ix),h
        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        push    hl
        ld      l,TF_XLO(ix)
        ld      h,TF_XLO + 1(ix)
        push    hl
        call    __tf_load_v
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h
        ld      l,TF_UHI(ix)
        ld      h,TF_UHI + 1(ix)
        push    hl
        ld      l,TF_ULO(ix)
        ld      h,TF_ULO + 1(ix)
        push    hl
        call    __tf_load_u
        call    ___fsadd                ; 2 * s * poly
        pop     bc
        pop     bc
        ld      TF_ULO(ix),e
        ld      TF_ULO + 1(ix),d
        ld      TF_UHI(ix),l
        ld      TF_UHI + 1(ix),h

        ;; e*ln(2)
        ld      l,TF_EXP(ix)
        ld      h,TF_EXP + 1(ix)
        call    ___sint2fs
        ld      TF_VLO(ix),e
        ld      TF_VLO + 1(ix),d
        ld      TF_VHI(ix),l
        ld      TF_VHI + 1(ix),h
        ld      hl,#0x3f31              ; ln(2)
        push    hl
        ld      hl,#0x7218
        push    hl
        call    __tf_load_v
        call    ___fsmul
        pop     bc
        pop     bc
        ld      TF_VLO(ix),e
        ld      TF_VLO + 1(ix),d
        ld      TF_VHI(ix),l
        ld      TF_VHI + 1(ix),h

        ;; log(x) = e*ln(2) + log(m)
        ld      l,TF_UHI(ix)
        ld      h,TF_UHI + 1(ix)
        push    hl
        ld      l,TF_ULO(ix)
        ld      h,TF_ULO + 1(ix)
        push    hl
        call    __tf_load_v
        call    ___fsadd
        pop     bc
        pop     bc
        jp      __tf_leave

__tf_load_v:
        ld      e,TF_VLO(ix)
        ld      d,TF_VLO + 1(ix)
        ld      l,TF_VHI(ix)
        ld      h,TF_VHI + 1(ix)
        ret
