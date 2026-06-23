        ;; remainderf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module remainderf
        .optsdcc -mz80 sdcccall(1)

        .globl  _remainderf
        .globl  ___fsdiv
        .globl  ___fsmul
        .globl  ___fssub
        .globl  _roundf

MF_QHI  .equ -6
MF_QLO  .equ -8
MF_THI  .equ -2
MF_TLO  .equ -4
MF_XHI  .equ -14
MF_XLO  .equ -16
MF_YHI  .equ -10
MF_YLO  .equ -12

        .area   _CODE
_remainderf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b
        ld      a,4(ix)
        ld      MF_YLO(ix),a
        ld      a,5(ix)
        ld      MF_YLO+1(ix),a
        ld      a,6(ix)
        ld      MF_YHI(ix),a
        ld      a,7(ix)
        ld      MF_YHI+1(ix),a
        ld      a,MF_YHI+1(ix)
        and     #0x7f
        ld      b,a
        ld      a,MF_YHI(ix)
        or      b
        ld      b,a
        ld      a,MF_YLO+1(ix)
        or      b
        ld      b,a
        ld      a,MF_YLO(ix)
        or      b
        jr      nz,remainderf_div
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret
remainderf_div:
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fsdiv
        pop     bc
        pop     bc
        call    _roundf
        ld      MF_QLO(ix),e
        ld      MF_QLO+1(ix),d
        ld      MF_QHI(ix),l
        ld      MF_QHI+1(ix),h
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_QLO(ix)
        ld      d,MF_QLO+1(ix)
        ld      l,MF_QHI(ix)
        ld      h,MF_QHI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      MF_TLO(ix),e
        ld      MF_TLO+1(ix),d
        ld      MF_THI(ix),l
        ld      MF_THI+1(ix),h
        ld      l,MF_THI(ix)
        ld      h,MF_THI+1(ix)
        push    hl
        ld      e,MF_TLO(ix)
        ld      d,MF_TLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; float remquof(float x, float y, int *quo)
