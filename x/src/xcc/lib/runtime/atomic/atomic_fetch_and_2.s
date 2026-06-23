        ; 2-byte atomic fetch-and helper.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_fetch_and_2
        .area   _CODE
        .globl  __atomic_fetch_and_2

        ; __atomic_fetch_and_2
        ; inputs: 4(ix)..5(ix) = pointer, 6(ix)..7(ix) = value.
        ; outputs: HL = previous 16-bit value.
        ; clobbers: AF, DE, IX.

__atomic_fetch_and_2:
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
        push    de
        ld      a, e
        and     a, 6(ix)
        ld      (hl), a
        inc     hl
        ld      a, d
        and     a, 7(ix)
        ld      (hl), a
        pop     hl
        ei
        pop     ix
        ret
