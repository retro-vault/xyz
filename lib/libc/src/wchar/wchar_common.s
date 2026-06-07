        ; wchar_common.s
        ;
        ; Shared helper for the wide-string functions: membership test of a
        ; wide character in a NUL-terminated wide set.  wchar_t is 16-bit.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module wchar_common
        .optsdcc -mz80 sdcccall(1)
        .globl  __wchar_is_delim
        .area   _CODE

        ; __wchar_is_delim
        ; inputs:  BC = ch, DE = delim pointer
        ; outputs: Z set if ch is in delim, NZ otherwise
        ; preserves: BC, DE, HL   (clobbers AF)
__wchar_is_delim::
        push    de
        push    hl
wid_loop:
        ld      a,(de)
        ld      l,a
        inc     de
        ld      a,(de)
        ld      h,a
        inc     de                      ; HL = *delim
        ld      a,h
        or      l
        jr      z,wid_nf
        ld      a,l
        cp      c
        jr      nz,wid_loop
        ld      a,h
        cp      b
        jr      nz,wid_loop
        pop     hl
        pop     de
        xor     a                       ; Z = found
        ret
wid_nf:
        pop     hl
        pop     de
        ld      a,#1
        or      a                       ; NZ = not found
        ret
