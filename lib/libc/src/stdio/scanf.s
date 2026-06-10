        ;; scanf.s
        ;;
        ;; Public scanf() wrapper. Variadic stdio entry points are forced to the
        ;; stack-only ABI, so the first unnamed argument lives at ix+6.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module scanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _scanf
        .globl  __stdio_scan_fmt
        .globl  __stdio_scan_ap
        .globl  __stdio_scan_init_stdin
        .globl  __stdio_scan_core

        .area   _CODE

_scanf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_scan_init_stdin
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__stdio_scan_fmt),hl
        push    ix
        pop     hl
        ld      de,#0x0006
        add     hl,de
        ld      (__stdio_scan_ap),hl
        call    __stdio_scan_core
        push    hl
        pop     de
        pop     ix
        ret
