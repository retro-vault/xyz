        ; wcsrchr.s — last occurrence of a wide char (c==0 -> terminator).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsrchr
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsrchr
        .area   _CODE
        ; HL = s, DE = c -> DE = pointer or 0
_wcsrchr::
        push    ix
        push    de
        ld      ix,#0
        add     ix,sp
        ld      de,#0                   ; match = NULL
wcr_loop:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        ld      b,a
        dec     hl                      ; BC = *s
        ld      a,b
        or      c
        jr      z,wcr_done              ; end of string
        ld      a,0(ix)
        cp      c
        jr      nz,wcr_adv
        ld      a,1(ix)
        cp      b
        jr      nz,wcr_adv
        ld      d,h
        ld      e,l                     ; match = s
wcr_adv:
        inc     hl
        inc     hl
        jr      wcr_loop
wcr_done:
        ld      a,0(ix)
        ld      c,a
        ld      a,1(ix)
        or      c
        jr      nz,wcr_ret              ; c != 0 -> return match
        ex      de,hl                   ; c == 0 -> return terminator
        ld      sp,ix
        pop     bc                      ; discard saved search code unit
        pop     ix
        ret
wcr_ret:
        ld      sp,ix
        pop     bc                      ; discard saved search code unit
        pop     ix
        ret
