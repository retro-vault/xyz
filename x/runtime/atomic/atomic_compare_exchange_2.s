        ; 2-byte atomic compare-exchange helper.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_compare_exchange_2
        .area   _CODE
        .globl  __atomic_compare_exchange_2

        ; __atomic_compare_exchange_2
        ; inputs: ptr at 4(ix)..5(ix), expected at 6(ix)..7(ix), desired
        ; at 8(ix)..9(ix).
        ; outputs: HL = 1 on swap, HL = 0 on mismatch.
        ; clobbers: AF, DE, IX.

__atomic_compare_exchange_2:
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
        cp      a, e
        jr      nz, .acas2_fail
        ld      a, 7(ix)
        cp      a, d
        jr      nz, .acas2_fail
        ld      a, 8(ix)
        ld      (hl), a
        inc     hl
        ld      a, 9(ix)
        ld      (hl), a
        ld      hl, #1
        ei
        pop     ix
        ret
.acas2_fail:
        ld      hl, #0
        ei
        pop     ix
        ret
