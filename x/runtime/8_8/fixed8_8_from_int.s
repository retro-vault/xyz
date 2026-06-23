        ; fixed8_8_from_int.s
        ;
        ; Convert signed int to 8.8 fixed. Overflow wraps.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_from_int
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_from_int

        .area   _CODE

        ; inputs:  HL = signed int
        ; outputs: DE = (fixed8_8)(HL << 8)
_fixed8_8_from_int::
        ld      d,l
        ld      e,#0
        ret
