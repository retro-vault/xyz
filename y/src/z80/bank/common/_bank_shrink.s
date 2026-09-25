        ; Shrink one live allocation in a selected logical bank.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_shrink
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_shrink
        .globl  __bank_map
        .globl  __sys_heap
        .globl  __mem_split
        .globl  _mem_free
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; inputs: HL = payload, DE = retained bytes,
        ;         A = logical bank or FFh for the fixed OS heap
        ; output: DE = payload or zero. A selected bank remains mapped.
__bank_shrink::
        push    ix
        push    af
        push    hl
        push    de
        call    _enter_critical_section
        pop     de
        pop     hl
        pop     af
        inc     a
        jr      z,.os_heap
        dec     a
        call    __bank_map
        ld      bc,#0xc000
        jr      .heap_ready
.os_heap:
        ld      bc,#__sys_heap
.heap_ready:
        push    bc
        push    hl
        ld      bc,#-7
        add     hl,bc
        push    hl
        pop     ix
        bit     0,4(ix)
        jr      z,.failed
        call    __mem_split
        jr      c,.kept
        ex      de,hl
        pop     bc
        pop     hl
        push    bc
        call    _mem_free
        pop     de
        jr      .leave
.kept:
        pop     de
        pop     hl
.leave:
        pop     ix
        jp      _leave_critical_section
.failed:
        pop     hl
        pop     hl
        ld      de,#0
        jr      .leave
