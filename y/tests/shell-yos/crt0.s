        ; Minimal startup for the relocatable YOS shell fixture.
        ; The kernel owns the process stack and initialized state.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 Tomaz Stih

        .module   shell_crt0

        .globl    _main
        .globl    _entry
        .globl    _query_service
        .globl    ___sdcc_call_hl
        .globl    __sdcc_call_hl
        .globl    ___sdcc_call_iy
        .globl    __sdcc_call_iy

        .area     _CODE
_entry::
        call      _main
        ret

        ; RST18 is YOS's named-service trap. RST10 remains the immediate
        ; return required by esxDOS while it cold-boots the replacement ROM.
_query_service::
        rst       0x18
        ret

___sdcc_call_hl::
__sdcc_call_hl::
        jp        (hl)

___sdcc_call_iy::
__sdcc_call_iy::
        push      iy
        ret

        .area     _GSINIT
        .area     _GSFINAL
        .area     _DATA
        .area     _INITIALIZED
        .area     _BSS
        .area     _HEAP
        .area     _INITIALIZER
