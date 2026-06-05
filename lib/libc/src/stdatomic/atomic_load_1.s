        ; 1-byte atomic load helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_load_1
        .area   _CODE
        .globl  __atomic_load_1

        ; __atomic_load_1
        ; inputs: 4(ix)..5(ix) = pointer.
        ; outputs: L = loaded byte.
        ; clobbers: H, IX.

__atomic_load_1:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ; pointer low byte
        ld      l, 4(ix)
        ; pointer high byte
        ld      h, 5(ix)
        ld      l, (hl)
        ei
        pop     ix
        ret
