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

        ;; __wchar_is_delim
        ;; Scan a NUL-terminated wchar_t set for the 16-bit character in BC.
        ;; The helper reports membership through Z so callers can branch without
        ;; materializing a separate boolean result.
__wchar_is_delim::
        push    de
        push    hl
wid_loop:
        ld      a,(de)
        ld      l,a
        inc     de
        ld      a,(de)
        ld      h,a
        inc     de                      ; HL = current delimiter entry
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
        xor     a                       ; Return Z for "found in delimiter set".
        ret
wid_nf:
        pop     hl
        pop     de
        ld      a,#1
        or      a                       ; Return NZ for "not found".
        ret
