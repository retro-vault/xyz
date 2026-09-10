        ; Read one handler from the writable restart-vector table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module sys_vec_get
        .optsdcc -mz80 sdcccall(1)

        .globl  _sys_vec_get
        .globl  __sys_vec_tbl
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; _sys_vec_get, sdcccall(1)
        ; input: a = vector number
        ; output: de = handler address
        ; clobbers: af, de, hl; preserves bc, ix and iy
_sys_vec_get::
        call    _enter_critical_section
        ld      e, a
        ld      d, #0
        ld      hl, #__sys_vec_tbl
        add     hl, de
        add     hl, de
        add     hl, de
        inc     hl
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        call    _leave_critical_section
        ret
