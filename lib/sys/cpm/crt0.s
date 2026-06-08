        ; crt0.s  (CP/M COM startup)
        ;
        ; Minimal CP/M startup for a COM-style program. Execution begins at
        ; 0x0100 and CP/M provides a valid stack/return path back to the CCP,
        ; so startup calls _main and returns.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  _entry

        .area   _CODE
_entry::
        call    _main
        ret

        .area   _GSINIT
        .area   _GSFINAL
        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _HEAP
        .area   _INITIALIZER
