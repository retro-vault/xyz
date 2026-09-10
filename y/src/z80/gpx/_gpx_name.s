        ; Immutable name for the YOS graphics service.
        ;
        ; GPL-2.0 License (see: LICENSE.libgpx)
        ; Copyright (C) 2026 tomaz stih

        .module _gpx_name
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_name

        .area   _CONST
__gpx_name::
        .asciz  "gpx"
