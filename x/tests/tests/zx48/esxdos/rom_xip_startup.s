        ; Exercise ROM constants, writable data and startup fragments.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rom_xip_startup
        .optsdcc -mz80 sdcccall(1)

        .globl  _zx_rom_pattern
        .globl  _zx_rom_initialized
        .globl  _zx_rom_init_hook

        .area   _CONST
_zx_rom_pattern::
        .db     0xa6,0x59
        .ds     10236
        .db     0x3c,0xc3

        .area   _INITIALIZER
        .dw     0xc3d2

        .area   _INITIALIZED
_zx_rom_initialized::
        .ds     2

        .area   _BSS
_zx_rom_init_hook::
        .ds     2

        .area   _GSINIT
        ld      hl,#0x6b19
        ld      (_zx_rom_init_hook),hl
