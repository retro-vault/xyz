        ; Shared checked path operation and POSIX result conversion.
        ; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module _esxdos_path_operation
        .optsdcc -mz80 sdcccall(1)
        .globl __zx_esx_path_operation, __zx_esx_result, __return_zero
        .globl __zx_esx_path, __zx_esx_errno, __zx_esx_error
        .area _CODE
        ; HL=path, BC=native operation; returns DE=0/-1, errno on failure.
        ; Clobbers AF/BC/DE/HL; preserves IX/IY; no stack arguments.
__zx_esx_path_operation::
        call __zx_esx_path
        jp c,__zx_esx_errno
        ld a,#0x2a
        ld de,#__zx_esx_result
        push de                         ; native operation returns to conversion
        push bc
        ret
        ; Native A/CY -> POSIX DE result. Also used by rename/fsync.
__zx_esx_result::
        jp c,__zx_esx_error
        jp __return_zero
