        ; Register a named YOS service.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module svc_register
        .optsdcc -mz80 sdcccall(1)
        .globl  _svc_register
        .globl  __svc_first
        .globl  _so_create
        .globl  __string_copy
        .area   _CODE

        ; hl = name, de = function table; returns de = service or zero.
_svc_register::
        push    hl
        push    de
        ld      hl, #0
        push    hl
        ld      de, #22
        ld      hl, #__svc_first
        call    _so_create
        pop     bc
        pop     hl
        ld      a, d
        or      e
        ret     z
        push    de
        push    bc
        ex      de, hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        call    __string_copy
        pop     bc
        pop     de
        ld      hl, #20
        add     hl, de
        ld      (hl), c
        inc     hl
        ld      (hl), b
        ret
