        ; wmemchr.s — find c in count wide elements.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wmemchr
        .optsdcc -mz80 sdcccall(1)
        .globl  _wmemchr
        .area   _CODE
        ; HL = s, DE = c, 4(ix) = count -> DE = pointer or 0
_wmemchr::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
wmh_loop:
        ld      a,b
        or      c
        jr      z,wmh_nf
        ld      a,(hl)
        cp      e
        jr      nz,wmh_next
        inc     hl
        ld      a,(hl)
        dec     hl
        cp      d
        jr      nz,wmh_next
        ex      de,hl
        pop     ix
        ret
wmh_next:
        inc     hl
        inc     hl
        dec     bc
        jr      wmh_loop
wmh_nf:
        ld      de,#0
        pop     ix
        ret
