        ; wcsncat.s — append up to count elements, then terminate.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsncat
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsncat
        .area   _CODE
        ;; _wcsncat
        ;; Append at most count wchar_t elements from src and always leave dst
        ;; terminated. If src ends earlier, the copied NUL stops the loop naturally.
_wcsncat::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; Preserve the original destination for the return value.
wnca_end:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        inc     hl
        or      c
        jr      nz,wnca_end
        dec     hl
        dec     hl                      ; Rewind from the probe step to the terminator slot.
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = remaining element count
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
        jr      z,wnca_done             ; Copying the source terminator finishes the append.
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
