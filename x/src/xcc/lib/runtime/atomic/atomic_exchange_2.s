        ; 2-byte atomic exchange helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_exchange_2
        .area   _CODE
        .globl  __atomic_exchange_2

        ; __atomic_exchange_2
        ; inputs: HL = pointer, DE = new value.
        ; outputs: DE = previous 16-bit value.
        ; clobbers: A, BC, HL.

__atomic_exchange_2:
        di
        ld      a, (hl)
        ld      c, a
        ld      (hl), e
        inc     hl
        ld      a, (hl)
        ld      b, a
        ld      (hl), d
        ld      e, c
        ld      d, b
        ei
        ret
