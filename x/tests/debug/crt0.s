        .module crt0

        .globl    _main
        .globl    _entry

        .area     _CODE
_entry::
        call      _main

halt_loop:
        halt
        jr        halt_loop

        .area     _GSINIT
        .area     _GSFINAL
        .area     _DATA
        .area     _INITIALIZED
        .area     _BSS
        .area     _HEAP
        .area     _INITIALIZER
