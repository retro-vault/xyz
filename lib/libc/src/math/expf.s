        ;; expf.s
        ;;
        ;; Public expf() entry point for the shared transcendental float core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module expf
        .optsdcc -mz80 sdcccall(1)

        .globl  _expf
        .globl  __libc_expf_core

        .area   _CODE

_expf::
        jp      __libc_expf_core
