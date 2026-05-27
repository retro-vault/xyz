        ; ABI bridge for IEEE-754 single division.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fsdiv_bridge
        .area   _CODE
        .globl  __fsdiv
        .globl  ___fsdiv

        ; __fsdiv
        ; inputs: 4(ix)..11(ix) = xcc 32-bit float operands.
        ; outputs: HL = low16 quotient, DE = high16 quotient.
        ; clobbers: af, bc, de, hl, ix.

__fsdiv:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      e, 4(ix)
        ld      d, 5(ix)
        ld      l, 6(ix)
        ld      h, 7(ix)
        ld      c, 10(ix)
        ld      b, 11(ix)
        push    bc
        ld      c, 8(ix)
        ld      b, 9(ix)
        push    bc
        call    ___fsdiv
        ex      de, hl
        pop     ix
        ret
