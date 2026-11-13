        ; Rewind a YOS directory to its first entry.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rewinddir
        .optsdcc -mz80 sdcccall(1)

        .globl  _rewinddir
        .globl  __critical_call
        .globl  __directory_validate
        .globl  __next_fs_errno
        .globl  __next_fs_error
        .globl  __next_fs_f_rewinddir

        .area   _CODE

        ; input: HL = DIR pointer; errors are reported through errno.
_rewinddir::
        call    __critical_call
        call    __directory_validate
        jp      c,__next_fs_errno
        call    __next_fs_f_rewinddir
        jp      c,__next_fs_error
        ret
