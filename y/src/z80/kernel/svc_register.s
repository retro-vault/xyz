        ; Register a named YOS service.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module svc_register
        .optsdcc -mz80 sdcccall(1)
        .globl  _svc_register
        .globl  __svc_first
        .globl  __library_private_services
        .globl  __current_process
        .globl  _so_create
        .globl  __string_copy
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .area   _CODE

        ; hl = name, de = function table; returns de = service or zero.
_svc_register::
        call    _enter_critical_section
        push    hl
        push    de
        call    __current_process
        push    bc
        ld      h, b
        ld      l, c
        ld      a, h
        or      l
        jr      z, .public
        ld      de, #5
        add     hl, de
        bit     0, (hl)
        ld      hl, #__library_private_services
        jr      nz, .create
.public:
        ld      hl, #__svc_first
.create:
        ld      de, #23
        call    _so_create
        pop     bc
        pop     hl
        ld      a, d
        or      e
        jp      z, _leave_critical_section
        push    de
        push    bc
        ex      de, hl
        ld      bc, #5
        add     hl, bc
        ld      b, #15
        call    __string_copy
        pop     bc
        pop     de
        ld      hl, #21
        add     hl, de
        ld      (hl), c
        inc     hl
        ld      (hl), b
        jp      _leave_critical_section
