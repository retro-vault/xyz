        ; wcsncat.s — append up to count elements, then terminate.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsncat
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsncat
        .area   _CODE
        ; HL = dst, DE = src, 4(ix) = count -> DE = dst
_wcsncat::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; dst (return)
wnca_end:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        inc     hl
        or      c
        jr      nz,wnca_end
        dec     hl
        dec     hl                      ; HL = dst terminator
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = count
wnca_copy:
        ld      a,b
        or      c
        jr      z,wnca_term
        ld      a,(de)
        ld      (hl),a
        inc     de
        inc     hl
        ld      a,(de)
        ld      (hl),a
        inc     de
        inc     hl
        dec     hl
        dec     hl
        or      (hl)                    ; copied char == 0 ?
        inc     hl
        inc     hl
        jr      z,wnca_done             ; copied the NUL -> finished
        dec     bc
        jr      wnca_copy
wnca_term:
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
wnca_done:
        pop     de
        pop     ix
        ret
