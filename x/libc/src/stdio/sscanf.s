        ;; sscanf.s
        ;;
        ;; Public sscanf() wrapper around the shared scanning core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module sscanf
        .optsdcc -mz80 sdcccall(0)

        .globl  _sscanf
        .globl  __stdio_scan_init_string
        .globl  __stdio_scan_core

SCAN_FMT_LO     .equ -99
SCAN_FMT_HI     .equ -98
SCAN_AP_LO      .equ -97
SCAN_AP_HI      .equ -96

        .area   _CODE

_sscanf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-107
        add     hl,sp
        ld      sp,hl
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_scan_init_string
        ld      l,6(ix)
        ld      h,7(ix)
        ld      SCAN_FMT_LO(ix),l
        ld      SCAN_FMT_HI(ix),h
        push    ix
        pop     hl
        ld      de,#0x0008
        add     hl,de
        ld      SCAN_AP_LO(ix),l
        ld      SCAN_AP_HI(ix),h
        call    __stdio_scan_core
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret
