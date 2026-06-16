        ;; legacymathd.s
        ;;
        ;; Real double / long double wrappers for the older non-transcendental
        ;; math entry points that historically aliased directly to the float
        ;; bodies. These wrappers convert through the existing single-precision
        ;; kernels until dedicated 64-bit double kernels exist.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih




        .module legacymathd
        .optsdcc -mz80 sdcccall(1)

        .globl  _fabs
        .globl  _fabsl
        .globl  __lgd_load_arg0_raw

        .area   _CODE
_fabs::
_fabsl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_raw
        exx
        res     7,h
        exx
        pop     ix
        ret

