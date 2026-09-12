        ; Writable Kempston mouse position and button history.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _mouse_state
        .optsdcc -mz80 sdcccall(1)

        .globl  __mouse_cursor
        .globl  __mouse_hardware
        .globl  __mouse_buttons
        .globl  __mouse_changes

        .area   _BSS
__mouse_cursor::
        .ds     2
__mouse_buttons::
        .ds     1
__mouse_changes::
        .ds     1
__mouse_hardware::
        .ds     2
