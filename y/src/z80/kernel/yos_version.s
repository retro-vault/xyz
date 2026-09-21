        ; Return the YOS API version.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module yos_version
        .optsdcc -mz80 sdcccall(1)
        .globl  _yos_version
        ; Use four of the free header-data bytes below the fixed 0100 entry.
        .area   _HEADER_DATA
_yos_version::
        ld      de, #3
        ret
