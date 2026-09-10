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
_mkdir::
        ; FAT has no POSIX permission bits; the mode argument is
        ; ignored.
        call    __zx_esx_path
        jp      c,__zx_esx_errno
        ld      a,#0x2a
        call    __zx_esx_f_mkdir
        jp      c,__zx_esx_error
        ld      de,#0
        ret
