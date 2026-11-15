        ; Validated XPRG carrier for the fixed RAM paging kernel.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module divmmc_image
        .optsdcc -mz80 sdcccall(1)
        .globl _entry, __divmmc_probe
        .area _CODE
        ; The loader has checked the complete image CRC before publication.
        ; Boot owns the empty 48K heap; code/source end below C000h. No return.
_entry::
        di
        call __divmmc_probe
        ld a,b
        ld (0x5efd),a
        ld hl,#.image
        ld de,#0xc000
        ld bc,#16384
        ldir
        jp 0xc000
.image:
        .include "divmmc_kernel.inc"
