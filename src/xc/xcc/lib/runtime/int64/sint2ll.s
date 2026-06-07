        ; signed int16 to int64 — sign-extend HL to DE:HL:DE':HL'
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module sint2ll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___sint2ll

        ; ___sint2ll
        ; inputs:  HL = int16
        ; outputs: DE:HL:DE':HL' = sign-extended int64
        ; clobbers: af, de, de', hl'

___sint2ll:
        ld      d, h
        ld      e, l            ; DE = int16 (bits[15:0])
        ld      a, h
        rlca                    ; sign → carry
        sbc     a, a            ; A = 0xFF if negative, 0x00 if positive
        ld      h, a
        ld      l, a            ; HL = sign-extended word (bits[31:16])
        exx
        ld      d, a
        ld      e, a            ; DE' = sign-extended word (bits[47:32])
        ld      h, a
        ld      l, a            ; HL' = sign-extended word (bits[63:48])
        exx
        ret
