        ; A=logical SRAM page, FF=fixed memory. Preserve BC/DE/HL/IX/IY.
        ; The low firmware shadow keeps YOS RST gates visible at all times.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        ;
        .module divmmc_map
        .optsdcc -mz80 sdcccall(1)
        .globl __divmmc_map, __bank_current
        .area _CODE
__divmmc_map::
        ld (__bank_current),a
        cp #0xff
        ret z
        add a,#5
        or #0x40
        out (0xe3),a
        ret
