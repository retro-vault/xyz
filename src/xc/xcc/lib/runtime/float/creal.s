        ; Real-part accessor for float complex values.
        ; Returns the low two words of the incoming complex argument.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module creal
        .area   _CODE
        .globl  __creal

        ; __creal
        ; inputs: 4(ix)..11(ix) = complex value, real words first.
        ; outputs: DE = high 16 bits, HL = low 16 bits of the real part.
        ; clobbers: IX.

__creal:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 4(ix)
        ld      h, 5(ix)
        push    hl
        ld      l, 6(ix)
        ld      h, 7(ix)
        ex      de, hl
        pop     hl
        pop     ix
        ret
