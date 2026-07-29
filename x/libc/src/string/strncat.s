        ; strncat.s
        ;
        ; libc strncat implementation for the xcc Z80 libc.
        ; Appends up to N source bytes, then always writes a terminating NUL.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strncat
        .optsdcc -mz80 sdcccall(1)


        .globl  _strncat
        .globl  __string_scan_nul

        .area   _CODE

        ; _strncat
        ; inputs:
        ;   HL         = destination string
        ;   DE         = source string
        ;   4(ix)..5(ix) = maximum byte count to append
        ; outputs:
        ;   DE = original destination pointer
        ; clobbers: AF, BC, HL, IX
_strncat::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; preserve original destination
        push    de                      ; preserve source until end-of-dest scan
        call    __string_scan_nul
        pop     de
        ld      c,4(ix)
        ld      b,5(ix)
        ex      de,hl                   ; HL = source, DE = destination end
strncat_copy:
        ld      a,b
        or      c
        jr      z,strncat_term
        ld      a,(hl)
        or      a
        jr      z,strncat_term
        ld      (de),a
        inc     hl
        inc     de
        dec     bc
        jr      strncat_copy
strncat_term:
        xor     a
        ld      (de),a                  ; append the required trailing NUL
        pop     de
        pop     ix
        ret
