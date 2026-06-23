        ; unsigned int16 to int64 — zero-extend HL to DE:HL:DE':HL'
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module uint2ll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___uint2ll

        ; ___uint2ll
        ; inputs:  HL = uint16
        ; outputs: DE:HL:DE':HL' = zero-extended uint64
        ; clobbers: af, de, de', hl'

___uint2ll:
        ld      d, h
        ld      e, l            ; DE = uint16 (bits[15:0])
        xor     a
        ld      h, a
        ld      l, a            ; HL = 0 (bits[31:16])
        exx
        ld      d, a
        ld      e, a            ; DE' = 0 (bits[47:32])
        ld      h, a
        ld      l, a            ; HL' = 0 (bits[63:48])
        exx
        ret
