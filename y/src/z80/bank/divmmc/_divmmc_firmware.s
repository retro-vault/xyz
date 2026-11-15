        ; Exchange only firmware restart bytes around an esxDOS call.
        ; Both entries preserve all caller registers. Caller holds a critical
        ; section. The firmware owns its own bank latch while it is running.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        ;
        .module divmmc_firmware
        .optsdcc -mz80 sdcccall(1)
        .globl __divmmc_firmware, __divmmc_resume, __bank_current
        .globl __sys_vec_tbl, __divmmc_map
        .area _CODE
__divmmc_firmware::
        push af
        push bc
        push de
        push hl
        ld a,#0x83
        out (0xe3),a
        ld hl,#0
        ld de,#0x2000
        ld bc,#106
        ldir
        call 0x1ff8
        ld a,#0x40
        out (0xe3),a
        jr .restore
__divmmc_resume::
        push af
        push bc
        push de
        push hl
        ld a,#0x83
        out (0xe3),a
        ld a,#0xc3
        ld (0x2001),a                  ; warm reset restores the firmware shadow
        ld hl,#__divmmc_reset
        ld (0x2002),hl
        ld hl,#.vectors
        ld de,#0x2018
        ld b,#4
.vector:
        push bc
        ld bc,#3
        ldir
        ex de,hl
        ld bc,#5
        add hl,bc
        ex de,hl
        pop bc
        djnz .vector
        ld hl,#0xc9f1                  ; POP AF / RET after base-ROM PUSH AF
        ld (0x2039),hl
        ld hl,#0xf1f5                  ; NMI: PUSH AF, POP AF, RETN
        ld (0x2066),hl                  ; same delayed entry as the base ROM
        ld hl,#0x45ed
        ld (0x2068),hl
        call 0x1ff8                    ; normalize either firmware exit state
        ld a,#0x40
        out (0xe3),a
        rst 0x38                       ; automap the patched firmware shadow
        ld a,(__bank_current)
        call __divmmc_map
.restore:
        pop hl
        pop de
        pop bc
        pop af
        ret
.vectors:
        jp __sys_vec_tbl+6
        jp __sys_vec_tbl+9
        jp __sys_vec_tbl+12
        jp __sys_vec_tbl+15

        ; MAPRAM survives a warm reset. Restore the complete firmware image
        ; before letting esxDOS reinitialize and hand control to the base ROM.
        ; The CPU has just reset: no caller, valid stack or IFF is assumed.
        .globl __divmmc_reset
        .globl __divmmc_shadow
__divmmc_reset::
        di
        ld sp,#0x5eff
        call __divmmc_shadow
        jp 0
