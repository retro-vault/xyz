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

        ; Runtime addresses. Startup copies the matching 57-byte gate image
        ; after esxDOS has finished its cold boot.
        .area   _INITIALIZED
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

        ; Each gate is RST 08, inline service selector, RET.
        .area   _INITIALIZER
        .db     0xcf, 0x9a, 0xc9
        .db     0xcf, 0x9b, 0xc9
        .db     0xcf, 0x9c, 0xc9
        .db     0xcf, 0x9d, 0xc9
        .db     0xcf, 0x9e, 0xc9
        .db     0xcf, 0x9f, 0xc9
        .db     0xcf, 0xa0, 0xc9
        .db     0xcf, 0xa1, 0xc9
        .db     0xcf, 0xa8, 0xc9
        .db     0xcf, 0xa9, 0xc9
        .db     0xcf, 0xaa, 0xc9
        .db     0xcf, 0xab, 0xc9
        .db     0xcf, 0xac, 0xc9
        .db     0xcf, 0xad, 0xc9
        .db     0xcf, 0xb0, 0xc9
        .db     0xcf, 0xa3, 0xc9
        .db     0xcf, 0xa4, 0xc9
        .db     0xcf, 0xa7, 0xc9
        .db     0xcf, 0x84, 0xc9
