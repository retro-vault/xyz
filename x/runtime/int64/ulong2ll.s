        ; unsigned int32 to int64 — zero-extend DE:HL to DE:HL:DE':HL'
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module ulong2ll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___ulong2ll

        ; ___ulong2ll
        ; inputs:  DE = uint32 low16, HL = uint32 high16
        ; outputs: DE:HL:DE':HL' = zero-extended uint64
        ; clobbers: af, de', hl'

___ulong2ll:
        xor     a
        exx
        ld      d, a
        ld      e, a            ; DE' = 0 (bits[47:32])
        ld      h, a
        ld      l, a            ; HL' = 0 (bits[63:48])
        exx
        ret
