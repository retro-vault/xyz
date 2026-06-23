        ; rawmemchr.s
        ;
        ; libc rawmemchr implementation for the xcc Z80 libc.
        ; Scans forward for a byte known to be present (GNU extension); no
        ; length bound, so the caller guarantees a match exists.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rawmemchr
        .optsdcc -mz80 sdcccall(1)


        .globl  _rawmemchr
        .globl  __string_return_hl

        .area   _CODE

        ; _rawmemchr
        ; inputs:  HL = span pointer, DE = search byte (E)
        ; outputs: DE = pointer to the first matching byte
        ; clobbers: AF, HL
_rawmemchr::
rawmemchr_loop:
        ld      a,(hl)
        cp      e
        jr      z,rawmemchr_done
        inc     hl
        jr      rawmemchr_loop
rawmemchr_done:
        jp      __string_return_hl
