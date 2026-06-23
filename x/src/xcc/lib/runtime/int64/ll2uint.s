        ; int64 to uint16 — truncate to low 16 bits (already in DE)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module ll2uint
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___ll2uint

        ; ___ll2uint
        ; inputs:  DE:HL:DE':HL' = int64
        ; outputs: DE = uint16 (low 16 bits)
        ; clobbers: none

___ll2uint:
        ret                     ; DE already holds bits[15:0]
