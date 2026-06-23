        ;; atanh.s
        ;;
        ;; Real double wrapper for atanh(). The current libc still routes
        ;; 64-bit double transcendental entry points through the proven float
        ;; kernels until dedicated double kernels exist.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module atanh
        .optsdcc -mz80 sdcccall(1)

        .globl  _atanh
        .globl  ___db2fs
        .globl  ___fs2db
        .globl  _atanhf

        .area   _CODE

__atanh_load_arg0_fs:
        ld      a,4(ix)
        ld      e,a
        ld      a,5(ix)
        ld      d,a
        ld      a,6(ix)
        ld      l,a
        ld      a,7(ix)
        ld      h,a
        exx
        ld      a,8(ix)
        ld      e,a
        ld      a,9(ix)
        ld      d,a
        ld      a,10(ix)
        ld      l,a
        ld      a,11(ix)
        ld      h,a
        exx
        jp      ___db2fs

_atanh::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __atanh_load_arg0_fs
        call    _atanhf
        call    ___fs2db
        pop     ix
        ret
