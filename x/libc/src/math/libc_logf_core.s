        ;; libc_logf_core.s
        ;; Split from transf_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_logf_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_logf_core
        .globl  ___fsadd
        .globl  ___fscmp
        .globl  ___fsdiv
        .globl  ___fsmul
        .globl  ___fssub
        .globl  ___libc_fpclassifyf
        .globl  ___libc_signbitf
        .globl  ___sint2fs
        .globl  __tf_leave
        .globl  __tf_load_u
        .globl  __tf_load_x
        .globl  __tf_ret_nan
        .globl  __tf_ret_pinf
        .globl  _frexpf

TF_EXP  .equ -2
TF_UHI  .equ -16
TF_ULO  .equ -18
TF_VHI  .equ -12
TF_VLO  .equ -14
TF_XHI  .equ -24
TF_XLO  .equ -26
TF_YHI  .equ -20
TF_YLO  .equ -22

        .area   _CODE
__tf_ret_ninf:
        ld      hl,#0xff80
        ld      de,#0x0000
        jp      __tf_leave

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
