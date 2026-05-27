        ; ABI bridge for 32-bit unsigned modulus.
        ; xcc expects HL = low16 remainder, DE = high16 remainder.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mod32_bridge
        .area   _CODE
        .globl  __mod32
        .globl  __modulong

        ; __mod32
        ; inputs: 4(ix)..11(ix) = xcc 32-bit operands on the stack.
        ; outputs: HL = low16 remainder, DE = high16 remainder.
        ; clobbers: af, bc, de, hl, ix.

__mod32:
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
        call    __modulong
        pop     bc
        pop     bc
        ex      de, hl
        pop     ix
        ret
