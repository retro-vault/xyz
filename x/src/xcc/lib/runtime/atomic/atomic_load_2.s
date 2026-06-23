        ; 2-byte atomic load helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_load_2
        .area   _CODE
        .globl  __atomic_load_2

        ; __atomic_load_2
        ; inputs: 4(ix)..5(ix) = pointer.
        ; outputs: HL = loaded 16-bit value.
        ; clobbers: DE, IX.

__atomic_load_2:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ; pointer low byte
        ld      l, 4(ix)
        ; pointer high byte
        ld      h, 5(ix)
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ex      de, hl
        ei
        pop     ix
        ret
