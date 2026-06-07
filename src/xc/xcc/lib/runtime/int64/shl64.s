        ; 64-bit logical left shift
        ; a in DE:HL:DE':HL', count at ix+4 (low byte of stack arg)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module shl64
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __shl64

        ; __shl64
        ; inputs:  DE:HL:DE':HL' = value, ix+4 = shift count (0..63)
        ; outputs: DE:HL:DE':HL' = value << count
        ; clobbers: af, bc (for frame), bc'

__shl64:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, 4(ix)        ; load count into B
        pop     ix              ; restore IX immediately (no locals needed)

        ld      a, b
        or      a
        ret     z               ; count = 0: done

.shl64_loop:
        ; Shift DE:HL:DE':HL' left by 1 bit.
        ; DE is lsb, HL is mid-low, DE' is mid-high, HL' is msb.
        ; For left shift: start from lsb (E), propagate carry upward.
        sla     e               ; E bit7 → carry, 0 → bit0
        rl      d               ; carry → D bit0, D bit7 → carry
        rl      l               ; carry → L bit0, L bit7 → carry
        rl      h               ; carry → H bit0, H bit7 → carry (crosses to high32)
        exx
        rl      e               ; carry → E' bit0
        rl      d
        rl      l
        rl      h               ; H' bit7 discarded (overflow)
        exx
        djnz    .shl64_loop
        ret
