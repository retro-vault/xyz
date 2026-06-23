        ; wcscspn.s — leading run length of chars NOT in reject.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcscspn
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcscspn
        .globl  __wchar_is_delim
        .area   _CODE
        ;; _wcscspn
        ;; Advance over the leading wchar_t run that does not belong to reject.
        ;; The distance is accumulated as a byte delta and converted back to
        ;; element count at the end.
_wcscspn::
        push    hl                      ; start
wcsp_loop:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        ld      b,a
        dec     hl                      ; BC = *s
        ld      a,b
        or      c
        jr      z,wcsp_done
        call    __wchar_is_delim        ; Stop as soon as the current wchar_t is rejected.
        jr      z,wcsp_done
        inc     hl
        inc     hl
        jr      wcsp_loop
wcsp_done:
        ex      de,hl                   ; DE = current
        pop     hl                      ; start
        ld      a,e
        sub     l
        ld      e,a
        ld      a,d
        sbc     h
        ld      d,a                     ; DE = bytes
        srl     d
        rr      e                       ; Convert the byte delta to wchar_t count.
        ret
