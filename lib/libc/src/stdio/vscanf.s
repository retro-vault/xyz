        ;; vscanf.s
        ;;
        ;; Public vscanf() wrapper around the shared scanning core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module vscanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _vscanf
        .globl  __stdio_scan_fmt
        .globl  __stdio_scan_ap
        .globl  __stdio_scan_init_stdin
        .globl  __stdio_scan_core

        .area   _CODE

_vscanf::
        push    de
        push    hl
        call    __stdio_scan_init_stdin
        pop     hl
        ld      (__stdio_scan_fmt),hl
        pop     hl
        ld      (__stdio_scan_ap),hl
        call    __stdio_scan_core
        push    hl
        pop     de
        ret
