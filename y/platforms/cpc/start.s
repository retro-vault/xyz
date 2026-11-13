        ;; CPC entry after AMSDOS loads common RAM and the compressed kernel.
        .module cpc_desktop_start
        .globl _entry, _main, _memory_init, _cpmfs_init, __partner_bank_init
        .globl s__FIXED_CODE, l__FIXED_CODE, s__CODE, l__CODE, l__DATA, s__BSS, l__BSS
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
        ld bc,#0x50-1
        ld (hl),#0
        ldir
        ld hl,#s__BSS
        ld de,#s__BSS+1
        ld bc,#l__BSS-1
        ld (hl),#0
        ldir
.initialized:
        ; Graphics scratch follows the resident low-bank renderer.
        ld hl,#s__KERNEL_BSS
        ld de,#s__KERNEL_BSS+1
        ld bc,#l__KERNEL_BSS-1
        ld (hl),#0
        ldir
        ld hl,#s__CODE
        ld de,#l__CODE
        add hl,de
        inc hl
        res 0,l
        ld (0xfe04),hl
        ld hl,#0x0100
        ld (0xfe06),hl
        ; The shared allocator consumes this boot memory descriptor.
        ld hl,#0x4d42
        ld (0xfe00),hl
        ld hl,#0x4d45
        ld (0xfe02),hl
        ld a,#1
        ld (0xfe08),a
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
