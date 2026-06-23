        ; strchrnul.s
        ;
        ; libc strchrnul implementation for the xcc Z80 libc.
        ; Like strchr, but on no match returns a pointer to the terminating
        ; NUL instead of NULL (GNU extension).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strchrnul
        .optsdcc -mz80 sdcccall(1)


        .globl  _strchrnul
        .globl  __string_return_hl

        .area   _CODE

        ; _strchrnul
        ; inputs:  HL = string pointer, DE = search byte (E)
        ; outputs: DE = pointer to first match, or to the terminating NUL
        ; clobbers: AF, HL
_strchrnul::
strchrnul_loop:
        ld      a,(hl)
        cp      e
        jr      z,strchrnul_done
        or      a
        jr      z,strchrnul_done        ; stop at NUL (and return its address)
        inc     hl
        jr      strchrnul_loop
strchrnul_done:
        jp      __string_return_hl
