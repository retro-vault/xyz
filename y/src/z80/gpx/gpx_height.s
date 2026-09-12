        ;; ZX Spectrum display height, independent of application context.
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module gpx_height
        .optsdcc -mz80 sdcccall(1)
        .globl  _gpx_height
        .area   _CODE

        ;; No arguments; DE = height in pixels. Clobbers DE only.
_gpx_height::
        ld      de,#192
        ret
