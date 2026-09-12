        ; Process/library-owned malloc adapter for the public YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_malloc
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_malloc
        .globl  __heap
        .globl  _mem_allocate
        .globl  __current_process
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .equ    THREAD_PROCESS, 22

        .area   _CODE

        ; __yos_malloc, internal service-table adapter
        ; inputs: hl = requested byte count
        ; outputs: de = payload or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
__yos_malloc::
        ex      de, hl
        call    _enter_critical_section
        call    __current_process
        push    bc
        ld      hl, #__heap
        call    _mem_allocate
        jp      _leave_critical_section
