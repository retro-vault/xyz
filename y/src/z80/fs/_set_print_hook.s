        ; Install a RAM print sink for esxDOS's 09F4h ROM entry.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module set_print_hook
        .optsdcc -mz80 sdcccall(1)
        .globl  _set_print_hook
        .globl  __esx_print_hook
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; _set_print_hook
        ; inputs: HL = replacement RAM sink, or zero to disable.
        ; outputs: DE = previous sink.
        ; clobbers: af, de; preserves bc, hl, ix and iy.
_set_print_hook::
        call    _enter_critical_section
        ld      de,(__esx_print_hook)
        ld      (__esx_print_hook),hl
        jp      _leave_critical_section
