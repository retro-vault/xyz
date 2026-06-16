        ;; fmaf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fmaf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fmaf
        .globl  ___fsadd
        .globl  ___fsmul

MF_THI  .equ -2
MF_TLO  .equ -4
MF_XHI  .equ -14
MF_XLO  .equ -16
MF_YHI  .equ -10
MF_YLO  .equ -12

        .area   _CODE
_fmaf::
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
        call    ___fsmul
        pop     bc
        pop     bc
        ld      MF_TLO(ix),e
        ld      MF_TLO+1(ix),d
        ld      MF_THI(ix),l
        ld      MF_THI+1(ix),h
        ld      a,8(ix)
        ld      MF_YLO(ix),a
        ld      a,9(ix)
        ld      MF_YLO+1(ix),a
        ld      a,10(ix)
        ld      MF_YHI(ix),a
        ld      a,11(ix)
        ld      MF_YHI+1(ix),a
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_TLO(ix)
        ld      d,MF_TLO+1(ix)
        ld      l,MF_THI(ix)
        ld      h,MF_THI+1(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; float hypotf(float x, float y)
