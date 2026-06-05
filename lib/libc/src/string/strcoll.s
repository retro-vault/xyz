        ; strcoll.s
        ;
        ; libc strcoll implementation for the xcc Z80 libc.
        ; The current libc is locale-agnostic, so collation falls back to the
        ; plain bytewise strcmp ordering.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strcoll
        .optsdcc -mz80 sdcccall(1)


        .globl  _strcoll
        .globl  _strcmp

        .area   _CODE

        ; _strcoll
        ; inputs/outputs/clobbers: same as _strcmp
_strcoll::
        jp      _strcmp
