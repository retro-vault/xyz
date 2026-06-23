        ; imaxdiv.s
        ;
        ; libc imaxdiv for the xcc Z80 libc.  intmax_t == long long and
        ; imaxdiv_t has the same layout as lldiv_t, so this is an exact-ABI
        ; tail call into lldiv.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module imaxdiv
        .optsdcc -mz80 sdcccall(1)

        .globl  _imaxdiv
        .globl  _lldiv

        .area   _CODE
_imaxdiv::
        jp      _lldiv
