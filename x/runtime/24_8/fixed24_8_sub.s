        ; fixed24_8_sub.s
        ;
        ; Signed 24.8 subtract. Two's-complement overflow wraps.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_sub
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_sub

        .area   _CODE

        ; inputs:  DE:HL = a, 4(ix)..7(ix) = b
        ; outputs: DE:HL = a - b
_fixed24_8_sub::
        push    ix
        ld      ix,#0
        add     ix,sp
        or      a
        ld      a,e
        sbc     a,4(ix)
        ld      e,a
        ld      a,d
        sbc     a,5(ix)
        ld      d,a
        ld      a,l
        sbc     a,6(ix)
        ld      l,a
        ld      a,h
        sbc     a,7(ix)
        ld      h,a
        pop     ix
        ret
