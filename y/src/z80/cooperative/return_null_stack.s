        ;; A=callee-clean argument bytes; preserve IX/IY and caller SP.
        ;; Shared by optional API adapters on cooperative Z80 hosts.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module return_null_stack
        .optsdcc -mz80 sdcccall(1)
        .globl __yos_return_null
        .area _CODE
__yos_return_null_stack::
        pop bc
        ld l,a
        ld h,#0
        add hl,sp
        ld sp,hl
        push bc
        jp __yos_return_null
