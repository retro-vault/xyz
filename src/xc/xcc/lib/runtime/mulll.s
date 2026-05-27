        ; 64-bit multiply stub for the merged runtime.
        ; Link a full long-long helper library for real behaviour.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mulll
        .area   _CODE
        .globl  __mulll

        ; __mulll
        ; inputs: xcc long-long helper ABI.
        ; outputs: no useful result yet.
        ; clobbers: none.

__mulll:
        ret
