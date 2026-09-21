        ; Execute an esxDOS dot command through the native RAM gate.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module exec_command
        .optsdcc -mz80 sdcccall(1)
        .globl  _exec_command
        .globl  __zx_esx_m_execcmd

        .area   _CODE

        ; _exec_command
        ; inputs: HL = RAM NUL-terminated command line.
        ; outputs: DE = zero on success, 0x100 + native esxDOS error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
_exec_command::
        call    __zx_esx_m_execcmd
        ld      de,#0
        ret     nc
        ld      e,a
        inc     d                       ; preserve native error zero
        ret
