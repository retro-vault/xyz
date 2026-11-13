        ;; manifest.inc
        ;;
        ;; Sector-zero boot-manifest layout consumed by the ROM-loaded loader.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-10   tstih

        .equ    MANIFEST_BASE,              0xe000
        .equ    MANIFEST_MAGIC,             (MANIFEST_BASE + 3)
        .equ    MANIFEST_VERSION,           (MANIFEST_BASE + 7)
        .equ    MANIFEST_FLAGS,             (MANIFEST_BASE + 8)
        .equ    MANIFEST_BANK1_DESCRIPTOR,  (MANIFEST_BASE + 9)
        .equ    MANIFEST_BANK2_DESCRIPTOR,  (MANIFEST_BASE + 26)
        .equ    MANIFEST_CHECKSUM,          (MANIFEST_BASE + 43)

        ;; Each packed descriptor is copied verbatim into loader state.
        .equ    MANIFEST_DESC_USER,          0
        .equ    MANIFEST_DESC_NAME,          1
        .equ    MANIFEST_DESC_SECTORS,      12
        .equ    MANIFEST_DESC_LOAD,         13
        .equ    MANIFEST_DESC_LOAD_HIGH,    14
        .equ    MANIFEST_DESC_ENTRY,        15
        .equ    MANIFEST_DESC_ENTRY_HIGH,   16
        .equ    MANIFEST_DESC_SIZE,         17

        .equ    MANIFEST_FLAG_BANK2,         0x01
        .equ    MANIFEST_FLAG_START_BANK2,   0x02
        .equ    MANIFEST_VALID_FLAGS,        0x03

        .equ    MANIFEST_CURRENT_VERSION,   3
        .equ    MANIFEST_CHECKSUM_FIRST,    3
        .equ    MANIFEST_CHECKSUM_LAST,     43
        .equ    MANIFEST_MAX_SECTORS,       192
