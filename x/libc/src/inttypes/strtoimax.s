        ; strtoimax.s
        ;
        ; libc strtoimax for the xcc Z80 libc.  intmax_t == long long, so this
        ; is an exact-ABI tail call into strtoll.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strtoimax
        .optsdcc -mz80 sdcccall(1)

        .globl  _strtoimax
        .globl  _strtoll

        .area   _CODE
_strtoimax::
        jp      _strtoll
