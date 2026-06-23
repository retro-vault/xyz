        ; signed 8-bit division front-end helpers
        ;
        ; builds signed 16-bit operands, then tail-calls the shared
        ; signed 16-bit divide core in __sdiv16
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2000-2021 michael hope, philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module divschar
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE                   ; code segment

        .globl  __divschar_rrx_s
        .globl  __divschar_rrf_s
        .globl  __divschar
        .globl  __div8
        .globl  __div_signexte
        .globl  __sdiv16

        ; __divschar
        ; inputs:  a = dividend (signed 8-bit), l = divisor (signed 8-bit)
        ; outputs: de = quotient (signed 16-bit), hl = remainder (signed 16-bit)
        ; clobbers: a, d, e, h, l, f
__divschar_rrx_s::
__divschar_rrf_s::
__divschar:
        ld      e, l                    ; e = divisor (orig l)
        ld      l, a                    ; l = dividend low

        ; __div8
        ; inputs:  hl low contains dividend byte
        ; action:  sign-extend dividend into h
__div8::
        ld      a, l
        rlca                            ; sign bit into carry
        sbc     a, a                    ; a = 00 or ff
        ld      h, a                    ; h = sign(dividend)

        ; __div_signexte
        ; inputs:  e contains divisor byte
        ; action:  sign-extend divisor into d, then fall to __sdiv16
__div_signexte::
        ld      a, e
        rlca                            ; sign bit into carry
        sbc     a, a                    ; a = 00 or ff
        ld      d, a                    ; d = sign(divisor)
        jp      __sdiv16
