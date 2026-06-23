        ; memccpy.s
        ;
        ; libc memccpy implementation for the xcc Z80 libc.
        ; Copies forward until either the delimiter byte is copied or the byte
        ; count is exhausted.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memccpy
        .optsdcc -mz80 sdcccall(1)


        .globl  _memccpy
        .globl  __string_return_zero

        .area   _CODE

        ; _memccpy
        ; inputs:
        ;   HL         = destination
        ;   DE         = source
        ;   4(ix)      = delimiter byte
        ;   6(ix)..7(ix) = byte count
        ; outputs:
        ;   DE = pointer one past the copied delimiter, or 0 if not found
        ; clobbers: AF, BC, HL, IX
        ; notes:
        ;   The delimiter byte is parked in AF' so A can still be used by LDI
        ;   and the compare sequence inside the copy loop.
_memccpy::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,4(ix)                 ; delimiter byte
        ex      af,af'                  ; stash it in the alternate accumulator
        ld      c,6(ix)
        ld      b,7(ix)
        ex      de,hl                   ; HL = source, DE = destination
memccpy_loop:
        ld      a,b
        or      c
        jr      z,memccpy_not_found
        ldi                             ; copy one byte and advance both ends
        dec     de
        ex      af,af'
        ex      de,hl
        cp      (hl)                    ; compare copied byte with delimiter
        ex      de,hl
        ex      af,af'
        inc     de
        jr      z,memccpy_found
        jr      memccpy_loop
memccpy_found:
        pop     ix
        ret
memccpy_not_found:
        pop     ix
        jp      __string_return_zero
