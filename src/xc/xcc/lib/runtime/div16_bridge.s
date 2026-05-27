        ; ABI bridge for 16-bit unsigned division.
        ; xcc passes both operands on the stack and expects HL =
        ; quotient,
        ; DE = remainder.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module div16_bridge
        .area   _CODE
        .globl  __div16
        .globl  __divuint

        ; __div16
        ; inputs: 4(ix)..5(ix) = dividend, 6(ix)..7(ix) = divisor.
        ; outputs: HL = quotient, DE = remainder.
        ; clobbers: af, bc, de, hl, ix.

__div16:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      e, 6(ix)
        ld      d, 7(ix)
        call    __divuint
        ex      de, hl
        pop     ix
        ret
