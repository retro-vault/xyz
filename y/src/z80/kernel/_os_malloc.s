        ; Allocate common memory for an OS-managed object.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _os_malloc
        .optsdcc -mz80 sdcccall(1)

        .globl  __os_malloc
        .globl  __sys_heap
        .globl  _mem_allocate
        .globl  __current_process
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; input: HL = requested byte count
        ; output: DE = common payload or zero
        ; Ownership follows the current process/library initialization owner.
        ; Clobbers AF/BC/DE/HL; preserves IX/IY.
__os_malloc::
        ex      de,hl
        call    _enter_critical_section
        call    __current_process
        push    bc
        ld      hl,#__sys_heap
        call    _mem_allocate
        jp      _leave_critical_section

