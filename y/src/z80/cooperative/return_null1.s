        ;; Optional public operation with 1 callee-clean argument bytes.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module return_null1
        .optsdcc -mz80 sdcccall(1)
        .globl __yos_return_null_stack
        .area _CODE
__yos_return_null1::
        ld a,#1
        jp __yos_return_null_stack
