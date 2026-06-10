        ;; vsscanf.s
        ;;
        ;; Public vsscanf() wrapper around the shared scanning core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module vsscanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _vsscanf
        .globl  __stdio_scan_fmt
        .globl  __stdio_scan_ap
        .globl  __stdio_scan_init_string
        .globl  __stdio_scan_core

        .area   _CODE

_vsscanf::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    de
        call    __stdio_scan_init_string
        pop     hl
        ld      (__stdio_scan_fmt),hl
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__stdio_scan_ap),hl
        call    __stdio_scan_core
        push    hl
        pop     de
        pop     ix
        ret
