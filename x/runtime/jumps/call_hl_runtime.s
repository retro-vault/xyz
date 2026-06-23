        ; Shared indirect call through hl
        ; used by the compiler to implement function pointers.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module call_hl_runtime
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  __call_hl
        .globl  ___sdcc_call_hl
        .globl  __sdcc_call_hl

__call_hl:
___sdcc_call_hl:
        ; __sdcc_call_hl
        ; inputs:  hl = target address
        ; outputs: n/a (jumps to target)
        ; clobbers: depends on target
__sdcc_call_hl:
        jp      (hl)
