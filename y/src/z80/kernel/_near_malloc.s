        ; Allocate near caller data in its execution bank when possible.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _near_malloc
        .optsdcc -mz80 sdcccall(1)
        .globl  __near_malloc
        .globl  __bank_current
        .globl  __os_malloc
        .globl  __bank_allocate_current
        .area   _ROM_AUX

        ; input: HL = size; output: DE = stable near pointer or zero.
        ; Fixed execution uses common memory. Far calls can change the mapped
        ; bank between IRQs, so the thread's last saved bank is not authoritative.
        ; A banked caller receives storage in its current execution bank.
__near_malloc::
        ld      a, (__bank_current)
        inc     a
        jp      z, __os_malloc
        set     7, h
        jp      __bank_allocate_current
