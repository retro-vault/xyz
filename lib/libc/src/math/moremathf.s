        ;; moremathf.s
        ;;
        ;; Additional single-precision math entry points for the xcc Z80 libc.
        ;; These are the missing non-transcendental pieces that can be built
        ;; directly on top of the existing soft-float runtime and libc helpers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih




        .module moremathf
        .optsdcc -mz80 sdcccall(1)

        .globl  _rintf
        .globl  _roundf

        .area   _CODE
_rintf::
        jp      _roundf

