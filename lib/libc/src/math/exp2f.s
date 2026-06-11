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

        .area   _CODE

_exp2f::
        ld      hl,#0x3f31              ; ln(2)
        push    hl
        ld      hl,#0x7218
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        jp      __libc_expf_core
