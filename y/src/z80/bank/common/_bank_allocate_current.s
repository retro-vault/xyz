        .globl __bank_model, __sys_heap, __os_malloc, __os_free
        ; Allocate only in the currently mapped user bank for near libc data.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_allocate_current
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_allocate_current
        .globl  __bank_allocate_dispatch
        .globl  __bank_allocate
        .globl  __os_malloc
        .globl  __bank_current
        .globl  __current_process
        .globl  _mem_allocate
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; Private selectors keep near allocations in a known address space.
        ; Bit 15 alone requests the mapped user bank; bits 15+14 request
        ; stable common memory. Bit 14 alone remains an oversized public
        ; request and is rejected by the ordinary bank allocator.
__bank_allocate_dispatch::
        bit     7,h
        jr      z,.any_bank
        bit     6,h
        jp      z,__bank_allocate_current
        res     7,h
        res     6,h
        call    __os_malloc
        ld      a,#0xff
        ret
.any_bank:
        jp      __bank_allocate

        ; input: HL = requested bytes with private bit 15 set
        ; output: DE = payload (zero on failure), A = current bank
        ; The routine never changes the mapped bank. Preserves IX/IY.
__bank_allocate_current::
        .if YOS_UNIFIED_48K
        ld a,(__bank_model)
        or a
        jr nz,.banked
        res 7,h
        call __os_malloc
        ld a,#0xff
        ret
.banked:
        .endif
        ex      de,hl
        res     7,d
        call    _enter_critical_section
        call    __current_process
        push    bc                      ; allocation owner
        ld      hl,#0xc000
        call    _mem_allocate
        ld      a,(__bank_current)
        jp      _leave_critical_section
