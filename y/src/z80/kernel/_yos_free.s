        ; Kernel-heap free adapter for the public YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_free
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_free
        .globl  __heap
        .globl  _mem_free

        .area   _CODE

        ; __yos_free, internal service-table adapter
        ; inputs: hl = payload address
        ; outputs: de = merged payload or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
__yos_free::
        ex      de, hl
        ld      hl, #__heap
        jp      _mem_free
