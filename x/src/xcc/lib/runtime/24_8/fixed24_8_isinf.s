        ; fixed24_8_isinf.s
        ;
        ; 24.8 fixed reserves the extreme encodings for +/-infinity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_isinf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_isinf

        .area   _CODE

_fixed24_8_isinf::
        ld      a,h
        cp      #0x7f
        jr      z,.positive
        cp      #0x80
        jr      z,.negative
        ld      de,#0
        ret

.positive:
        ld      a,l
        cp      #0xff
        jr      nz,.false
        ld      a,d
        cp      #0xff
        jr      nz,.false
        ld      a,e
        cp      #0xff
        jr      z,.true
        jr      .false

.negative:
        ld      a,l
        or      d
        or      e
        jr      z,.true

.false:
        ld      de,#0
        ret

.true:
        ld      de,#1
        ret
