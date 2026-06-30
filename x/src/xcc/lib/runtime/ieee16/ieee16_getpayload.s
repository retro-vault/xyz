        ; ieee16_getpayload.s
        .module ieee16_getpayload
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_getpayload
        .globl  ___ieee16_load_half_ptr_hl

        .area   _CODE
_ieee16_getpayload::
        call    ___ieee16_load_half_ptr_hl
        ld      a,h
        and     #0x7c
        cp      #0x7c
        jr      nz,.zero
        ld      a,h
        and     #0x03
        ld      d,a
        ld      e,l
        ld      a,d
        or      e
        jr      z,.zero
        ld      a,d
        or      #0x3c
        ld      d,a
        ret
.zero:
        ld      de,#0
        ret
