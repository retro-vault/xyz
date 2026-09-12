        ;; ZX Spectrum display width, independent of application context.
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module gpx_width
        .optsdcc -mz80 sdcccall(1)
        .globl  _gpx_width
        .area   _CODE

        ;; No arguments; DE = width in pixels. Clobbers DE only.
_gpx_width::
        ld      de,#256
        ret
