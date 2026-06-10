        ;; log10f.s
        ;;
        ;; Computes log10(x) as log(x) / ln(10).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module log10f
        .optsdcc -mz80 sdcccall(1)

        .globl  _log10f
        .globl  ___fsmul
        .globl  __libc_logf_core

        .area   _DATA
__log10f_x:
        .ds     4

        .area   _CODE

_log10f::
        call    __libc_logf_core
        ld      (__log10f_x),de
        ld      (__log10f_x + 2),hl
        ld      hl,#0x3ede              ; 1 / ln(10)
        push    hl
        ld      hl,#0x5bd9
        push    hl
        ld      de,(__log10f_x)
        ld      hl,(__log10f_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ret
