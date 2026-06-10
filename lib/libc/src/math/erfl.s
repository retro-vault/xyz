        ;; erfl.s
        ;;
        ;; long double erfl() wrapper through the single-precision erff()
        ;; kernel until a dedicated 64-bit implementation exists.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module erfl
        .optsdcc -mz80 sdcccall(1)

        .globl  _erfl
        .globl  __db1arg_load_arg0_fs
        .globl  _erff
        .globl  ___fs2db

        .area   _CODE

_erfl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db1arg_load_arg0_fs
        call    _erff
        call    ___fs2db
        pop     ix
        ret
