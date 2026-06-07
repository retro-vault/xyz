        ; wcsrchr.s — last occurrence of a wide char (c==0 -> terminator).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsrchr
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsrchr
        .area   _DATA
__wcsrchr_c: .dw 0
        .area   _CODE
        ; HL = s, DE = c -> DE = pointer or 0
_wcsrchr::
        ld      (__wcsrchr_c),de        ; target
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
        ld      a,(__wcsrchr_c)
        cp      c
        jr      nz,wcr_adv
        ld      a,(__wcsrchr_c + 1)
        cp      b
        jr      nz,wcr_adv
        ld      d,h
        ld      e,l                     ; match = s
wcr_adv:
        inc     hl
        inc     hl
        jr      wcr_loop
wcr_done:
        ld      a,(__wcsrchr_c)
        ld      c,a
        ld      a,(__wcsrchr_c + 1)
        or      c
        jr      nz,wcr_ret              ; c != 0 -> return match
        ex      de,hl                   ; c == 0 -> return terminator
        ret
wcr_ret:
        ret
