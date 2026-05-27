        ; ABI bridge for 16-bit unsigned modulus.
        ; xcc's caller expects the helper to return the remainder in DE,
        ; because
        ; code generation does a final `ex de,hl` at the call site.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mod16_bridge
        .area   _CODE
        .globl  __mod16
        .globl  __moduint

        ; __mod16
        ; inputs: 4(ix)..5(ix) = dividend, 6(ix)..7(ix) = divisor.
        ; outputs: DE = remainder for the xcc caller-side swap.
        ; clobbers: af, bc, de, hl, ix.

__mod16:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      e, 6(ix)
        ld      d, 7(ix)
        call    __moduint
        ex      de, hl
        pop     ix
        ret
