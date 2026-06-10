        ;; strtod.s
        ;;
        ;; Public double parser wrapper around the shared decimal core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module strtod
        .optsdcc -mz80 sdcccall(1)

        .globl  _strtod
        .globl  __strtod_core

        .area   _CODE

_strtod::
        jp      __strtod_core
