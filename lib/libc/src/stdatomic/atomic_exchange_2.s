        ; 2-byte atomic exchange helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_exchange_2
        .area   _CODE
        .globl  __atomic_exchange_2

        ; __atomic_exchange_2
        ; inputs: 4(ix)..5(ix) = pointer, 6(ix)..7(ix) = new value.
        ; outputs: HL = previous 16-bit value.
        ; clobbers: AF, DE, IX.

__atomic_exchange_2:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        dec     hl
        ld      a, 6(ix)
        ld      (hl), a
        inc     hl
        ld      a, 7(ix)
        ld      (hl), a
        ex      de, hl
        ei
        pop     ix
        ret
