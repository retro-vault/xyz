        ;; scalblnf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module scalblnf
        .optsdcc -mz80 sdcccall(1)

        .globl  _scalblnf
        .globl  _ldexpf

MF_XHI  .equ -14
MF_XLO  .equ -16

        .area   _CODE
_scalblnf::
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
        ;; accept only values that already fit in signed 16-bit, otherwise clamp
        ld      a,7(ix)
        cp      #0xff
        jr      nz,scalblnf_hi_not_ff
        ld      a,6(ix)
        cp      #0xff
        jr      nz,scalblnf_hi_not_ff
        bit     7,5(ix)
        jr      z,scalblnf_clamp_neg
        ld      e,4(ix)
        ld      d,5(ix)
        jr      scalblnf_call
scalblnf_hi_not_ff:
        ld      a,7(ix)
        or      6(ix)
        jr      nz,scalblnf_hi_nonzero
        bit     7,5(ix)
        jr      nz,scalblnf_clamp_pos
        ld      e,4(ix)
        ld      d,5(ix)
        jr      scalblnf_call
scalblnf_hi_nonzero:
        bit     7,7(ix)
        jr      nz,scalblnf_clamp_neg
scalblnf_clamp_pos:
        ld      de,#0x7fff
        jr      scalblnf_call
scalblnf_clamp_neg:
        ld      de,#0x8000
scalblnf_call:
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    _ldexpf
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; float fmaf(float x, float y, float z)
        ;; x in HL:DE, y at 4(ix)..7(ix), z at 8(ix)..11(ix)
