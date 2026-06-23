        ; wcsstr.s — locate a wide substring.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsstr
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsstr
        .globl  _wcslen
        .globl  _wcsncmp

WCSSTR_NDL  .equ -4
WCSSTR_LEN  .equ -2

        .area   _CODE
        ; HL = haystack, DE = needle -> DE = pointer or 0
_wcsstr::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l                     ; preserve haystack across frame setup
        ld      b,h
        ld      hl,#-4
        add     hl,sp
        ld      sp,hl
        ld      WCSSTR_NDL(ix),e
        ld      WCSSTR_NDL + 1(ix),d
        ld      l,c
        ld      h,b
        push    hl                      ; haystack
        ex      de,hl                   ; HL = needle
        call    _wcslen                 ; DE = needle length
        ld      a,d
        or      e
        jr      z,wcst_ret_h            ; empty needle -> haystack
        ld      WCSSTR_LEN(ix),e
        ld      WCSSTR_LEN + 1(ix),d
        pop     hl                      ; haystack
wcst_loop:
        ld      a,(hl)
        inc     hl
        or      (hl)
        dec     hl
        jr      z,wcst_nf               ; end of haystack
        push    hl
        ld      e,WCSSTR_LEN(ix)
        ld      d,WCSSTR_LEN + 1(ix)
        push    de                      ; stacked count for wcsncmp
        ld      e,WCSSTR_NDL(ix)
        ld      d,WCSSTR_NDL + 1(ix)
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
        jr      wcst_leave
wcst_nf:
        ld      de,#0
        jr      wcst_leave
wcst_ret_h:
        pop     hl
        ex      de,hl
wcst_leave:
        ld      sp,ix
        pop     ix
        ret
