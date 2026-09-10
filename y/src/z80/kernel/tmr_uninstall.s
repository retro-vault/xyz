        ; Uninstall a YOS timer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module tmr_uninstall
        .optsdcc -mz80 sdcccall(1)
        .globl  _tmr_uninstall
        .globl  __tmr_first
        .globl  _so_destroy
        .area   _CODE
_tmr_uninstall::
        ex      de, hl
        ld      hl, #__tmr_first
        jp      _so_destroy
