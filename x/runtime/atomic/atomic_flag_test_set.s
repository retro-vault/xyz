        ; Atomic flag test-and-set helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_flag_test_set
        .area   _CODE
        .globl  __atomic_flag_test_set

        ; __atomic_flag_test_set
        ; inputs: HL = flag pointer.
        ; outputs: A = previous flag value.
        ; clobbers: HL.

__atomic_flag_test_set:
        di
        ld      a, (hl)
        ld      (hl), #1
        ei
        ret
