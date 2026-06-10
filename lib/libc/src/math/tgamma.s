        ;; tgamma.s
        ;;
        ;; double tgamma() wrapper through the single-precision tgammaf()
        ;; kernel.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module tgamma
        .optsdcc -mz80 sdcccall(1)

        .globl  _tgamma
        .globl  __db1arg_load_arg0_fs
        .globl  _tgammaf
        .globl  ___fs2db

        .area   _CODE

_tgamma::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db1arg_load_arg0_fs
        call    _tgammaf
        call    ___fs2db
        pop     ix
        ret
