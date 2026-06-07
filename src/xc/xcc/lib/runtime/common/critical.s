        ; Shared critical-section entry helper
        ;
        ; returns previous interrupt enable state in flags (P/V from `ld
        ; a,i`)
        ; and disables interrupts. caller can later restore with:
        ;   jp po, no_ei
        ;   ei
        ; no_ei:
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module critical
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___sdcc_critical
        .globl  __sdcc_critical

___sdcc_critical:
        ; __sdcc_critical
        ; inputs:  n/a
        ; outputs: A = I register, P/V reflects previous IFF2
        ; clobbers: A, F
__sdcc_critical:
        ld      a, i
        di
        ret
