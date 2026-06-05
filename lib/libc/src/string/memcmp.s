        ; memcmp.s
        ;
        ; libc memcmp implementation for the xcc Z80 libc.
        ; Performs a bounded byte comparison over raw memory rather than
        ; stopping on NUL.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memcmp
        .optsdcc -mz80 sdcccall(1)


        .globl  _memcmp
        .globl  __string_compare_result

        .area   _CODE

        ; _memcmp
        ; inputs:
        ;   HL         = left-hand byte span
        ;   DE         = right-hand byte span
        ;   4(ix)..5(ix) = byte count
        ; outputs:
        ;   DE = negative / zero / positive comparison result
        ; clobbers: AF, BC, HL, IX
_memcmp::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        ex      de,hl                   ; DE = lhs, HL = rhs
memcmp_loop:
        ld      a,b
        or      c
        jr      z,memcmp_equal
        ld      a,(de)
        cp      (hl)
        jr      z,memcmp_same
        call    __string_compare_result
        pop     ix
        ret
memcmp_same:
        inc     de
        inc     hl
        dec     bc
        jr      memcmp_loop
memcmp_equal:
        ld      de,#0x0000
        pop     ix
        ret
