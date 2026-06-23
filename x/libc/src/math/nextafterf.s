        ;; nextafterf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module nextafterf
        .optsdcc -mz80 sdcccall(1)

        .globl  _nextafterf
        .globl  __float_cmp_xy

MF_XHI  .equ -14
MF_XLO  .equ -16

        .area   _CODE
_nextafterf::
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
        ld      l,c
        ld      h,b
        call    __float_cmp_xy
        ld      b,a
        or      a
        jp      z,nextafterf_ret_y
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
        jr      nz,nextafterf_nonzero
        ld      a,7(ix)
        and     #0x80
        ld      h,a
        ld      l,#0x00
        ld      d,#0x00
        ld      e,#0x01
        ld      sp,ix
        pop     ix
        ret
nextafterf_nonzero:
        ld      a,MF_XHI+1(ix)
        bit     7,a
        jr      nz,nextafterf_neg
        ld      a,b
        cp      #0xff
        jr      z,nextafterf_inc
        jr      nextafterf_dec
nextafterf_neg:
        ld      a,b
        cp      #0xff
        jr      z,nextafterf_dec
nextafterf_inc:
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
        jr      nextafterf_ret_x
nextafterf_dec:
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
nextafterf_ret_x:
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        ld      sp,ix
        pop     ix
        ret
nextafterf_ret_y:
        ld      a,4(ix)
        ld      e,a
        ld      a,5(ix)
        ld      d,a
        ld      a,6(ix)
        ld      l,a
        ld      a,7(ix)
        ld      h,a
        ld      sp,ix
        pop     ix
        ret

        ;; ----------------------------------------------------------------
        ;; C23 nextup / nextdown (new functions, implemented here in the
        ;; existing moremathf.s only — no new source files).
        ;; nextupf(x) = smallest float > x (toward +∞)
        ;; nextdownf(x) = largest float < x (toward -∞)
        ;; Uses stack frame only (no static data, thread-safe).
        ;; Special cases for ±0, ±inf, NaN follow IEEE.
        ;; ----------------------------------------------------------------

        ;; float nextupf(float x)
