        ; POSIX chdir over resident esxDOS firmware.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module chdir
        .optsdcc -mz80 sdcccall(1)

        .globl  _chdir
        .globl  __zx_esx_path
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_chdir

        .area   _CODE

        ; _chdir
        ; inputs: HL = NUL-terminated path (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
        .globl __zx_esx_path_operation
_chdir::
        ld bc,#__zx_esx_f_chdir
        jp __zx_esx_path_operation
