        ; Imaginary-part accessor for float complex values.
        ; Returns the upper two words of the incoming complex argument.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module cimag
        .area   _CODE
        .globl  __cimag

        ; __cimag
        ; inputs: 4(ix)..11(ix) = complex value, real words first.
        ; outputs: DE = high 16 bits, HL = low 16 bits of the imag part.
        ; clobbers: IX.

__cimag:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 8(ix)
        ld      h, 9(ix)
        push    hl
        ld      l, 10(ix)
        ld      h, 11(ix)
        ex      de, hl
        pop     hl
        pop     ix
        ret
