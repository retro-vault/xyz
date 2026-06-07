        ; wmemcmp.s — compare count wide elements (value order, -1/0/1).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wmemcmp
        .optsdcc -mz80 sdcccall(1)
        .globl  _wmemcmp
        .area   _CODE
        ; HL = lhs, DE = rhs, 4(ix) = count -> DE = -1/0/1
_wmemcmp::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
wmm_loop:
        ld      a,b
        or      c
        jr      z,wmm_eq
        inc     hl
        inc     de
        ld      a,(hl)                  ; lhs high
        ex      de,hl
        cp      (hl)                    ; lhs_high - rhs_high
        ex      de,hl
        jr      c,wmm_lt
        jr      nz,wmm_gt
        dec     hl
        dec     de
        ld      a,(hl)                  ; lhs low
        ex      de,hl
        cp      (hl)                    ; lhs_low - rhs_low
        ex      de,hl
        jr      c,wmm_lt
        jr      nz,wmm_gt
        inc     hl
        inc     hl
        inc     de
        inc     de
        dec     bc
        jr      wmm_loop
wmm_lt:
        ld      de,#0xffff
        pop     ix
        ret
wmm_gt:
        ld      de,#1
        pop     ix
        ret
wmm_eq:
        ld      de,#0
        pop     ix
        ret
