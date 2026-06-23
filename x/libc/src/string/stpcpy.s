        ; stpcpy.s
        ;
        ; libc stpcpy implementation for the xcc Z80 libc.
        ; Copies the source string into the destination and returns a pointer
        ; to the terminating NUL of the destination (POSIX extension).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module stpcpy
        .optsdcc -mz80 sdcccall(1)


        .globl  _stpcpy

        .area   _CODE

        ; _stpcpy
        ; inputs:  HL = destination, DE = source
        ; outputs: DE = pointer to the terminating NUL in the destination
        ; clobbers: AF, HL
_stpcpy::
stpcpy_loop:
        ld      a,(de)
        ld      (hl),a
        or      a
        jr      z,stpcpy_done
        inc     hl
        inc     de
        jr      stpcpy_loop
stpcpy_done:
        ex      de,hl                   ; DE = address of the copied NUL
        ret
