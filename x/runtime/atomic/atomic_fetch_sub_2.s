        ; 2-byte atomic fetch-sub helper.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_fetch_sub_2
        .area   _CODE
        .globl  __atomic_fetch_sub_2

        ; __atomic_fetch_sub_2
        ; inputs: 4(ix)..5(ix) = pointer, 6(ix)..7(ix) = value.
        ; outputs: HL = previous 16-bit value.
        ; clobbers: AF, BC, DE, IX.

__atomic_fetch_sub_2:
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
        ld      c, 6(ix)
        ld      b, 7(ix)
        push    de
        ex      de, hl
        or      a, a
        sbc     hl, bc
        ex      de, hl
        ld      (hl), e
        inc     hl
        ld      (hl), d
        pop     hl
        ei
        pop     ix
        ret
