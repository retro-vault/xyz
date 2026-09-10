        ; Three-byte RAM gates for esxDOS's external RST 08 ABI.
        ; Only these instructions execute outside the application ROM.
        ; Firmware pages itself out before returning to each RET.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module esxdos_gates
        .optsdcc -mz80 sdcccall(1)
        .globl  __zx_esx_gates_start
        .globl  __zx_esx_gate_88
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

        ; inputs/outputs/clobbers: the selected firmware service ABI.
        ; Gates preserve returned registers and flags, including carry.
        ; _DATA gives each gate a RAM address and a ROM load image.
        .area   _DATA
__zx_esx_gates_start::
__zx_esx_gate_88::
        rst     0x08
        .db     0x88
        ret
__zx_esx_gate_9a::
        rst     0x08
        .db     0x9a
        ret
__zx_esx_gate_9b::
        rst     0x08
        .db     0x9b
        ret
__zx_esx_gate_9c::
        rst     0x08
        .db     0x9c
        ret
__zx_esx_gate_9d::
        rst     0x08
        .db     0x9d
        ret
__zx_esx_gate_9e::
        rst     0x08
        .db     0x9e
        ret
__zx_esx_gate_9f::
        rst     0x08
        .db     0x9f
        ret
__zx_esx_gate_a0::
        rst     0x08
        .db     0xa0
        ret
__zx_esx_gate_a1::
        rst     0x08
        .db     0xa1
        ret
__zx_esx_gate_a8::
        rst     0x08
        .db     0xa8
        ret
__zx_esx_gate_a9::
        rst     0x08
        .db     0xa9
        ret
__zx_esx_gate_aa::
        rst     0x08
        .db     0xaa
        ret
__zx_esx_gate_ab::
        rst     0x08
        .db     0xab
        ret
__zx_esx_gate_ac::
        rst     0x08
        .db     0xac
        ret
__zx_esx_gate_ad::
        rst     0x08
        .db     0xad
        ret
__zx_esx_gate_b0::
        rst     0x08
        .db     0xb0
        ret
