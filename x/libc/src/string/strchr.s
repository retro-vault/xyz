        ; strchr.s
        ;
        ; libc strchr implementation for the xcc Z80 libc.
        ; Returns the first matching byte in the string, including support for
        ; searching for the terminating NUL itself.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strchr
        .optsdcc -mz80 sdcccall(1)


        .globl  _strchr
        .globl  __string_return_zero
        .globl  __string_return_hl

        .area   _CODE

        ; _strchr
        ; inputs:
        ;   HL = string pointer
        ;   DE = search byte (low byte E is used)
        ; outputs:
        ;   DE = pointer to the first matching byte, or 0
        ; clobbers: AF, HL
_strchr::
strchr_loop:
        ld      a,(hl)
        cp      e
        jr      z,strchr_found
        or      a
        jr      z,strchr_not_found
        inc     hl
        jr      strchr_loop
strchr_found:
        jp      __string_return_hl
strchr_not_found:
        jp      __string_return_zero
