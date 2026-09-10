        ; Default maskable-interrupt and restart handler.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _sys_reti
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_reti

        .area   _CODE

        ; __sys_reti, internal default vector handler
        ; preserves all registers
__sys_reti::
        reti
