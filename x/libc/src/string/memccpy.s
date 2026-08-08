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
        jr      z,memccpy_found_restore
        ex      af,af'
        inc     de
        jr      memccpy_loop
memccpy_found_restore:
        ex      af,af'
        inc     de
memccpy_found:
        pop     ix
        jr      memccpy_return
memccpy_not_found:
        pop     ix
        ld      de,#0x0000
memccpy_return:
        ; Both trailing arguments are spilled as words.  sdcccall(1) returns
        ; of at most 16 bits are callee-clean, so discard them while keeping
        ; the pointer result in DE intact.
        pop     hl                      ; return address
        pop     bc                      ; delimiter byte (word slot)
        pop     bc                      ; byte count
        jp      (hl)
