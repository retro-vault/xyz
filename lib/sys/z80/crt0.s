        ; crt0.s  (generic flat Z80 startup)
        ;
        ; Minimal bare-metal startup for a flat 64K Z80 memory map.
        ; Code starts at address 0 and the stack is placed at the top
        ; of the address space. This startup intentionally does not
        ; clear BSS or copy initialized data.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  _entry

        .area   _CODE
_entry::
        ld      sp,#0xffff
        call    _main
halt_loop:
        halt
        jr      halt_loop

        .area   _GSINIT
        .area   _GSFINAL
        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _HEAP
        .area   _INITIALIZER
