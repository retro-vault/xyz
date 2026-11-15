        ; Install an esxDOS firmware shadow without taking firmware RAM 0-4.
        ; Fixed RAM execution, interrupts disabled; clobbers AF/BC/DE/HL.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        ;
        .module divmmc_initialize
        .optsdcc -mz80 sdcccall(1)
        .globl __divmmc_initialize, __divmmc_resume, __divmmc_shadow
        .area _CODE
__divmmc_initialize::
        call __divmmc_shadow
        jp __divmmc_resume
__divmmc_shadow::
        ld a,#0x83
        out (0xe3),a
        ld hl,#0
        ld de,#0x2000
        ld bc,#0x2000
        ldir
        call 0x1ff8                    ; stock esxDOS 0.8.9 page-out RET
        ld a,#0x40
        out (0xe3),a
        ret
