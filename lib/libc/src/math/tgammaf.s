        ;; tgammaf.s
        ;;
        ;; Public tgammaf() entry point for the shared gamma core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module tgammaf
        .optsdcc -mz80 sdcccall(1)

        .globl  _tgammaf
        .globl  __libc_tgammaf_core

        .area   _CODE

_tgammaf::
        jp      __libc_tgammaf_core
