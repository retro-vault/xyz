        ; POSIX rmdir over resident esxDOS firmware.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rmdir
        .optsdcc -mz80 sdcccall(1)

        .globl  _rmdir
        .globl  __zx_esx_path
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_rmdir

        .area   _CODE

        ; _rmdir
        ; inputs: HL = NUL-terminated path (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
        .globl __zx_esx_path_operation
_rmdir::
        ld bc,#__zx_esx_f_rmdir
        jp __zx_esx_path_operation
