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

        .area   _CODE

_log2f::
        call    __libc_logf_core
        ld      hl,#0x3fb8              ; 1 / ln(2)
        push    hl
        ld      hl,#0xaa3b
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        ret
