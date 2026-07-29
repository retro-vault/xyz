        ; strlcat.s
        ;
        ; libc strlcat implementation for the xcc Z80 libc.
        ; Appends src to a size-bounded dest string, always NUL terminating,
        ; and returns strnlen(dest,size) + strlen(src) (BSD).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strlcat
        .optsdcc -mz80 sdcccall(1)


        .globl  _strlcat

        .area   _CODE

        ; _strlcat
        ; inputs:  HL = destination, DE = source, 4(ix)..5(ix) = dest size
        ; outputs: DE = strnlen(dest,size) + strlen(source)
        ; clobbers: AF, BC, DE, HL, IX
        ; locals (relative to frame ix):
        ;   -2(ix) = dest base, -4(ix) = src base, -6(ix) = dlen0
_strlcat::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; -2(ix) = dest base
        push    de                      ; -4(ix) = src base
        ; --- dlen0 = strnlen(dest, size); HL ends at insertion point ---
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = size (becomes remaining room)
        ld      de,#0                   ; DE = dlen0
strlcat_dl:
        ld      a,b
        or      c
        jr      z,strlcat_dldone        ; size exhausted: no terminator/room
        ld      a,(hl)
        or      a
        jr      z,strlcat_dldone
        inc     hl
        inc     de
        dec     bc
        jr      strlcat_dl
strlcat_dldone:
        push    de                      ; -6(ix) = dlen0
        ; --- copy src into HL, bounded by remaining room BC ---
        ld      e,-4(ix)
        ld      d,-3(ix)                ; DE = src base
strlcat_cl:
        ld      a,b
        or      c
        jr      z,strlcat_cdone         ; no room left
        dec     bc
        ld      a,b
        or      c
        jr      z,strlcat_cterm         ; only the NUL slot remains
        ld      a,(de)
        ld      (hl),a
        or      a
        jr      z,strlcat_cdone         ; copied the source NUL
        inc     hl
        inc     de
        jr      strlcat_cl
strlcat_cterm:
        ld      (hl),#0x00
strlcat_cdone:
        ; --- result = dlen0 + strlen(src) ---
        ld      e,-4(ix)
        ld      d,-3(ix)                ; DE = src base
        ld      hl,#0                   ; HL = slen
strlcat_sl:
        ld      a,(de)
        or      a
        jr      z,strlcat_sldone
        inc     de
        inc     hl
        jr      strlcat_sl
strlcat_sldone:
        ld      c,-6(ix)
        ld      b,-5(ix)                ; BC = dlen0
        add     hl,bc                   ; HL = dlen0 + slen
        ex      de,hl                   ; DE = result
        ld      sp,ix                   ; discard the three locals
        pop     ix
        ret
