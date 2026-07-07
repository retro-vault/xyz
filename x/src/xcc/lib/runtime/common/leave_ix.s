        ; Shared function epilogue helper
        ; factors out ix frame pointer teardown to reduce code size.
        ;
        ; the compiler emits `jp __sdcc_leave_ix` at function exit
        ; instead of repeating the epilogue inline in every function.
        ;
        ; on entry:
        ;   ix = current frame pointer
        ;   stack top holds the saved caller ix
        ;
        ; on exit:
        ;   caller ix restored
        ;   returns to the caller of the current function
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module leave_ix
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___sdcc_leave_ix
        .globl  __sdcc_leave_ix

___sdcc_leave_ix:
__sdcc_leave_ix:
        ld      sp, ix
        pop     ix
        ret
