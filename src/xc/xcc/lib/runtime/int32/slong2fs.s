        ; signed long to float via ulong2fs (ieee-754 single) for sdcc
        ; z80
        ; converts 32-bit signed int to single precision by:
        ;  - producing unsigned magnitude
        ;  - calling ___ulong2fs
        ;  - setting sign bit in the result if input was negative
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module slong2fs
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___slong2fs
        .globl  ___ulong2fs

        ; ___slong2fs
        ; inputs:  hl:de = a (signed long), hl=high word, de=low word
        ; outputs: hl:de = (float)a
        ; clobbers: af, bc, de, hl
___slong2fs:
        ; zero?
        ld      a,h
        or      l
        or      d
        or      e
        jr      nz, .nonzero
        xor     a
        ld      h,a
        ld      l,a
        ld      d,a
        ld      e,a
        ret

.nonzero:
        ; sign flag in B (0x80 if negative else 0)
        ld      b,#0x00
        bit     7,h
        jr      z, .mag_ok
        ld      b,#0x80

        ; magnitude = - (HL:DE)
        ; two's complement negate 32-bit: invert then +1
        ld      a,e
        cpl
        ld      e,a
        ld      a,d
        cpl
        ld      d,a
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a

        inc     de
        jr      nz, .mag_ok
        inc     hl

.mag_ok:
        ; convert magnitude (___ulong2fs clobbers BC, so save sign
        ; first)
        push    bc
        call    ___ulong2fs
        pop     bc

        ; apply sign: set sign bit in top byte of float (H bit7)
        bit     7,b
        ret     z
        set     7,h
        ret
