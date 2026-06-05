        ; strrchr.s
        ;
        ; libc strrchr implementation for the xcc Z80 libc.
        ; Tracks the most recent match while scanning to the end of the string.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strrchr
        .optsdcc -mz80 sdcccall(1)


        .globl  _strrchr
        .globl  __string_return_bc

        .area   _CODE

        ; _strrchr
        ; inputs:
        ;   HL = string pointer
        ;   DE = search byte (low byte E is used)
        ; outputs:
        ;   DE = pointer to the last matching byte, or 0
        ; clobbers: AF, BC, HL
_strrchr::
        ld      bc,#0x0000               ; BC holds the last seen match
strrchr_loop:
        ld      a,(hl)
        cp      e
        jr      nz,strrchr_next
        ld      b,h
        ld      c,l
strrchr_next:
        ld      a,(hl)
        or      a
        jr      z,strrchr_done
        inc     hl
        jr      strrchr_loop
strrchr_done:
        jp      __string_return_bc
