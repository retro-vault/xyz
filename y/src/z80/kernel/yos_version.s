        ; Return the YOS API version.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module yos_version
        .optsdcc -mz80 sdcccall(1)
        .globl  _yos_version
        ; Four bytes fit in the five-byte gap following RST 08h.
        .area   _CODE
_yos_version::
        ld      de, #9
        ret
