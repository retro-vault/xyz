        ; 64-bit modulus stub for the merged runtime.
        ; Link a full long-long helper library for real behaviour.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module modll
        .area   _CODE
        .globl  __modll

        ; __modll
        ; inputs: xcc long-long helper ABI.
        ; outputs: no useful result yet.
        ; clobbers: none.

__modll:
        ret
