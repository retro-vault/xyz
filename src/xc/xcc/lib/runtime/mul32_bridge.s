        ; ABI bridge for 32-bit multiply.
        ; xcc passes both operands on the stack and expects HL = low16,
        ; DE =
        ; high16.
        ; The shared core takes the left operand in DE:HL,
        ; the
        ; right
        ; operand on the stack, and returns DE = low16, HL = high16.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mul32_bridge
        .area   _CODE
        .globl  __mul32
        .globl  __mullong

        ; __mul32
        ; inputs: 4(ix)..11(ix) = xcc 32-bit operands on the stack.
        ; outputs: HL = low16 product, DE = high16 product.
        ; clobbers: af, bc, de, hl, ix.

__mul32:
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
        call    __mullong
        pop     bc
        pop     bc
        ex      de, hl
        pop     ix
        ret
