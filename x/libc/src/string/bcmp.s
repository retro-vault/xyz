        ; bcmp.s
        ;
        ; libc bcmp implementation for the xcc Z80 libc.
        ; Legacy BSD byte compare; returns zero iff the spans are equal.
        ; memcmp's tri-state result already satisfies that contract.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module bcmp
        .optsdcc -mz80 sdcccall(1)


        .globl  _bcmp
        .globl  _memcmp

        .area   _CODE

        ; _bcmp
        ; inputs:  HL = first span, DE = second span, 4(ix)..5(ix) = count
        ; outputs: DE = 0 if equal, non-zero otherwise
        ; clobbers: AF, BC, DE, HL, IX
_bcmp::
        jp      _memcmp
