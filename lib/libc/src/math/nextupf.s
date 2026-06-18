        ;; nextupf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module nextupf
        .optsdcc -mz80 sdcccall(1)

        .globl  _nextupf
        .globl  nextupf_ret_x

MF_XHI  .equ -14
MF_XLO  .equ -16

        .area   _CODE
_nextupf::
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

        ; zero check (all bytes 0)
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
        jr      z,nextupf_zero

        ; exp all ones -> inf or nan: return x as-is (propagation)
        ld      a,MF_XHI+1(ix)
        and     #0x7f
        cp      #0x7f
        jp      z,nextupf_ret_x

        ld      a,MF_XHI+1(ix)
        bit     7,a
        jr      nz,nextupf_dec   ; negative x: decrement bits to increase value

        ; positive: increment bits
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
        jp      nextupf_ret_x

nextupf_dec:
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
        jp      nextupf_ret_x

nextupf_zero:
        ; nextup(±0) = smallest positive subnormal
        ld      e,#1
        ld      d,#0
        ld      l,#0
        ld      h,#0
        ld      sp,ix
        pop     ix
        ret
