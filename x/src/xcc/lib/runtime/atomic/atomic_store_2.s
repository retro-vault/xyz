        ; 2-byte atomic store helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_store_2
        .area   _CODE
        .globl  __atomic_store_2

        ; __atomic_store_2
        ; inputs: HL = pointer, DE = value.
        ; outputs: none.
        ; clobbers: A, HL.

__atomic_store_2:
        di
        ld      (hl), e
        inc     hl
        ld      (hl), d
        ei
        ret
