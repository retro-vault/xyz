        ;; strtold.s
        ;;
        ;; long double is currently the same ABI and representation as double
        ;; on this target, so strtold can tail-call the shared double parser.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module strtold
        .optsdcc -mz80 sdcccall(1)

        .globl  _strtold
        .globl  __strtod_core

        .area   _CODE

_strtold::
        jp      __strtod_core
