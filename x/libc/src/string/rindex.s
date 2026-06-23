        ; rindex.s
        ;
        ; libc rindex implementation for the xcc Z80 libc.
        ; Legacy BSD alias for strrchr.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rindex
        .optsdcc -mz80 sdcccall(1)


        .globl  _rindex
        .globl  _strrchr

        .area   _CODE

        ; _rindex
        ; inputs:  HL = string pointer, DE = search byte (E)
        ; outputs: DE = pointer to last match, or 0
        ; clobbers: AF, BC, HL
_rindex::
        jp      _strrchr
