        ; signed 16x16 -> 32 multiply using unsigned core and sign fix
        ; takes signed hl and de, multiplies via ___muluint2ulong,
        ; negates if needed
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module mulsint2slong
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE                   ; code segment

        .globl  ___muluint2ulong        ; import unsigned core
        .globl  ___mulsint2slong        ; export symbol

        ; ___mulsint2slong
        ; inputs:  hl = multiplicand (signed 16-bit)
        ;          de = multiplier   (signed 16-bit)
        ; outputs: de:hl = product (signed 32-bit, de=low, hl=high)
        ; clobbers: a, b, c, d, e, h, l, f
        ; notes: lsb of c tracks sign of result; b cached as 0 to help
        ; negation
___mulsint2slong:
        ; use lowest bit of c to remember if result needs negation. b =
        ; 0.
        ld      bc, #0

        bit     #7, h                   ; test sign of hl
        jr      z, .hl_nonneg
        ld      a, b                    ; a = 0
        sub     a, l                    ; l <- -l
        ld      l, a
        ld      a, b                    ; a = 0
        sbc     a, h                    ; h <- -h with borrow
        ld      h, a
        inc     c                       ; flip sign flag (c^=1)
.hl_nonneg:
        bit     #7, d                   ; test sign of de
        jr      z, .de_nonneg
        ld      a, b                    ; a = 0
        sub     a, e                    ; e <- -e
        ld      e, a
        ld      a, b                    ; a = 0
        sbc     a, d                    ; d <- -d with borrow
        ld      d, a
        inc     c                       ; flip sign flag (c^=1)
.de_nonneg:
        push    bc
        call    ___muluint2ulong
        pop     bc

        bit     #0, c                   ; is result negative?
        ret     z                       ; no -> done

        ; negate 32-bit result in de:hl (two's complement): 0 - (de:hl)
        ld      a, b                    ; a = 0
        sub     a, e                    ; e <- -e
        ld      e, a
        ld      a, b                    ; a = 0
        sbc     a, d                    ; d <- -d - carry
        ld      d, a
        ld      a, b                    ; a = 0
        sbc     a, l                    ; l <- -l - carry
        ld      l, a
        ld      a, b                    ; a = 0
        sbc     a, h                    ; h <- -h - carry
        ld      h, a
        ret
