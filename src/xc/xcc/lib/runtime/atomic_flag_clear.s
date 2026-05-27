        ; Atomic flag clear helper for the merged runtime.
        ; Uses DI/EI for simple preemptive-system atomicity.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atomic_flag_clear
        .area   _CODE
        .globl  __atomic_flag_clear

        ; __atomic_flag_clear
        ; inputs: 4(ix)..5(ix) = flag pointer.
        ; outputs: none.
        ; clobbers: HL, IX.

__atomic_flag_clear:
        push    ix
        ld      ix, #0
        add     ix, sp
        di
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      (hl), #0
        ei
        pop     ix
        ret
