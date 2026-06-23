        ;; erf.s
        ;;
        ;; double erf() wrapper through the single-precision erff() kernel.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module erf
        .optsdcc -mz80 sdcccall(1)

        .globl  _erf
        .globl  __db1arg_load_arg0_fs
        .globl  _erff
        .globl  ___fs2db

        .area   _CODE

_erf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db1arg_load_arg0_fs
        call    _erff
        call    ___fs2db
        pop     ix
        ret
