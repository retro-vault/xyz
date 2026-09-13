        ; RAM gates for esxDOS calls made by the replacement ROM.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _esxdos_gates
        .optsdcc -mz80 sdcccall(1)

        .globl  __zx_esx_gates_start
        .globl  __zx_esx_gate_9a
        .globl  __zx_esx_gate_9b
        .globl  __zx_esx_gate_9c
        .globl  __zx_esx_gate_9d
        .globl  __zx_esx_gate_9e
        .globl  __zx_esx_gate_9f
        .globl  __zx_esx_gate_a0
        .globl  __zx_esx_gate_a1
        .globl  __zx_esx_gate_a8
        .globl  __zx_esx_gate_a9
        .globl  __zx_esx_gate_aa
        .globl  __zx_esx_gate_ab
        .globl  __zx_esx_gate_ac
        .globl  __zx_esx_gate_ad
        .globl  __zx_esx_gate_b0
        .globl  __zx_esx_gate_a3
        .globl  __zx_esx_gate_a4
        .globl  __zx_esx_gate_a7
        .globl  __zx_esx_gate_84
        .globl  __esxdos_gates_init

        ; Runtime addresses. Startup copies the matching 57-byte gate image
        ; after esxDOS has finished its cold boot.
        .area   _BSS
__zx_esx_gates_start::
__zx_esx_gate_9a::
        .ds     3
__zx_esx_gate_9b::
        .ds     3
__zx_esx_gate_9c::
        .ds     3
__zx_esx_gate_9d::
        .ds     3
__zx_esx_gate_9e::
        .ds     3
__zx_esx_gate_9f::
        .ds     3
__zx_esx_gate_a0::
        .ds     3
__zx_esx_gate_a1::
        .ds     3
__zx_esx_gate_a8::
        .ds     3
__zx_esx_gate_a9::
        .ds     3
__zx_esx_gate_aa::
        .ds     3
__zx_esx_gate_ab::
        .ds     3
__zx_esx_gate_ac::
        .ds     3
__zx_esx_gate_ad::
        .ds     3
__zx_esx_gate_b0::
        .ds     3
__zx_esx_gate_a3::
        .ds     3
__zx_esx_gate_a4::
        .ds     3
__zx_esx_gate_a7::
        .ds     3
__zx_esx_gate_84::
        .ds     3

        ; Generate the writable gates after BSS clearing. Only their service
        ; selectors need stored ROM bytes.
        .area   _CODE
__esxdos_gates_init::
        ld      hl,#.selectors
        ld      de,#__zx_esx_gates_start
        ld      bc,#19
.next:
        ld      a,#0xcf                 ; RST 08
        ld      (de),a
        inc     de
        ldi
        ld      a,#0xc9                 ; RET
        ld      (de),a
        inc     de
        ld      a,b
        or      c
        jr      nz,.next
        ret

        .area   _HEADER_DATA
.selectors:
        .db     0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1
        .db     0xa8,0xa9,0xaa,0xab,0xac,0xad,0xb0,0xa3
        .db     0xa4,0xa7,0x84
