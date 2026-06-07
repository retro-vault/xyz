        ; isascii.s
        ;
        ; libc isascii implementation for the xcc Z80 libc.
        ; Returns non-zero when the value is a 7-bit ASCII character
        ; (0..127) (POSIX extension).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isascii
        .optsdcc -mz80 sdcccall(1)


        .globl  _isascii

        .area   _CODE

        ; _isascii
        ; inputs:  HL = promoted int character value
        ; outputs: DE = 1 when 0 <= value <= 127, else 0
        ; clobbers: AF
_isascii::
        ; ASCII iff the high byte is zero and bit7 of the low byte is clear.
        ld      a,h
        or      a
        jr      nz,isascii_false
        bit     7,l
        jr      nz,isascii_false
        ld      de,#0x0001
        ret
isascii_false:
        ld      de,#0x0000
        ret
