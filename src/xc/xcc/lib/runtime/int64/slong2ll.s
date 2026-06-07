        ; signed int32 to int64 — sign-extend DE:HL to DE:HL:DE':HL'
        ; input uses existing 32-bit ABI: DE=low16, HL=high16
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module slong2ll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___slong2ll

        ; ___slong2ll
        ; inputs:  DE = int32 low16, HL = int32 high16
        ; outputs: DE:HL:DE':HL' = sign-extended int64
        ;          (DE and HL unchanged; DE', HL' filled with sign byte)
        ; clobbers: af, de', hl'

___slong2ll:
        ; Sign byte from bit7 of H (most-significant byte of int32)
        ld      a, h
        rlca                    ; sign → carry
        sbc     a, a            ; A = 0xFF if negative, 0x00 if positive
        exx
        ld      d, a
        ld      e, a            ; DE' = sign word (bits[47:32])
        ld      h, a
        ld      l, a            ; HL' = sign word (bits[63:48])
        exx
        ret
