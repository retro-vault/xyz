        ; Choose expanded 48K boot without changing the application ABI.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module divmmc_start
        .optsdcc -mz80 sdcccall(1)
        .include "yos.inc"
        .globl _entry, __divmmc_probe
        .area _DATA
.yos: .dw 0
        .area _CODE
        ; Ordinary XPRG process. No arguments; returns DE=0 after scheduling
        ; the next image. Clobbers AF/BC/DE/HL; preserves IX/IY.
_entry::
        ld hl,#.yos_name
        rst 0x18
        ld (.yos),de
        ld bc,#YOS_OFFSET_ROM_MODEL
        call .yos_call
        ld a,e
        or a
        jr nz,.shell
        ld bc,#YOS_OFFSET_ENTER_CRITICAL_SECTION
        call .yos_call
        call __divmmc_probe
        push bc
        ld bc,#YOS_OFFSET_LEAVE_CRITICAL_SECTION
        call .yos_call
        pop bc
        ld a,b
        or a
        jr z,.shell
        ld hl,#.expanded
        ld bc,#YOS_OFFSET_LOAD_PROCESS
        call .yos_call
        ld a,d
        or e
        jr nz,.done
.shell:
        ld hl,#.shell_name
        ld bc,#YOS_OFFSET_LOAD_PROCESS
        call .yos_call
.done:
        ld de,#0
        ret
.yos_call:
        push hl
        ld hl,(.yos)
        add hl,bc
        ld c,(hl)
        inc hl
        ld b,(hl)
        pop hl
        push bc
        ret
.yos_name: .asciz "yos"
.expanded: .asciz "/ALTO/SYS/divmmc.sys"
.shell_name: .asciz "/ALTO/SYS/shell.sys"
