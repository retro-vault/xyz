        ;; getchar.s  (sys backend: ZX Spectrum ROM image)
        ;;
        ;; Input is not wired up yet for the generic ROM backend, so the hook
        ;; behaves like an empty input stream.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _getchar
        .globl  _getchar

        .area   _CODE

_getchar:
_getchar::
        ld      de,#0xffff
        ret
