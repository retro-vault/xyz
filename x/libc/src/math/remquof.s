        ;; remquof.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module remquof
        .optsdcc -mz80 sdcccall(1)

        .globl  _remquof
        .globl  ___fs2slong
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
_remquof::
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
        jr      nz,remquof_div
        ld      c,8(ix)
        ld      b,9(ix)
        ld      a,b
        or      c
        jr      z,remquof_nan
        xor     a
        ld      (bc),a
        inc     bc
        ld      (bc),a
remquof_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret
remquof_div:
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
        ld      c,8(ix)
        ld      b,9(ix)
        ld      a,b
        or      c
        jr      z,remquof_store_done
        ld      e,MF_QLO(ix)
        ld      d,MF_QLO+1(ix)
        ld      l,MF_QHI(ix)
        ld      h,MF_QHI+1(ix)
        call    ___fs2slong
        ld      c,8(ix)
        ld      b,9(ix)
        ld      a,e
        and     #0x7f
        ld      e,a
        ld      d,#0
        bit     7,h
        jr      z,remquof_store
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
remquof_store:
        ld      a,e
        ld      (bc),a
        inc     bc
        ld      a,d
        ld      (bc),a
remquof_store_done:
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

        ;; float nextafterf(float x, float y)
