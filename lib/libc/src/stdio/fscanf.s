        ;; fscanf.s
        ;;
        ;; Public fscanf() wrapper around the shared scanning core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fscanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fscanf
        .globl  __stdio_scan_fmt
        .globl  __stdio_scan_ap
        .globl  __stdio_scan_init_stream
        .globl  __stdio_scan_core

        .area   _CODE

_fscanf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_scan_init_stream
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
