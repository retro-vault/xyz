        ; shared float helper epilogue for the runtime
        ; drops one 32-bit stack argument (4 bytes) and returns to
        ; caller.
        ;
        ; expected stack on entry:
        ; [sp+0..1] return address
        ; [sp+2..5] 32-bit argument to discard
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module fpret
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fp_retpop4

        ; __fp_retpop4
        ; inputs: stack = return address + one 32-bit arg to discard
        ; outputs: returns to caller with stack cleaned by 4 bytes
        ; clobbers: af, bc

__fp_retpop4:
        ; save return address
        pop     bc
        ; drop arg low word
        pop     af
        ; drop arg high word
        pop     af
        ; restore return address
        push    bc
        ret
