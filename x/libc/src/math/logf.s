        ;; logf.s
        ;;
        ;; Public logf() entry point for the shared transcendental float core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module logf
        .optsdcc -mz80 sdcccall(1)

        .globl  _logf
        .globl  __libc_logf_core

        .area   _CODE

_logf::
        jp      __libc_logf_core
