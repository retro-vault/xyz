        ; ABI bridge for 16-bit multiply.
        ; xcc passes both operands on the stack and expects HL =
        ; product.
        ; The shared core takes HL/DE and returns DE.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mul16_bridge
        .area   _CODE
        .globl  __mul16
        .globl  __mulint

        ; __mul16
        ; inputs: 4(ix)..5(ix) = left operand, 6(ix)..7(ix) = right
        ; operand.
        ; outputs: HL = low 16 bits of the product.
        ; clobbers: af, bc, de, hl, ix.

__mul16:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      e, 6(ix)
        ld      d, 7(ix)
        call    __mulint
        ex      de, hl
        pop     ix
        ret
