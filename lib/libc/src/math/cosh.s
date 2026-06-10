        ;; cosh.s
        ;;
        ;; Real double / long double wrappers for cosh(). The current libc still
        ;; routes 64-bit double entry points through the proven float kernels
        ;; until dedicated double transcendental kernels exist.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cosh
        .optsdcc -mz80 sdcccall(1)

        .globl  _cosh
        .globl  _coshl
        .globl  ___db2fs
        .globl  ___fs2db
        .globl  _coshf

        .area   _CODE

__cosh_load_arg0_fs:
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

_cosh::
_coshl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __cosh_load_arg0_fs
        call    _coshf
        call    ___fs2db
        pop     ix
        ret
