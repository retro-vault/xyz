        ; Writable ZX Spectrum keyboard scanner state and event queue.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _keyboard_state
        .optsdcc -mz80 sdcccall(1)

        .globl  __kbd_prev_scan
        .globl  __kbd_caps
        .globl  __kbd_symbol
        .globl  __kbd_buffer

        .area   _BSS
__kbd_prev_scan::
        .ds     8
__kbd_caps::
        .ds     1
__kbd_symbol::
        .ds     1
__kbd_buffer::
        .ds     35
