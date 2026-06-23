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

        .area   _CODE

FPWS_PTR    .equ 0
FPWS_STREAM .equ 2

_fputws::
        push    ix
        push    de
        push    hl
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      l
        jr      z,__fputws_fail
__fputws_loop:
        ld      l,FPWS_PTR(ix)
        ld      h,FPWS_PTR+1(ix)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      FPWS_PTR(ix),l
        ld      FPWS_PTR+1(ix),h
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
        ld      e,FPWS_STREAM(ix)
        ld      d,FPWS_STREAM+1(ix)
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
        ld      sp,ix
        pop     hl
        pop     bc
        pop     ix
        ret
__fputws_fail:
        ld      de,#0xffff
        ld      sp,ix
        pop     hl
        pop     bc
        pop     ix
        ret
