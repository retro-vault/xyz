        ; bcopy.s
        ;
        ; libc bcopy implementation for the xcc Z80 libc.
        ; Legacy BSD copy with (src, dest, n) argument order; defers to
        ; memmove for correct overlap handling.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module bcopy
        .optsdcc -mz80 sdcccall(1)


        .globl  _bcopy
        .globl  _memmove

        .area   _CODE

        ; _bcopy
        ; inputs:  HL = source, DE = destination, 4(ix)..5(ix) = count
        ; outputs: none
        ; clobbers: AF, BC, DE, HL, IX
_bcopy::
        ex      de,hl                   ; memmove wants HL=dest, DE=source
        jp      _memmove
