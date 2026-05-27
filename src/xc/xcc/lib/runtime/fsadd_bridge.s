        ; ABI bridge for IEEE-754 single addition.
        ; xcc passes both operands on the stack and expects HL = low16,
        ; DE =
        ; high16.
        ; The shared core takes the left operand in DE:HL,
        ; the
        ; right
        ; operand on the stack, pops that right operand itself, and
        ; returns
        ; DE = low16, HL = high16.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fsadd_bridge
        .area   _CODE
        .globl  __fsadd
        .globl  ___fsadd

        ; __fsadd
        ; inputs: 4(ix)..11(ix) = xcc 32-bit float operands.
        ; outputs: HL = low16 sum, DE = high16 sum.
        ; clobbers: af, bc, de, hl, ix.

__fsadd:
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
        call    ___fsadd
        ex      de, hl
        pop     ix
        ret
