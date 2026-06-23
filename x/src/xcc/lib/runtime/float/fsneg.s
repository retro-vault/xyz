        ; Soft-float negation helper for the merged xcc runtime.
        ; Flips the sign bit of a 32-bit IEEE-754 value on the stack.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fsneg
        .area   _CODE
        .globl  __fsneg

        ; __fsneg
        ; inputs: 4(ix)..7(ix) = float argument, low word first.
        ; outputs: DE = high 16 bits, HL = low 16 bits of -x.
        ; clobbers: AF, IX.

__fsneg:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 4(ix)
        ld      h, 5(ix)
        push    hl
        ld      l, 6(ix)
        ld      h, 7(ix)
        ld      a, h
        xor     a, #0x80
        ld      h, a
        ex      de, hl
        pop     hl
        pop     ix
        ret
