        ; Kernel-owned malloc adapter for the public YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_malloc
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_malloc
        .globl  __heap
        .globl  _mem_allocate

        .area   _CODE

        ; __yos_malloc, internal service-table adapter
        ; inputs: hl = requested byte count
        ; outputs: de = payload or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
__yos_malloc::
        ex      de, hl
        ld      bc, #0
        push    bc
        ld      hl, #__heap
        call    _mem_allocate
        ret
