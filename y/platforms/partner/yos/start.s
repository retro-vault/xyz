        ;; ALTO baremetal entry, reached through the standard-ROM floppy loader.
        .module partner_yos_start
        .globl _entry, _main, _memory_init, _cpmfs_init, __partner_bank_init
        .globl __partner_initial_data, s__FIXED_CODE, l__FIXED_CODE, s__CODE, l__CODE, l__DATA, s__BSS, l__BSS
        .globl s__KERNEL_BSS, l__KERNEL_BSS
        .globl __partner_bank_calls_ready
        .globl _smalloc, _interrupt_init
        .globl __partner_keyboard_init
        .area _CODE
_entry::
        di
        ld sp,#0xfe00
        ld hl,#0xc000
        ld de,#0xc001
        ld bc,#0x300-1
        ld (hl),#0
        ldir
        ld hl,#s__BSS
        ld de,#s__BSS+1
        ld bc,#l__BSS-1
        ld (hl),#0
        ldir
        ld hl,#l__FIXED_CODE
        ld bc,#l__DATA
        add hl,bc
        ld b,h
        ld c,l
        ld hl,#__partner_initial_data
        ld de,#s__FIXED_CODE
        ld a,b
        or c
        jr z,.initialized
        ldir
.initialized:
        ; The packed shared initializer is now reclaimable in the kernel bank.
        ld hl,#s__KERNEL_BSS
        ld de,#s__KERNEL_BSS+1
        ld bc,#l__KERNEL_BSS-1
        ld (hl),#0
        ldir
        ld hl,#s__KERNEL_BSS
        ld de,#l__KERNEL_BSS
        add hl,de
        inc hl
        res 0,l
        ld (0xfe04),hl
        ld hl,#0x0100
        ld (0xfe06),hl
        ; Replace every loader ISR before reclaiming its F600..FBFF code.
        call _interrupt_init
        call _cpmfs_init
        ld a,d
        or e
        jr nz,.halt
        call __partner_bank_init
        ld hl,#1024
        call _smalloc
        ld a,d
        or e
        jr z,.halt
        ld hl,#1024
        add hl,de
        ld sp,hl
        ld a,#1
        ld (__partner_bank_calls_ready),a
        call __partner_keyboard_init
        ei
        call _main
.halt:
        di
        halt
        jr .halt
