        ; Unregister a named YOS service.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module svc_unregister
        .optsdcc -mz80 sdcccall(1)
        .globl  _svc_unregister
        .globl  __svc_first
        .globl  _so_destroy
        .area   _CODE

_svc_unregister::
        ex      de, hl
        ld      hl, #__svc_first
        jp      _so_destroy
