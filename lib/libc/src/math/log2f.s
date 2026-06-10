        ;; log2f.s
        ;;
        ;; Computes log2(x) as log(x) / ln(2).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module log2f
        .optsdcc -mz80 sdcccall(1)

        .globl  _log2f
        .globl  ___fsmul
        .globl  __libc_logf_core

        .area   _DATA
__log2f_x:
        .ds     4

        .area   _CODE

_log2f::
        call    __libc_logf_core
        ld      (__log2f_x),de
        ld      (__log2f_x + 2),hl
        ld      hl,#0x3fb8              ; 1 / ln(2)
        push    hl
        ld      hl,#0xaa3b
        push    hl
        ld      de,(__log2f_x)
        ld      hl,(__log2f_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ret
