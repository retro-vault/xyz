        ; 1-byte atomic compare-exchange helper.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_compare_exchange_1
        .area   _CODE
        .globl  __atomic_compare_exchange_1

        ; __atomic_compare_exchange_1
        ; inputs: ptr at 4(ix)..5(ix), expected at 6(ix), desired at
        ; 8(ix).
        ; outputs: HL = 1 on swap, HL = 0 on mismatch.
        ; clobbers: AF, E, IX.

__atomic_compare_exchange_1:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      a, (hl)
        ld      e, 6(ix)
        cp      a, e
        jr      nz, .acas1_fail
        ld      a, 8(ix)
        ld      (hl), a
        ld      hl, #1
        ei
        pop     ix
        ret
.acas1_fail:
        ld      hl, #0
        ei
        pop     ix
        ret
