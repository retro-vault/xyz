        ; ABI bridge for 16-bit signed division.
        ; xcc passes both operands on the stack and expects HL = quotient.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module sdiv16_bridge
        .area   _CODE
        .globl  __sdiv16
        .globl  __divsint

        ; __sdiv16
        ; inputs: 4(ix)..5(ix) = dividend, 6(ix)..7(ix) = divisor.
        ; outputs: HL = quotient.
        ; clobbers: af, bc, de, hl, ix.

__sdiv16:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      e, 6(ix)
        ld      d, 7(ix)
        call    __divsint
        ex      de, hl          ; __divsint: DE=quotient -> move to HL
        pop     ix
        ret
