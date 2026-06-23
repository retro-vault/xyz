        ; fixed24_8_mul3_2.s
        ;
        ; Signed 24.8 fixed-point multiply by exact 3/2.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_mul3_2
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_mul3_2

        .area   _CODE

        ; inputs:  DE:HL = a
        ; outputs: DE:HL = a * 3/2, matching generic multiply shift semantics
_fixed24_8_mul3_2::
        push    hl
        push    de
        call    .sar_dehl_1
.add_original:
        pop     bc
        ld      a,e
        add     a,c
        ld      e,a
        ld      a,d
        adc     a,b
        ld      d,a
        pop     bc
        ld      a,l
        adc     a,c
        ld      l,a
        ld      a,h
        adc     a,b
        ld      h,a
        ret

.sar_dehl_1:
        sra     h
        rr      l
        rr      d
        rr      e
        ret
