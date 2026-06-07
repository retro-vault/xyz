        ; 64-bit arithmetic (signed) right shift
        ; a in DE:HL:DE':HL', count at ix+4
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module shr64s
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __shr64s

        ; __shr64s
        ; inputs:  DE:HL:DE':HL' = value, ix+4 = shift count (0..63)
        ; outputs: DE:HL:DE':HL' = value >> count (arithmetic, sign bit propagates)
        ; clobbers: af, b

__shr64s:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, 4(ix)
        pop     ix

        ld      a, b
        or      a
        ret     z

.shr64s_loop:
        ; Arithmetic right shift: SRA on msb (H') copies sign bit.
        exx
        sra     h               ; H' bit7 preserved (sign), H' bit0 → carry
        rr      l
        rr      d
        rr      e               ; E' bit0 → carry (crosses to low32)
        exx
        rr      h               ; carry → H bit7
        rr      l
        rr      d
        rr      e
        djnz    .shr64s_loop
        ret
