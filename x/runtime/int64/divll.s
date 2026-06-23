        ; 64-bit divide stub for the merged runtime.
        ; Link a full long-long helper library for real behaviour.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module divll
        .area   _CODE
        .globl  __divll

        ; __divll
        ; inputs: xcc long-long helper ABI.
        ; outputs: no useful result yet.
        ; clobbers: none.

__divll:
        ret
