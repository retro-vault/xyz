        ;; cpmfs.inc
        ;;
        ;; Internal layout and fixed Partner P CP/M filesystem constants.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .include "list.inc"

        .equ    CPMFS_MAGIC_VALUE,          0x5346
        .equ    CPMFS_STATE_MAGIC,          0xc040
        .equ    CPMFS_STATE_HEAD,           0xc042
        .equ    CPMFS_STATE_NEXT_FD,        0xc044
        .equ    CPMFS_STATE_DRIVE,          0xc046
        .equ    CPMFS_STATE_AREA,           0xc047
        .equ    CPMFS_STATE_ACTIVE_DRIVE,   0xc048
        .equ    CPMFS_STATE_ERRNO,          0xc049

        .equ    CPMFS_TMP_NODE,             0xc050
        .equ    CPMFS_TMP_LBA,              0xc052
        .equ    CPMFS_TMP_ENTRY,            0xc054
        .equ    CPMFS_TMP_TARGET_LBA,       0xc056
        .equ    CPMFS_TMP_EXTENT,           0xc058
        .equ    CPMFS_TMP_BLOCK,            0xc05a
        .equ    CPMFS_TMP_SECTOR,           0xc05c
        .equ    CPMFS_TMP_ENTRY_INDEX,      0xc05e
        .equ    CPMFS_TMP_TARGET_INDEX,     0xc05f
        .equ    CPMFS_TMP_BLOCK_SLOT,       0xc060
        .equ    CPMFS_TMP_IN_BLOCK,         0xc061
        .equ    CPMFS_TMP_CREATE,           0xc062
        .equ    CPMFS_TMP_FOUND,            0xc063
        .equ    CPMFS_TMP_DIRTY,            0xc064
        .equ    CPMFS_TMP_BEST_EXTENT,      0xc065
        .equ    CPMFS_TMP_SIZE,             0xc067
        .equ    CPMFS_TMP_VALUE,            0xc06b
        .equ    CPMFS_TMP_VALUE2,           0xc06f
        .equ    CPMFS_TMP_POINTER,          0xc073
        .equ    CPMFS_TMP_REMAINING,        0xc075
        .equ    CPMFS_TMP_TRANSFERRED,      0xc077
        .equ    CPMFS_TMP_CHUNK,            0xc079
        .equ    CPMFS_TMP_FLAGS,            0xc07b
        .equ    CPMFS_TMP_FD,               0xc07d
        .equ    CPMFS_TMP_PATH1,            0xc080
        .equ    CPMFS_TMP_PATH2,            0xc09a
        .equ    CPMFS_ALLOC_MAP,            0xc100
        .equ    CPMFS_ALLOC_MAP_SIZE,       306

        .equ    CPMFS_ERR_NONE,             0
        .equ    CPMFS_ERR_ARGUMENT,         1
        .equ    CPMFS_ERR_PATH,             2
        .equ    CPMFS_ERR_NOT_FOUND,        3
        .equ    CPMFS_ERR_EXISTS,           4
        .equ    CPMFS_ERR_MEMORY,           5
        .equ    CPMFS_ERR_ACCESS,           6
        .equ    CPMFS_ERR_IO,               7
        .equ    CPMFS_ERR_NOSPC,            8
        .equ    CPMFS_ERR_BAD_FD,           9
        .equ    CPMFS_ERR_RANGE,            10
        .equ    CPMFS_ERR_DEVICE,           11

        .equ    CPMFS_FD_DRIVES,            4
        .equ    CPMFS_HD_DRIVES,            2
        .equ    CPMFS_DEVICES,              6
        .equ    CPMFS_HD_BASE,              CPMFS_FD_DRIVES
        .equ    CPMFS_AREAS,                16
        .equ    CPMFS_FILENAME_MAX,         8
        .equ    CPMFS_EXTENSION_MAX,        3
        .equ    CPMFS_BOOT_SECTORS,         36
        .equ    CPMFS_DIRECTORY_LBA,        36
        .equ    CPMFS_DIRECTORY_SECTORS,    16
        .equ    CPMFS_ENTRIES_PER_SECTOR,   8
        .equ    CPMFS_DIRENT_SIZE,          32
        .equ    CPMFS_BLOCK_SECTORS,        8
        .equ    CPMFS_TOTAL_BLOCKS,         342

        .equ    DIRENT_USER,                0
        .equ    DIRENT_NAME,                1
        .equ    DIRENT_EX,                  12
        .equ    DIRENT_BC,                  13
        .equ    DIRENT_S2,                  14
        .equ    DIRENT_RC,                  15
        .equ    DIRENT_BLOCKS,              16
        .equ    DIRENT_DELETED,             0xe5

        .equ    PATH_DRIVE,                 0
        .equ    PATH_AREA,                  1
        .equ    PATH_FILENAME,              2
        .equ    PATH_EXTENSION,             11
        .equ    PATH_CANONICAL,             15
        .equ    PATH_SIZE,                  26

        .equ    FILE_NEXT,                  LIST_NODE_NEXT
        .equ    FILE_FD,                    2
        .equ    FILE_FLAGS,                 4
        .equ    FILE_DRIVE,                 6
        .equ    FILE_AREA,                  7
        .equ    FILE_NAME,                  8
        .equ    FILE_POSITION,              19
        .equ    FILE_SIZE_BYTES,            23
        .equ    FILE_CACHE_SECTOR,          27
        .equ    FILE_CACHE_LBA,             29
        .equ    FILE_CACHE_VALID,           31
        .equ    FILE_CACHE_DIRTY,           32
        .equ    FILE_DMA,                   33
        .equ    FILE_NODE_SIZE,             289

        .equ    DIRECTORY_MAGIC,            0
        .equ    DIRECTORY_DRIVE,            2
        .equ    DIRECTORY_AREA,             3
        .equ    DIRECTORY_LBA,              4
        .equ    DIRECTORY_INDEX,            6
        .equ    DIRECTORY_VALID,            7
        .equ    DIRECTORY_RESULT,           8
        .equ    DIRECTORY_RESULT_NAME,      8
        .equ    DIRECTORY_RESULT_DRIVE,     21
        .equ    DIRECTORY_RESULT_AREA,      22
        .equ    DIRECTORY_DMA,              24
        .equ    DIRECTORY_PUBLIC_ENTRY,     316
        .equ    DIRECTORY_SIZE,             595
        .equ    DIRECTORY_MAGIC_VALUE,      0x5244

        .equ    O_ACCMODE_VALUE,            0x0003
        .equ    O_WRONLY_VALUE,             0x0001
        .equ    O_RDWR_VALUE,               0x0002
        .equ    O_CREAT_VALUE,              0x0100
        .equ    O_TRUNC_VALUE,              0x0200
        .equ    O_APPEND_VALUE,             0x0400
        .equ    F_GETFL_VALUE,              3
        .equ    CPMFS_F_SYNC_VALUE,         0x4301

        .equ    SEEK_SET_VALUE,             0
        .equ    SEEK_CUR_VALUE,             1
        .equ    SEEK_END_VALUE,             2

        ;; Partner YOS extension: media geometry and a full HDD allocation map.
        .equ CPMFS_G_DIR,       0xc0e0
        .equ CPMFS_G_END,       0xc0e2
        .equ CPMFS_G_BLOCKS,    0xc0e4
        .equ CPMFS_G_SHIFT,     0xc0e6
        .equ CPMFS_G_RESERVED,  0xc0e7
        .equ CPMFS_G_EXM,       0xc0e8
        .globl __cpmfs_geometry, __cpmfs_geometry_iy
        .globl __cpmfs_block_valid, __cpmfs_extent_base
        .globl __cpmfs_record_count
