        ; imaxabs.s
        ;
        ; libc imaxabs implementation for the xcc Z80 libc.
        ; intmax_t is long long on this target, so imaxabs is identical to
        ; llabs and simply tail-calls it.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module imaxabs
        .optsdcc -mz80 sdcccall(1)


        .globl  _imaxabs
        .globl  _llabs

        .area   _CODE

        ; _imaxabs
        ; inputs:  DE:HL:DE':HL' = signed intmax_t (long long)
        ; outputs: DE:HL:DE':HL' = |value|
        ; clobbers: AF
_imaxabs::
        jp      _llabs
