        ;; read.s  (sys backend: generic z80)
        ;;
        ;; No default input device is available, so reads report EOF.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read

        .area   _CODE

_read::
        ld      de,#0x0000
        ret
