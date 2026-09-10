        ; Destroy a YOS synchronization event.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module evt_destroy
        .optsdcc -mz80 sdcccall(1)
        .globl  _evt_destroy
        .globl  __evt_first
        .globl  _so_destroy
        .area   _CODE

        ; hl = event; returns de = released block or zero.
_evt_destroy::
        ex      de, hl
        ld      hl, #__evt_first
        jp      _so_destroy
