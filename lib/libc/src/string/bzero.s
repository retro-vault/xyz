        ; bzero.s
        ;
        ; libc bzero implementation for the xcc Z80 libc.
        ; Legacy BSD helper that fills a span with zero bytes.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module bzero
        .optsdcc -mz80 sdcccall(1)


        .globl  _bzero

        .area   _CODE

        ; _bzero
        ; inputs:  HL = span pointer, DE = byte count
        ; outputs: none
        ; clobbers: AF, DE, HL
_bzero::
bzero_loop:
        ld      a,d
        or      e
        ret     z                       ; count exhausted
        ld      (hl),#0x00
        inc     hl
        dec     de
        jr      bzero_loop
