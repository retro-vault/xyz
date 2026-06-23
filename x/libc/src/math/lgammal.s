        ;; lgammal.s
        ;;
        ;; long double lgammal() wrapper through the single-precision
        ;; lgammaf() kernel.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module lgammal
        .optsdcc -mz80 sdcccall(1)

        .globl  _lgammal
        .globl  __db1arg_load_arg0_fs
        .globl  _lgammaf
        .globl  ___fs2db

        .area   _CODE

_lgammal::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db1arg_load_arg0_fs
        call    _lgammaf
        call    ___fs2db
        pop     ix
        ret
