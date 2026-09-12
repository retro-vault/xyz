        ; Unregister a named YOS service.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module svc_unregister
        .optsdcc -mz80 sdcccall(1)
        .globl  _svc_unregister
        .globl  __svc_first
        .globl  _so_destroy
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .area   _CODE

_svc_unregister::
        call    _enter_critical_section
        ex      de, hl
        ld      hl, #__svc_first
        call    _so_destroy
        jp      _leave_critical_section
