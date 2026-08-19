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
        .globl  _strtof

        .area   _CODE

_atof::
        .if     __XCC_LIBC_DOUBLE
        ld      de,#0
        jp      __strtod_core
        .else
        ld      de,#0
        jp      _strtof
        .endif
