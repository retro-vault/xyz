        ; wcstok.s — reentrant wide tokenizer (state through *state).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcstok
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcstok
        .globl  __wchar_is_delim
        .area   _CODE
        ; HL = s, DE = delim, 4(ix) = state (wchar_t**)
_wcstok::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      l
        jr      nz,wtk_have
        ld      l,4(ix)
        ld      h,5(ix)                 ; HL = state
        ld      a,h
        or      l
        jr      z,wtk_ret0
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        ld      h,a
        ld      l,c                     ; HL = *state
wtk_have:
        ld      a,h
        or      l
        jr      z,wtk_ret0
wtk_skip:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        ld      b,a
        dec     hl
        ld      a,b
        or      c
        jr      z,wtk_none
        call    __wchar_is_delim
        jr      nz,wtk_start
        inc     hl
        inc     hl
        jr      wtk_skip
wtk_start:
        push    hl                      ; token start
wtk_scan:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        ld      b,a
        dec     hl
        ld      a,b
        or      c
        jr      z,wtk_setstate
        call    __wchar_is_delim
        jr      nz,wtk_next
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        jr      wtk_setstate
wtk_next:
        inc     hl
        inc     hl
        jr      wtk_scan
wtk_setstate:
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,wtk_ret_start
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
wtk_ret_start:
        pop     de
        pop     ix
        ret
wtk_none:
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,wtk_ret0
        xor     a
        ld      (bc),a
        inc     bc
        ld      (bc),a
wtk_ret0:
        ld      de,#0
        pop     ix
        ret
