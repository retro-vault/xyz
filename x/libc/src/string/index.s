        ; index.s
        ;
        ; libc index implementation for the xcc Z80 libc.
        ; Legacy BSD alias for strchr.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module index
        .optsdcc -mz80 sdcccall(1)


        .globl  _index
        .globl  _strchr

        .area   _CODE

        ; _index
        ; inputs:  HL = string pointer, DE = search byte (E)
        ; outputs: DE = pointer to first match, or 0
        ; clobbers: AF, HL
_index::
        jp      _strchr
