        ; strcmp.s
        ;
        ; libc strcmp implementation for the xcc Z80 libc.
        ; Compares the strings byte-by-byte and uses the shared tri-state helper
        ; to map compare flags into the standard negative/zero/positive result.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strcmp
        .optsdcc -mz80 sdcccall(1)


        .globl  _strcmp
        .globl  __string_compare_result

        .area   _CODE

        ; _strcmp
        ; inputs:
        ;   HL = left-hand string
        ;   DE = right-hand string
        ; outputs:
        ;   DE = negative / zero / positive comparison result
        ; clobbers: AF, HL
_strcmp::
        ex      de,hl                   ; DE = lhs, HL = rhs
strcmp_loop:
        ld      a,(de)
        cp      (hl)
        jp      nz,__string_compare_result
        or      a
        jr      z,strcmp_equal
        inc     de
        inc     hl
        jr      strcmp_loop
strcmp_equal:
        ld      de,#0x0000
        ret
