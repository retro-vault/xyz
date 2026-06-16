        ;; nextdownf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module nextdownf
        .optsdcc -mz80 sdcccall(1)

        .globl  _nextdownf

MF_XHI  .equ -14
MF_XLO  .equ -16

        .area   _CODE
_nextdownf::
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

        ld      a,MF_XHI+1(ix)
        and     #0x7f
        ld      c,a
        ld      a,MF_XHI(ix)
        or      c
        ld      c,a
        ld      a,MF_XLO+1(ix)
        or      c
        ld      c,a
        ld      a,MF_XLO(ix)
        or      c
        jr      z,nextdownf_zero

        ld      a,MF_XHI+1(ix)
        and     #0x7f
        cp      #0x7f
        jr      z,nextdownf_ret_x

        ld      a,MF_XHI+1(ix)
        bit     7,a
        jr      nz,nextdownf_inc   ; negative x: increment bits to decrease value (more negative)

        ; positive: decrement bits
        ld      a,MF_XLO(ix)
        sub     #1
        ld      MF_XLO(ix),a
        ld      a,MF_XLO+1(ix)
        sbc     a,#0
        ld      MF_XLO+1(ix),a
        ld      a,MF_XHI(ix)
        sbc     a,#0
        ld      MF_XHI(ix),a
        ld      a,MF_XHI+1(ix)
        sbc     a,#0
        ld      MF_XHI+1(ix),a
        jr      nextdownf_ret_x

nextdownf_inc:
        ld      a,MF_XLO(ix)
        add     a,#1
        ld      MF_XLO(ix),a
        ld      a,MF_XLO+1(ix)
        adc     a,#0
        ld      MF_XLO+1(ix),a
        ld      a,MF_XHI(ix)
        adc     a,#0
        ld      MF_XHI(ix),a
        ld      a,MF_XHI+1(ix)
        adc     a,#0
        ld      MF_XHI+1(ix),a
        jr      nextdownf_ret_x

nextdownf_zero:
        ; nextdown(±0) = smallest negative subnormal
        ld      e,#1
        ld      d,#0
        ld      l,#0
        ld      h,#0x80
        ld      sp,ix
        pop     ix
        ret

nextdownf_ret_x:
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        ld      sp,ix
        pop     ix
        ret
