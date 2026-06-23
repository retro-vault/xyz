        ;; erff.s
        ;;
        ;; Public erff() entry point for the shared error-function core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module erff
        .optsdcc -mz80 sdcccall(1)

        .globl  _erff
        .globl  __libc_erff_core

        .area   _CODE

_erff::
        jp      __libc_erff_core
