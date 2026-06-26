        ; 2-byte atomic compare-exchange helper.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_compare_exchange_2
        .area   _CODE
        .globl  __atomic_compare_exchange_2

        ; __atomic_compare_exchange_2
        ; inputs: HL = pointer, DE = expected, desired at 4(ix)..5(ix).
        ; outputs: DE = 1 on swap, DE = 0 on mismatch.
        ; clobbers: AF, BC, HL, IX.

__atomic_compare_exchange_2:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ld      a, (hl)
        cp      e
        jr      nz, .acas2_fail
        inc     hl
        ld      a, (hl)
        cp      d
        jr      nz, .acas2_fail
        ld      b, 5(ix)
        ld      a, 4(ix)
        dec     hl
        ld      (hl), a
        inc     hl
        ld      (hl), b
        ld      de, #1
        ei
        pop     ix
        ret
.acas2_fail:
        ld      de, #0
        ei
        pop     ix
        ret
