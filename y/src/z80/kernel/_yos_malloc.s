        ; Process/library-owned banked allocator for the public YOS table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_malloc
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_malloc
        .globl  __bank_allocate_dispatch
        .globl  __bank_current
        .globl  __bank_map

        .area   _CODE

        ; input: HL = requested byte count
        ; output: HL = payload address, E = logical bank, D = zero.
        ;         A null result is HL=0000h, E=0.
        ; The caller's execution bank is restored before returning.
        ; Clobbers AF/BC/DE/HL; preserves IX/IY.
__yos_malloc::
        ld      a,(__bank_current)
        push    af
        call    __bank_allocate_dispatch
        ex      de,hl
        ld      e,a
        ld      d,#0
        pop     af
        call    __bank_map
        ret
