        ; Atomic flag clear helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_flag_clear
        .area   _CODE
        .globl  __atomic_flag_clear

        ; __atomic_flag_clear
        ; inputs: HL = flag pointer.
        ; outputs: none.
        ; clobbers: HL.

__atomic_flag_clear:
        di
        ld      (hl), #0
        ei
        ret
