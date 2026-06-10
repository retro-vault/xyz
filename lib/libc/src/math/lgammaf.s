        ;; lgammaf.s
        ;;
        ;; Public lgammaf() entry point for the shared gamma core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module lgammaf
        .optsdcc -mz80 sdcccall(1)

        .globl  _lgammaf
        .globl  __libc_lgammaf_core

        .area   _CODE

_lgammaf::
        jp      __libc_lgammaf_core
