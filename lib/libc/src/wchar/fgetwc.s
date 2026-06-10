        ;; fgetwc.s
        ;;
        ;; The current execution character set is single-byte, so the narrow
        ;; stdio byte fetch already yields the correct wide code unit.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fgetwc
        .optsdcc -mz80 sdcccall(1)

        .globl  _fgetwc
        .globl  _fgetc

        .area   _CODE

_fgetwc::
        jp      _fgetc
