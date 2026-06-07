        ; toascii.s
        ;
        ; libc toascii implementation for the xcc Z80 libc.
        ; Masks a value down to 7-bit ASCII (POSIX extension).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module toascii
        .optsdcc -mz80 sdcccall(1)


        .globl  _toascii

        .area   _CODE

        ; _toascii
        ; inputs:  HL = promoted int character value
        ; outputs: DE = value & 0x7F
        ; clobbers: AF
_toascii::
        ld      a,l
        and     #0x7f
        ld      e,a
        ld      d,#0x00
        ret
