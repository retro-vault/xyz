        ; stpncpy.s
        ;
        ; libc stpncpy implementation for the xcc Z80 libc.
        ; Copies at most n bytes from src to dest, NUL-padding any remainder,
        ; and returns a pointer to the first written NUL or to dest+n if the
        ; source was not shorter than n (POSIX extension).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module stpncpy
        .optsdcc -mz80 sdcccall(1)


        .globl  _stpncpy
        .globl  __string_return_hl

        .area   _CODE

        ; _stpncpy
        ; inputs:  HL = destination, DE = source, 4(ix)..5(ix) = n
        ; outputs: DE = dest + strnlen(src, n)
        ; clobbers: AF, BC, HL, IX
_stpncpy::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
stpncpy_loop:
        ld      a,b
        or      c
        jr      z,stpncpy_end           ; n bytes written
        dec     bc
        ld      a,(de)
        ld      (hl),a
        or      a
        jr      z,stpncpy_pad           ; copied NUL: HL points at it
        inc     hl
        inc     de
        jr      stpncpy_loop
stpncpy_pad:
        ; HL points at the first NUL written; this is the return value.
        ; Pad the rest of the field with NUL but keep HL as the result.
        push    hl                      ; save return pointer
stpncpy_pad_loop:
        ld      a,b
        or      c
        jr      z,stpncpy_pad_done
        dec     bc
        inc     hl
        ld      (hl),#0x00
        jr      stpncpy_pad_loop
stpncpy_pad_done:
        pop     hl                      ; restore return pointer (first NUL)
stpncpy_end:
        pop     ix
        jp      __string_return_hl
