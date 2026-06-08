        ; crt0.s  (ZX RAM image startup)
        ;
        ; Minimal ZX Spectrum RAM-loaded startup. Code is intended to be
        ; linked into the upper 32K RAM window, with a private stack at the
        ; top of memory. This startup does not clear BSS or copy initialized
        ; data.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  _entry

        .area   _CODE
_entry::
        di
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
