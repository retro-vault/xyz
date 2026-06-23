        ; int64 to int32 — truncate to low 32 bits (already in DE:HL)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module ll2slong
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___ll2slong

        ; ___ll2slong
        ; inputs:  DE:HL:DE':HL' = int64
        ; outputs: DE=low16, HL=high16 (int32 low 32 bits)
        ; clobbers: none

___ll2slong:
        ret                     ; DE and HL already hold bits[31:0]
