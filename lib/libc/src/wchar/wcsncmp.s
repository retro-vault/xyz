        ; wcsncmp.s — compare up to count elements (value order, -1/0/1).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsncmp
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsncmp
        .area   _CODE
        ; HL = lhs, DE = rhs, 4(ix) = count -> DE = -1/0/1
_wcsncmp::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
wncm_loop:
        ld      a,b
        or      c
        jr      z,wncm_eq
        inc     hl
        inc     de
        ld      a,(hl)                  ; lhs high
        ex      de,hl
        cp      (hl)
        ex      de,hl
        jr      c,wncm_lt
        jr      nz,wncm_gt
        dec     hl
        dec     de
        ld      a,(hl)                  ; lhs low
        ex      de,hl
        cp      (hl)
        ex      de,hl
        jr      c,wncm_lt
        jr      nz,wncm_gt
        ld      a,(hl)                  ; element equal; NUL?
        inc     hl
        or      (hl)
        dec     hl
        jr      z,wncm_eq
        inc     hl
        inc     hl
        inc     de
        inc     de
        dec     bc
        jr      wncm_loop
wncm_lt:
        ld      de,#0xffff
        pop     ix
        ret
wncm_gt:
        ld      de,#1
        pop     ix
        ret
wncm_eq:
        ld      de,#0
        pop     ix
        ret
