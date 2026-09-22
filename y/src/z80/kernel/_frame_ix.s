        ; Shared IX frame entry, called in place of the inline prologue.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih

        .module _frame_ix
        .optsdcc -mz80 sdcccall(1)
        .globl  __frame_ix
        .area   _CODE

        ; Inputs: CALL return at SP; all argument registers are live.
        ; Outputs: saved caller IX at SP, IX = SP, resume after CALL.
        ; Preserves A, BC, DE, HL, IY and alternate registers; flags
        ; follow ADD IX,SP (S/Z/PV preserved, N cleared, H/C clobbered).
        ; No caller arguments consumed. Two transient stack bytes.
__frame_ix::
        ex      (sp),ix
        push    ix
        ld      ix,#2
        add     ix,sp
        ret
