        ; strtoumax.s
        ;
        ; libc strtoumax for the xcc Z80 libc.  uintmax_t == unsigned long
        ; long, so this is an exact-ABI tail call into strtoull.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strtoumax
        .optsdcc -mz80 sdcccall(1)

        .globl  _strtoumax
        .globl  _strtoull

        .area   _CODE
_strtoumax::
        jp      _strtoull
