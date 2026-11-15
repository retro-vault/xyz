        ; Start the RAM kernel after the bootstrap has loaded it at C000.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        ;
        .module crt0divmmc
        .optsdcc -mz80 sdcccall(1)
        .globl __divmmc_entry, __startup_init, __sys_stack, __sys_vec_tbl
        .globl __divmmc_initialize, __esx_print, _main
        .area _CODE
__divmmc_entry::
        di
        ld sp,#0xffff
        call __startup_init
        ld sp,#__sys_stack
        call __divmmc_initialize
        jp _main
        .area _INITIALIZER
        .area _INITFINAL
        .area _GSINIT
        .area _GSFINAL
        .area _CONST
        .area _DATA
        .area _INITIALIZED
        .area _BSS
        .area _HEAP
