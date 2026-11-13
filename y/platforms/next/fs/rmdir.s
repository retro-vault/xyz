        ; POSIX rmdir over resident native filesystem firmware.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rmdir
        .optsdcc -mz80 sdcccall(1)

        .globl  _rmdir
        .globl  __next_fs_path
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_f_rmdir

        .area   _CODE

        ; _rmdir
        ; inputs: HL = NUL-terminated path (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_rmdir::
        call    __next_fs_path
        jp      c,__next_fs_errno
        ld      a,#0x2a
        call    __next_fs_f_rmdir
        jp      c,__next_fs_error
        ld      de,#0
        ret
