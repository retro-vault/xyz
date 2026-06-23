        ; 2-byte atomic store helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_store_2
        .area   _CODE
        .globl  __atomic_store_2

        ; __atomic_store_2
        ; inputs: 4(ix)..5(ix) = pointer, 6(ix)..7(ix) = value.
        ; outputs: none.
        ; clobbers: AF, HL, IX.

__atomic_store_2:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      a, 6(ix)
        ld      (hl), a
        inc     hl
        ld      a, 7(ix)
        ld      (hl), a
        ei
        pop     ix
        ret
