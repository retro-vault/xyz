        ; Shared indirect call through iy
        ; used by the compiler for indirect calls via the iy register.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module call_iy_runtime
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___sdcc_call_iy
        .globl  __sdcc_call_iy

___sdcc_call_iy:
        ; __sdcc_call_iy
        ; inputs:  iy = target address
        ; outputs: n/a (jumps to target)
        ; clobbers: depends on target
__sdcc_call_iy:
        jp      (iy)
