        ; fixed8_8_to_int.s
        ;
        ; Convert 8.8 fixed to signed int by truncating toward -infinity
        ; at the raw binary-point boundary, i.e. arithmetic shift right 8.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_to_int
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_to_int

        .area   _CODE

        ; inputs:  HL = fixed8_8
        ; outputs: DE = HL >> 8, sign-extended
_fixed8_8_to_int::
        ld      e,h
        ld      d,#0
        bit     7,e
        ret     z
        dec     d
        ret
