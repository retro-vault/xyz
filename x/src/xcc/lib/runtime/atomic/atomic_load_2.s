        ; 2-byte atomic load helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_load_2
        .area   _CODE
        .globl  __atomic_load_2

        ; __atomic_load_2
        ; inputs: HL = pointer.
        ; outputs: DE = loaded 16-bit value.
        ; clobbers: A, HL.

__atomic_load_2:
        di
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ei
        ret
