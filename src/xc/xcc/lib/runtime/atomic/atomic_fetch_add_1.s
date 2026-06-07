        ; 1-byte atomic fetch-add helper.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_fetch_add_1
        .area   _CODE
        .globl  __atomic_fetch_add_1

        ; __atomic_fetch_add_1
        ; inputs: 4(ix)..5(ix) = pointer, 6(ix) = value.
        ; outputs: L = previous byte.
        ; clobbers: AF, E, H, IX.

__atomic_fetch_add_1:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      a, (hl)
        ld      e, a
        add     a, 6(ix)
        ld      (hl), a
        ld      l, e
        ei
        pop     ix
        ret
