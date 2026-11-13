        ; Flush an open native filesystem file to its device.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fsync
        .optsdcc -mz80 sdcccall(1)

        .globl  _fsync
        .globl  __critical_call
        .globl  __next_fs_fd
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_f_sync

        .area   _CODE

        ; inputs: HL = fd; output: DE = 0 or -1.
        ; clobbers: af, bc, de, hl. IX/IY preserved.
_fsync::
        call    __critical_call
        call    __next_fs_fd
        jp      c,__next_fs_errno
        call    __next_fs_f_sync
        jp      c,__next_fs_error
        ld      de,#0
        ret
