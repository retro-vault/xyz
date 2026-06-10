        ;; exp2f.s
        ;;
        ;; Computes 2^x as expf(x * ln(2)) using the shared transcendental core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module exp2f
        .optsdcc -mz80 sdcccall(1)

        .globl  _exp2f
        .globl  ___fsmul
        .globl  __libc_expf_core

        .area   _DATA
__exp2f_x:
        .ds     4

        .area   _CODE

_exp2f::
        ld      (__exp2f_x),de
        ld      (__exp2f_x + 2),hl
        ld      hl,#0x3f31              ; ln(2)
        push    hl
        ld      hl,#0x7218
        push    hl
        ld      de,(__exp2f_x)
        ld      hl,(__exp2f_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        jp      __libc_expf_core
