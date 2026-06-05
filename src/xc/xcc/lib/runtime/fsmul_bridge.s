        ; Public fsmul entry point with caller-clean modern SDCC ABI.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fsmul_bridge
        .area   _CODE
        .globl  __fsmul
        .globl  ___fsmul

__fsmul:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      c, 6(ix)
        ld      b, 7(ix)
        push    bc
        ld      c, 4(ix)
        ld      b, 5(ix)
        push    bc
        call    ___fsmul
        pop     ix
        ret
