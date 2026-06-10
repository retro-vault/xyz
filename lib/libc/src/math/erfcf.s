        ;; erfcf.s
        ;;
        ;; Public erfcf() entry point. This stays tiny by building on erff():
        ;;   erfc(x) = 1 - erf(x)
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module erfcf
        .optsdcc -mz80 sdcccall(1)

        .globl  _erfcf
        .globl  _erff
        .globl  ___fssub

        .area   _CODE

_erfcf::
        call    _erff
        push    hl
        push    de
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        ret
