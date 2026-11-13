        ;; cpm_load.inc
        ;;
        ;; Private status values and fixed Partner P CP/M 3 disk parameters.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-10   tstih

        ;; Underlying libfloppy errors are returned as
        ;; LOADER_CPM_ERR_FLOPPY_BASE + FLOPPY_ERR_*.
        .equ    LOADER_CPM_OK,                    0
        .equ    LOADER_CPM_ERR_NOT_FOUND,         1
        .equ    LOADER_CPM_ERR_CORRUPT,           2
        .equ    LOADER_CPM_ERR_ARGUMENT,          3
        .equ    LOADER_CPM_ERR_FLOPPY_BASE,       0x10

        ;; Partner P CP/M disk parameter block represented in physical sectors.
        ;; CP/M sees the two heads as 154 logical tracks.
        .equ    LOADER_CPM_BOOT_SECTORS,          36
        .equ    LOADER_CPM_DIRECTORY_LBA,         36
        .equ    LOADER_CPM_DIRECTORY_SECTORS,     16
        .equ    LOADER_CPM_ENTRIES_PER_SECTOR,    8
        .equ    LOADER_CPM_ENTRY_SIZE,            32
        .equ    LOADER_CPM_BLOCK_SECTORS,         8
        .equ    LOADER_CPM_TOTAL_BLOCKS,          342

        ;; One 256-byte directory-sector buffer outside the payload and loader.
        .equ    LOADER_CPM_DIRECTORY_BUFFER,      0xfe00

        ;; Directory-entry offsets.
        .equ    LOADER_CPM_DIRENT_USER,            0
        .equ    LOADER_CPM_DIRENT_NAME,            1
        .equ    LOADER_CPM_DIRENT_EXTENT_LOW,     12
        .equ    LOADER_CPM_DIRENT_BYTE_COUNT,     13
        .equ    LOADER_CPM_DIRENT_EXTENT_HIGH,    14
        .equ    LOADER_CPM_DIRENT_RECORD_COUNT,   15
        .equ    LOADER_CPM_DIRENT_BLOCKS,         16
