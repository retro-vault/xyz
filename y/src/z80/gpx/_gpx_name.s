        ; Immutable name for the YOS graphics service.
        ;
        ; GPL-2.0 License (see: LICENSE.libgpx)
        ; Copyright (C) 2026 tomaz stih

        .module _gpx_name
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_name

        ; Installed in the free ROM bytes before the 3D00h divIDE range.
        ; See y/scripts/patch_rom.py.
__gpx_name = 0x3ce1
