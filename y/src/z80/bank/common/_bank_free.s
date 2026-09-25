        ; Free one allocation in a selected logical bank.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_free
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_free
        .globl  __bank_map
        .globl  _mem_free
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; inputs: HL = payload (zero is accepted), A = logical bank
        ; output: DE = merged payload or zero. Selected bank remains mapped.
__bank_free::
        ld      b,a
        ld      a,h
        or      l
        jr      nz,.free
        ld      de,#0
        ret
.free:
        ld      a,b
        call    _enter_critical_section
        call    __bank_map
        ex      de,hl
        ld      hl,#0xc000
        call    _mem_free
        jp      _leave_critical_section
