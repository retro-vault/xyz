        ;; moremathd.s
        ;;
        ;; Double / long double wrappers for the additional non-transcendental
        ;; math entry points.  The current libc still computes these through the
        ;; proven single-precision kernels, then converts back through the
        ;; 64-bit double runtime so programs link and run correctly.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih




        .module moremathd
        .optsdcc -mz80 sdcccall(1)

        .globl  _rint
        .globl  _rintl
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  _rintf

        .area   _CODE
_rint::
_rintl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        call    _rintf
        call    ___fs2db
        pop     ix
        ret

