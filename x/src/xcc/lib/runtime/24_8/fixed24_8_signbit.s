        ; fixed24_8_signbit.s
        ;
        ; C signbit helper for signed 24.8 fixed float mode.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_signbit
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_signbit

        .area   _CODE

        ; inputs:  DE:HL = fixed24_8
        ; outputs: DE = 1 if x < 0 else 0
_fixed24_8_signbit::
        ld      de,#0
        bit     7,h
        ret     z
        inc     de
        ret
