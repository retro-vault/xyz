        ;; atof.s
        ;;
        ;; atof is the null-endptr convenience wrapper over strtod.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module atof
        .optsdcc -mz80 sdcccall(1)

        .globl  _atof
        .globl  __strtod_core

        .area   _CODE

_atof::
        ld      de,#0
        jp      __strtod_core
