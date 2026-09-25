        ; Kernel-owned timer adapter for the public YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_install_timer
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_install_timer
        .globl  _tmr_install
        .globl  __current_process

        .area   _CODE

        ; __yos_install_timer, internal service-table adapter
        ; inputs: hl = hook, de = ticks
        ; outputs: de = timer or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
__yos_install_timer::
        call    __current_process
        push    bc
        call    _tmr_install
        ret
