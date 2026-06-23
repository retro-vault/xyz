        ; wmemcpy.s — copy count wide elements.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wmemcpy
        .optsdcc -mz80 sdcccall(1)
        .globl  _wmemcpy
        .area   _CODE
        ; HL = dst, DE = src, 4(ix) = count -> DE = dst
_wmemcpy::
        push    ix
        ld      ix,#0
        add     ix,sp
        ex      de,hl                   ; HL = src, DE = dst
        push    de                      ; save dst
        ld      c,4(ix)
        ld      b,5(ix)
        sla     c
        rl      b                       ; BC = 2*count (bytes)
        ld      a,b
        or      c
        jr      z,wmc_zero
        ldir
wmc_zero:
        pop     de
        pop     ix
        ret
