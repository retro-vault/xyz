        ; Create a YOS synchronization event.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module evt_create
        .optsdcc -mz80 sdcccall(1)
        .globl  _evt_create
        .globl  __evt_first
        .globl  _so_create
        .area   _CODE

        ; hl = owner; returns de = event or zero.
_evt_create::
        push    hl
        ld      de, #5
        ld      hl, #__evt_first
        call    _so_create
        ld      a, d
        or      e
        ret     z
        ld      hl, #4
        add     hl, de
        ld      (hl), #0
        ret
