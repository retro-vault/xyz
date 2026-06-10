        ;; erfc.s
        ;;
        ;; double erfc() wrapper through the single-precision erfcf() kernel.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module erfc
        .optsdcc -mz80 sdcccall(1)

        .globl  _erfc
        .globl  __db1arg_load_arg0_fs
        .globl  _erfcf
        .globl  ___fs2db

        .area   _CODE

_erfc::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db1arg_load_arg0_fs
        call    _erfcf
        call    ___fs2db
        pop     ix
        ret
