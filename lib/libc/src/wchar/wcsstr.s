        ; wcsstr.s — locate a wide substring.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsstr
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsstr
        .globl  _wcslen
        .globl  _wcsncmp
        .area   _DATA
__wcsstr_ndl: .dw 0
__wcsstr_len: .dw 0
        .area   _CODE
        ; HL = haystack, DE = needle -> DE = pointer or 0
_wcsstr::
        ld      (__wcsstr_ndl),de
        push    hl                      ; haystack
        ex      de,hl                   ; HL = needle
        call    _wcslen                 ; DE = needle length
        ld      a,d
        or      e
        jr      z,wcst_ret_h            ; empty needle -> haystack
        ld      (__wcsstr_len),de
        pop     hl                      ; haystack
wcst_loop:
        ld      a,(hl)
        inc     hl
        or      (hl)
        dec     hl
        jr      z,wcst_nf               ; end of haystack
        push    hl
        ld      de,(__wcsstr_len)
        push    de                      ; stacked count for wcsncmp
        ld      de,(__wcsstr_ndl)
        call    _wcsncmp
        pop     bc                      ; clean count
        ld      a,d
        or      e
        pop     hl
        jr      z,wcst_found
        inc     hl
        inc     hl
        jr      wcst_loop
wcst_found:
        ex      de,hl
        ret
wcst_nf:
        ld      de,#0
        ret
wcst_ret_h:
        pop     hl
        ex      de,hl
        ret
