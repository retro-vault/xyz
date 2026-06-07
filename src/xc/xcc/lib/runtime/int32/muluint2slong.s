        ; 16x16 -> 32 unsigned multiply, returns de:hl (low:high)
        ; shifts (iy:hl) left; if msb of multiplier set, adds de to low
        ; word
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module muluint2slong
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE                   ; code segment

        .globl  ___muluint2ulong        ; export symbol

        ; ___muluint2ulong
        ; inputs:  hl = multiplier (u16), de = multiplicand (u16)
        ; outputs: de:hl = product (u32) with de = low, hl = high
        ; clobbers: b, iy, h, l, d, e, f
        ; notes: uses shift-add algorithm; (iy:hl) holds partial product
___muluint2ulong:
        ld      iy, #0
        ld      b, #16
.loop:
        add     iy, iy
        adc     hl, hl
        jr      nc, .skip
        add     iy, de
        jr      nc, .skip
        inc     hl
.skip:
        djnz    .loop                   ; next bit
        push    iy
        pop     de
        ret                             ; de:hl = product
