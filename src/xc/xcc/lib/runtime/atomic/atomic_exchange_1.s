        ; 1-byte atomic exchange helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_exchange_1
        .area   _CODE
        .globl  __atomic_exchange_1

        ; __atomic_exchange_1
        ; inputs: 4(ix)..5(ix) = pointer, 6(ix) = new value.
        ; outputs: L = previous byte.
        ; clobbers: AF, E, H, IX.

__atomic_exchange_1:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      a, (hl)
        ld      e, 6(ix)
        ld      (hl), e
        ld      l, a
        ei
        pop     ix
        ret
