        ; ABI bridge for 32-bit signed modulus.
        ; xcc expects HL = low16 remainder, DE = high16 remainder.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module smod32_bridge
        .area   _CODE
        .globl  __smod32
        .globl  __modslong

        ; __smod32
        ; inputs: 4(ix)..11(ix) = xcc 32-bit operands on the stack.
        ; outputs: HL = low16 remainder, DE = high16 remainder.
        ; clobbers: af, bc, de, hl, ix.

__smod32:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      e, 4(ix)
        ld      d, 5(ix)                ; DE = dividend low16
        ld      l, 6(ix)
        ld      h, 7(ix)                ; HL = dividend high16
        ld      c, 10(ix)
        ld      b, 11(ix)
        push    bc                      ; divisor high16 (pushed first)
        ld      c, 8(ix)
        ld      b, 9(ix)
        push    bc                      ; divisor low16 (at 4(ix) inside __modslong)
        call    __modslong
        pop     bc
        pop     bc
        ex      de, hl                  ; __modslong: DE=low16 -> HL=low16, DE=high16
        pop     ix
        ret
