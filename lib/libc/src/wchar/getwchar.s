        ;; getwchar.s
        ;;
        ;; Single-byte console input maps directly onto the current wide-char
        ;; model, so getwchar is just getchar with a wider return type.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module getwchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _getwchar
        .globl  _getchar

        .area   _CODE

_getwchar::
        jp      _getchar
