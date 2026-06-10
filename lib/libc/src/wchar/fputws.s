        ;; fputws.s
        ;;
        ;; Write a wide string through the narrow stdio layer by narrowing each
        ;; code unit with wctob. Any unrepresentable code unit or failed byte
        ;; write aborts the whole call with EOF.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fputws
        .optsdcc -mz80 sdcccall(1)

        .globl  _fputws
        .globl  _wctob
        .globl  _fputc

        .area   _DATA
__fputws_ptr:
        .dw     0
__fputws_stream:
        .dw     0

        .area   _CODE

_fputws::
        ld      (__fputws_ptr),hl
        ld      (__fputws_stream),de
        ld      a,h
        or      l
        jr      z,__fputws_fail
__fputws_loop:
        ld      hl,(__fputws_ptr)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      (__fputws_ptr),hl
        ld      a,d
        or      e
        jr      z,__fputws_ok
        ex      de,hl
        call    _wctob
        ld      a,d
        cp      #0xff
        jr      nz,__fputws_emit
        ld      a,e
        cp      #0xff
        jr      z,__fputws_fail
__fputws_emit:
        ex      de,hl
        ld      de,(__fputws_stream)
        call    _fputc
        ld      a,d
        cp      #0xff
        jr      nz,__fputws_loop
        ld      a,e
        cp      #0xff
        jr      z,__fputws_fail
        jr      __fputws_loop
__fputws_ok:
        ld      de,#0x0000
        ret
__fputws_fail:
        ld      de,#0xffff
        ret
