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
_rmdir::
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ld      a,#0x2a
        call    __zx_esx_f_rmdir
        jp      c,__zx_esx_error
        ld      de,#0
        ret
