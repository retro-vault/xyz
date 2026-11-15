        ; POSIX mkdir over resident esxDOS firmware.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mkdir
        .optsdcc -mz80 sdcccall(1)

        .globl  _mkdir
        .globl  __zx_esx_path
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_mkdir

        .area   _CODE

        ; _mkdir
        ; inputs: HL = path, DE = ignored mode (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
        .globl __zx_esx_path_operation
_mkdir::
        ld bc,#__zx_esx_f_mkdir
        jp __zx_esx_path_operation
