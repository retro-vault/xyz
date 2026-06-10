        ;; sscanf.s
        ;;
        ;; Public sscanf() wrapper around the shared scanning core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module sscanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _sscanf
        .globl  __stdio_scan_fmt
        .globl  __stdio_scan_ap
        .globl  __stdio_scan_init_string
        .globl  __stdio_scan_core

        .area   _CODE

_sscanf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_scan_init_string
        ld      l,6(ix)
        ld      h,7(ix)
        ld      (__stdio_scan_fmt),hl
        push    ix
        pop     hl
        ld      de,#0x0008
        add     hl,de
        ld      (__stdio_scan_ap),hl
        call    __stdio_scan_core
        push    hl
        pop     de
        pop     ix
        ret
