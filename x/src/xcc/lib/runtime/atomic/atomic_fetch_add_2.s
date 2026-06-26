        ; 2-byte atomic fetch-add helper.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_fetch_add_2
        .area   _CODE
        .globl  __atomic_fetch_add_2

        ; __atomic_fetch_add_2
        ; inputs: HL = pointer, DE = value.
        ; outputs: DE = previous 16-bit value.
        ; clobbers: A, BC, HL.

__atomic_fetch_add_2:
        di
        ld      a, (hl)
        ld      c, a
        add     a, e
        ld      (hl), a
        inc     hl
        ld      a, (hl)
        ld      b, a
        adc     a, d
        ld      (hl), a
        ld      e, c
        ld      d, b
        ei
        ret
