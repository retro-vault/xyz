        ; Shared function prologue helper
        ; factors out ix frame pointer setup to reduce code size.
        ;
        ; the compiler emits `call __sdcc_enter_ix` at function entry
        ; instead of repeating the prologue inline in every function.
        ;
        ; on entry the stack looks like:
        ;   [sp+0..1] return address (inside the function being entered)
        ;
        ; on exit:
        ;   ix = caller's sp (frame pointer)
        ;   old ix saved on stack
        ;   execution continues at the return address
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module enter_ix
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___sdcc_enter_ix
        .globl  __sdcc_enter_ix

___sdcc_enter_ix:
        ; __sdcc_enter_ix
        ; inputs:  stack = return address (caller's code)
        ; outputs: ix = frame pointer (sp after push ix)
        ; clobbers: alternate bc
__sdcc_enter_ix:
        exx
        pop     bc                      ; bc' = return address
        exx
        push    ix                      ; save caller's frame pointer
        ld      ix, #0
        add     ix, sp
        exx
        push    bc                      ; restore continuation for RET
        exx
        ret                             ; resume in the calling function
