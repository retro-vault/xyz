        ; Replace one handler in the writable restart-vector table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module sys_vec_set
        .optsdcc -mz80 sdcccall(1)

        .globl  _sys_vec_set
        .globl  __sys_vec_tbl
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; _sys_vec_set, sdcccall(1)
        ; inputs: hl = handler, vector byte at sp+2
        ; outputs: none; removes vector byte
        ; clobbers: af, bc, de, hl; preserves ix and iy
_sys_vec_set::
        call    _enter_critical_section
        push    iy
        ld      iy, #4
        add     iy, sp
        ld      e, 0(iy)
        push    hl
        ld      d, #0
        ld      hl, #__sys_vec_tbl
        add     hl, de
        add     hl, de
        add     hl, de
        inc     hl
        pop     bc
        ld      (hl), c
        inc     hl
        ld      (hl), b
        call    _leave_critical_section
        pop     iy
        pop     bc
        inc     sp
        push    bc
        ret
