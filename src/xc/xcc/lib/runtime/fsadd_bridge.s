        ; Public fsadd entry point with caller-clean modern SDCC ABI.
        ; The shared core self-pops a copied right operand, so this shim
        ; duplicates the caller's stack argument and leaves the original
        ; bytes in place for the real caller to clean.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fsadd_bridge
        .area   _CODE
        .globl  __fsadd
        .globl  ___fsadd

__fsadd:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      c, 6(ix)
        ld      b, 7(ix)
        push    bc
        ld      c, 4(ix)
        ld      b, 5(ix)
        push    bc
        call    ___fsadd
        pop     ix
        ret
