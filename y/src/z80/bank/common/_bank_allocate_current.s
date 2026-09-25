        ; Allocate only in the currently mapped user bank for near libc data.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_allocate_current
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_allocate_current
        .globl  __bank_allocate_dispatch
        .globl  __bank_allocate
        .globl  __bank_current
        .globl  __current_process
        .globl  _mem_allocate
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; Select the ordinary all-bank allocator unless libc set bit 15.
__bank_allocate_dispatch::
        bit     7,h
        jp      z,__bank_allocate
        jp      __bank_allocate_current

        ; input: HL = requested bytes with private bit 15 set
        ; output: DE = payload (zero on failure), A = current bank
        ; The routine never changes the mapped bank. Preserves IX/IY.
__bank_allocate_current::
        ex      de,hl
        res     7,d
        call    _enter_critical_section
        call    __current_process
        push    bc                      ; allocation owner
        ld      hl,#0xc000
        call    _mem_allocate
        ld      a,(__bank_current)
        jp      _leave_critical_section
