        ; Shared indirect call through bc.
        ; The helper relies on normal Z80 call/ret mechanics:
        ;   call __sdcc_call_bc
        ; becomes
        ;   push return-address
        ;   jump into helper
        ; and the helper does:
        ;   push bc
        ;   ret
        ; so RET jumps to the function pointer in BC, and the callee's
        ; own RET naturally returns to the original caller.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module call_bc_runtime
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___sdcc_call_bc
        .globl  __sdcc_call_bc

___sdcc_call_bc:
        ; __sdcc_call_bc
        ; inputs:  bc = target address
        ; outputs: n/a (jumps to target)
        ; clobbers: depends on target
__sdcc_call_bc:
        push    bc
        ret
