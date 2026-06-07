        ; swab.s
        ;
        ; libc swab implementation for the xcc Z80 libc.
        ; Copies n bytes from src to dest, swapping each adjacent byte pair
        ; (POSIX).  An odd trailing byte is ignored, as the standard allows.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module swab
        .optsdcc -mz80 sdcccall(1)


        .globl  _swab

        .area   _CODE

        ; _swab
        ; inputs:  HL = source, DE = destination, 4(ix)..5(ix) = byte count
        ; outputs: none
        ; clobbers: AF, BC, DE, HL, IX
_swab::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        srl     b                       ; BC = number of byte pairs (count / 2)
        rr      c
swab_loop:
        ld      a,b
        or      c
        jr      z,swab_done
        dec     bc
        ld      a,(hl)                  ; low byte of the pair
        inc     hl
        push    af
        ld      a,(hl)                  ; high byte of the pair
        inc     hl
        ld      (de),a                  ; store swapped: high first
        inc     de
        pop     af
        ld      (de),a
        inc     de
        jr      swab_loop
swab_done:
        pop     ix
        ret
