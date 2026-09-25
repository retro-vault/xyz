        ; Release an OS-heap allocation.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _os_free
        .optsdcc -mz80 sdcccall(1)

        .globl  __os_free
        .globl  __sys_heap
        .globl  _mem_free
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; input: HL = common payload (zero is accepted)
        ; output: DE = merged payload or zero
        ; Clobbers AF/BC/DE/HL; preserves IX/IY.
__os_free::
        ex      de,hl
        call    _enter_critical_section
        ld      hl,#__sys_heap
        call    _mem_free
        jp      _leave_critical_section

