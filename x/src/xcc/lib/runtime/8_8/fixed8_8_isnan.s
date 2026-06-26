        ; fixed8_8_isnan.s
        ;
        ; 8.8 fixed reserves 0x7ffe for NaN.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_isnan
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_isnan

        .area   _CODE

_fixed8_8_isnan::
        ld      a,h
        cp      #0x7f
        jr      nz,.false
        ld      a,l
        cp      #0xfe
        jr      z,.true

.false:
        ld      de,#0
        ret

.true:
        ld      de,#1
        ret
