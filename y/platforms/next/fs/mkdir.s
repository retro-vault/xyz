        ; POSIX mkdir over resident native filesystem firmware.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mkdir
        .optsdcc -mz80 sdcccall(1)

        .globl  _mkdir
        .globl  __next_fs_path
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_f_mkdir

        .area   _CODE

        ; _mkdir
        ; inputs: HL = path, DE = ignored mode (sdcccall(1)).
        ; outputs: DE = 0 or -1 with errno set.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_mkdir::
        ; FAT has no POSIX permission bits; the mode argument is
        ; ignored.
        call    __next_fs_path
        jp      c,__next_fs_errno
        ld      a,#0x2a
        call    __next_fs_f_mkdir
        jp      c,__next_fs_error
        ld      de,#0
        ret
