        ; strncpy.s
        ;
        ; libc strncpy implementation for the xcc Z80 libc.
        ; Copies up to N bytes from the source, then pads the remainder with
        ; zero bytes if the source terminates early.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strncpy
        .optsdcc -mz80 sdcccall(1)


        .globl  _strncpy
        .globl  __string_ret_clean2

        .area   _CODE

        ; _strncpy
        ; inputs:
        ;   HL         = destination
        ;   DE         = source
        ;   4(ix)..5(ix) = byte limit
        ; outputs:
        ;   DE = original destination
        ; clobbers: AF, BC, HL, IX
_strncpy::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; preserve return pointer
        ld      c,4(ix)
        ld      b,5(ix)
        ex      de,hl                   ; HL = source, DE = destination
strncpy_copy:
        ld      a,b
        or      c
        jr      z,strncpy_done
        ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     de
        dec     bc
        or      a
        jr      z,strncpy_pad           ; source ended, pad the tail with NULs
        jr      strncpy_copy
strncpy_pad:
        ld      a,b
        or      c
        jr      z,strncpy_done
        xor     a
        ld      (de),a
        inc     de
        dec     bc
        jr      strncpy_pad
strncpy_done:
        pop     de
        pop     ix
        jp      __string_ret_clean2
