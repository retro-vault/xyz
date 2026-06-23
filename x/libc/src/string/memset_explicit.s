        ; memset_explicit.s
        ;
        ; libc memset_explicit implementation for the xcc Z80 libc.
        ; The backend does not currently delete explicit calls, so the helper
        ; simply aliases the normal memset implementation.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memset_explicit
        .optsdcc -mz80 sdcccall(1)


        .globl  _memset_explicit
        .globl  _memset

        .area   _CODE

        ; _memset_explicit
        ; inputs/outputs/clobbers: same as _memset
_memset_explicit::
        jp      _memset
