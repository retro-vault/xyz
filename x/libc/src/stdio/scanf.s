        ;; scanf.s
        ;;
        ;; Public scanf() wrapper. Variadic stdio entry points are forced to the
        ;; stack-only ABI, so the first unnamed argument lives at ix+6.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module scanf
        .optsdcc -mz80 sdcccall(0)

        .globl  _scanf
        .globl  __stdio_scan_init_stdin
        .globl  __stdio_scan_core

SCAN_CTX_BYTES  .equ 107
SCAN_FMT_LO     .equ -99
SCAN_FMT_HI     .equ -98
SCAN_AP_LO      .equ -97
SCAN_AP_HI      .equ -96

        .area   _CODE

_scanf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-107
        add     hl,sp
        ld      sp,hl
        call    __stdio_scan_init_stdin
        ld      l,4(ix)
        ld      h,5(ix)
        ld      SCAN_FMT_LO(ix),l
        ld      SCAN_FMT_HI(ix),h
        push    ix
        pop     hl
        ld      de,#0x0006
        add     hl,de
        ld      SCAN_AP_LO(ix),l
        ld      SCAN_AP_HI(ix),h
        call    __stdio_scan_core
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret
