        ;; expm1f.s
        ;;
        ;; Computes exp(x) - 1 with the shared expf() core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module expm1f
        .optsdcc -mz80 sdcccall(1)

        .globl  _expm1f
        .globl  ___fssub
        .globl  __libc_expf_core

        .area   _CODE

_expm1f::
        call    __libc_expf_core
        ld      hl,#0x3f80              ; 1.0
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fssub
        pop     bc
        pop     bc
        ret
