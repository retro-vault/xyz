        ; ieee16_nextdown.s
        .module ieee16_nextdown
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_nextdown
        .globl  ___ieee16_classify_hl

        .area   _CODE
_ieee16_nextdown::
        call    ___ieee16_classify_hl
        ld      a,e
        cp      #0
        jr      z,.ret_x
        cp      #1
        jr      z,.ret_x
        cp      #2
        jr      z,.zero
        bit     7,h
        jr      nz,.inc_mag
        dec     l
        jr      nz,.ret_x
        dec     h
        jr      .ret_x
.inc_mag:
        inc     l
        jr      nz,.ret_x
        inc     h
        jr      .ret_x
.zero:
        ld      de,#0x8001
        ret
.ret_x:
        ld      d,h
        ld      e,l
        ret
