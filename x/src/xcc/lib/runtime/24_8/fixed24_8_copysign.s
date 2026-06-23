        ; fixed24_8_copysign.s
        ;
        ; Copy sign for signed 24.8 fixed values.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_copysign
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_copysign
        .globl  _fixed24_8_abs
        .globl  _fixed24_8_neg

        .area   _CODE

        ; inputs:  DE:HL = magnitude, stack = sign source
        ; outputs: DE:HL = copysign(magnitude, sign)
_fixed24_8_copysign::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    _fixed24_8_abs
        bit     7,7(ix)
        jr      z,.done
        call    _fixed24_8_neg
.done:
        pop     ix
        ret
