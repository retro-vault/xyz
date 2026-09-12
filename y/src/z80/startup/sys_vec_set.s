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
        ex      de,hl                   ; handler
        pop     bc
        pop     hl                      ; vector byte and untouched caller byte
        dec     sp                      ; consume only the vector byte
        push    bc                      ; relocate the return address
        ld      c,l
        ld      b,#0
        ld      hl,#__sys_vec_tbl+1
        add     hl,bc
        add     hl,bc
        add     hl,bc
        ld      (hl), e
        inc     hl
        ld      (hl), d
        jp      _leave_critical_section
