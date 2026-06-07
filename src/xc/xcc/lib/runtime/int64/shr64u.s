        ; 64-bit logical (unsigned) right shift
        ; a in DE:HL:DE':HL', count at ix+4
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module shr64u
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __shr64u

        ; __shr64u
        ; inputs:  DE:HL:DE':HL' = value, ix+4 = shift count (0..63)
        ; outputs: DE:HL:DE':HL' = value >> count (logical, 0 fills msb)
        ; clobbers: af, b

__shr64u:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, 4(ix)
        pop     ix

        ld      a, b
        or      a
        ret     z

.shr64u_loop:
        ; Shift right logical: start from msb (H' in alternate bank), carry down.
        exx
        srl     h               ; H' bit7 ← 0, H' bit0 → carry
        rr      l               ; carry → L' bit7
        rr      d
        rr      e               ; E' bit0 → carry (crosses to low32)
        exx
        rr      h               ; carry → H bit7
        rr      l
        rr      d
        rr      e               ; lsb: E bit0 discarded
        djnz    .shr64u_loop
        ret
