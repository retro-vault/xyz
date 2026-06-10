        ;; log1pf.s
        ;;
        ;; Computes log(1 + x) by forming 1+x and feeding the shared log core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module log1pf
        .optsdcc -mz80 sdcccall(1)

        .globl  _log1pf
        .globl  ___fsadd
        .globl  __libc_logf_core

        .area   _DATA
__log1pf_x:
        .ds     4

        .area   _CODE

_log1pf::
        ld      (__log1pf_x),de
        ld      (__log1pf_x + 2),hl
        ld      hl,#0x3f80              ; 1.0
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__log1pf_x)
        ld      hl,(__log1pf_x + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        jp      __libc_logf_core
