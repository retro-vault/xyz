        ;; partner.inc
        ;;
        ;; Iskra Delta Partner hardware and boot-memory constants shared by
        ;; the loader and bare-metal libraries.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-10   tstih

        ;; Memory banking.
        .equ    PARTNER_PORT_ROM_OFF,       0x80
        .equ    PARTNER_PORT_BANK_1,        0x88
        .equ    PARTNER_PORT_BANK_2,        0x90
        .equ    PARTNER_PORT_PIO_A_CTRL,    0xd1
        .equ    PARTNER_PORT_PIO_B_CTRL,    0xd3
        .equ    PARTNER_PIO_MODE_OUTPUT,    0x0f
        .equ    PARTNER_PIO_INT_DISABLE,    0x17
        .equ    PARTNER_PIO_INT_MASK_ALL,   0xff
        .equ    PARTNER_SHARED_BASE,         0xc000

        ;; Position-independent far-memory access code retained in the loader
        ;; region, above its ordinary code/state and below the stack reserve.
        .equ    PARTNER_FAR_TRAMPOLINE,       0xfc00
        .equ    PARTNER_FAR_GET,              PARTNER_FAR_TRAMPOLINE
        .equ    PARTNER_FAR_PUT,              (PARTNER_FAR_TRAMPOLINE + 6)
        .equ    PARTNER_BANK_LIMIT,           PARTNER_SHARED_BASE

        ;; Fixed common-memory handoff populated immediately before the loader
        ;; transfers control to a payload. It reuses the loader's now-idle
        ;; directory scratch page and remains above the payload stack reserve.
        .equ    PARTNER_MEMORY_INFO,          0xfe00
        .equ    PARTNER_MEMORY_INFO_MAGIC,    PARTNER_MEMORY_INFO
        .equ    PARTNER_MEMORY_INFO_BANK1_END,(PARTNER_MEMORY_INFO + 4)
        .equ    PARTNER_MEMORY_INFO_BANK2_END,(PARTNER_MEMORY_INFO + 6)
        .equ    PARTNER_MEMORY_INFO_BANK,     (PARTNER_MEMORY_INFO + 8)
        .equ    PARTNER_MEMORY_INFO_FLAGS,    (PARTNER_MEMORY_INFO + 9)

        ;; Standard ROM floppy hand-off.
        .equ    PARTNER_ROM_LOAD_BASE,      0xe000
        .equ    PARTNER_LOADER_BASE,        0xf600
        .equ    PARTNER_LOADER_DISK_LBA,    22
        .equ    PARTNER_LOADER_DISK_OFFSET, 0x1600
        .equ    PARTNER_LOADER_SLOT_SIZE,   0x0800

        ;; Runtime top-of-memory reserve.
        .equ    PARTNER_LOADER_STACK_TOP,   0xfe00
        .equ    PARTNER_IM2_BASE,           0xff00
        .equ    PARTNER_IM2_LAST,           0xffff

        ;; Classic Partner floppy geometry.
        .equ    PARTNER_FD_CYLINDERS,       77
        .equ    PARTNER_FD_HEADS,           2
        .equ    PARTNER_FD_SPT,             18
        .equ    PARTNER_FD_SECTOR_SIZE,     256
        .equ    PARTNER_FD_SECTORS,         2772
