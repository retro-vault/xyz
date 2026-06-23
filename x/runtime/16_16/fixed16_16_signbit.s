        ; fixed16_16_signbit.s
        ;
        ; C signbit helper for signed 16.16 fixed float mode.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_signbit
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_signbit

        .area   _CODE

        ; inputs:  DE:HL = fixed16_16
        ; outputs: DE = 1 if x < 0 else 0
_fixed16_16_signbit::
        ld      de,#0
        bit     7,h
        ret     z
        inc     de
        ret
