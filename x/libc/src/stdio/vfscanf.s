        ;; vfscanf.s
        ;;
        ;; Public vfscanf() wrapper around the shared scanning core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module vfscanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _vfscanf
        .globl  __stdio_scan_init_stream
        .globl  __stdio_scan_core

SCAN_FMT_LO     .equ -99
SCAN_FMT_HI     .equ -98
SCAN_AP_LO      .equ -97
SCAN_AP_HI      .equ -96

        .area   _CODE

_vfscanf::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        push    de
        ld      hl,#-107
        add     hl,sp
        ld      sp,hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      SCAN_FMT_LO(ix),l
        ld      SCAN_FMT_HI(ix),h
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    __stdio_scan_init_stream
        ld      l,4(ix)
        ld      h,5(ix)
        ld      SCAN_AP_LO(ix),l
        ld      SCAN_AP_HI(ix),h
        call    __stdio_scan_core
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret
