        ; Atomic flag test-and-set helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_flag_test_set
        .area   _CODE
        .globl  __atomic_flag_test_set

        ; __atomic_flag_test_set
        ; inputs: 4(ix)..5(ix) = flag pointer.
        ; outputs: L = previous flag value.
        ; clobbers: AF, H, IX.

__atomic_flag_test_set:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      a, (hl)
        ld      (hl), #1
        ld      l, a
        ei
        pop     ix
        ret
