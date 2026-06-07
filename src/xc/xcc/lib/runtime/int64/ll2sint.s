        ; int64 to int16 — truncate to low 16 bits (already in DE)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module ll2sint
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___ll2sint

        ; ___ll2sint
        ; inputs:  DE:HL:DE':HL' = int64
        ; outputs: DE = int16 (low 16 bits, sign-truncated)
        ; clobbers: none

___ll2sint:
        ret                     ; DE already holds bits[15:0]
