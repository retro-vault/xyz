        ;; strfroml.s
        ;;
        ;; long double strfroml() wrapper for the xcc Z80 libc.
        ;;
        ;; In this libc ABI, long double is the same runtime format and stack
        ;; layout as double, so the C23 formatter can tail-dispatch directly to
        ;; strfromd().
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module strfroml
        .optsdcc -mz80 sdcccall(1)

        .globl  _strfroml
        .globl  _strfromd

        .area   _CODE

_strfroml::
        jp      _strfromd
