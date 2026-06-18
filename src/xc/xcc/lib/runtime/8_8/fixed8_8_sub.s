        ; fixed8_8_sub.s
        ;
        ; Signed 8.8 subtract. Two's-complement overflow wraps.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_sub
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_sub

        .area   _CODE

        ; inputs:  HL = a, DE = b
        ; outputs: DE = a - b
_fixed8_8_sub::
        or      a
        sbc     hl,de
        ex      de,hl
        ret
