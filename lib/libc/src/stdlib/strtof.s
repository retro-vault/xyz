        ;; strtof.s
        ;;
        ;; Public float parser wrapper. The shared core parses through the
        ;; double runtime first, then this wrapper narrows the result back to
        ;; float32.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module strtof
        .optsdcc -mz80 sdcccall(1)

        .globl  _strtof
        .globl  __strtod_core
        .globl  ___db2fs

        .area   _CODE

_strtof::
        call    __strtod_core
        jp      ___db2fs
