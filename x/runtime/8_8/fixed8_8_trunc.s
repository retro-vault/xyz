        ; fixed8_8_trunc.s
        ;
        ; Round signed 8.8 toward zero.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_trunc
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_trunc
        .globl  _fixed8_8_to_int
        .globl  _fixed8_8_from_int

        .area   _CODE

        ; inputs:  HL = fixed8_8
        ; outputs: DE = trunc(x)
_fixed8_8_trunc::
        call    _fixed8_8_to_int
        push    de
        pop     hl
        jp      _fixed8_8_from_int
