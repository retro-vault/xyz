        ; Rewind a YOS directory to its first entry.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rewinddir
        .optsdcc -mz80 sdcccall(1)

        .globl  _rewinddir
        .globl  __critical_call
        .globl  __directory_validate
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_rewinddir

        .area   _CODE

        ; input: HL = DIR pointer; errors are reported through errno.
_rewinddir::
        call    __critical_call
        call    __directory_validate
        jp      c,__zx_esx_errno
        call    __zx_esx_f_rewinddir
        jp      c,__zx_esx_error
        ret
