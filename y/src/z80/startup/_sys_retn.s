        ; Default non-maskable-interrupt handler.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _sys_retn
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_retn

        .area   _CODE

        ; __sys_retn, internal default NMI vector handler
        ; preserves all registers
__sys_retn::
        retn
