        ; Kernel-heap free adapter for the public YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_free
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_free
        .globl  __heap
        .globl  _mem_free
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; __yos_free, internal service-table adapter
        ; inputs: hl = payload address
        ; outputs: de = merged payload or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
__yos_free::
        ex      de, hl
        call    _enter_critical_section
        ld      hl, #__heap
        call    _mem_free
        call    _leave_critical_section
        ret
