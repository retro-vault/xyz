        ; Shared IX frame teardown, entered by a tail jump.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih

        .module _frame_return
        .optsdcc -mz80 sdcccall(1)
        .globl  __frame_return
        .area   _CODE

        ; Inputs: IX points to saved caller IX, then return address.
        ; Outputs: frame discarded, IX restored; return to caller.
        ; Preserves every other register and flags. No stack arguments
        ; consumed; use only for caller-cleanup/register arguments.
__frame_return::
        ld      sp,ix
        pop     ix
        ret
