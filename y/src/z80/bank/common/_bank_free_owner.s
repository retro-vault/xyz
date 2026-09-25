        ; Release an owner's allocations from every logical bank.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_free_owner
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_free_owner
        .globl  __bank_current
        .globl  __bank_count
        .globl  __bank_map
        .globl  _mem_free_owner

        .area   _CODE

        ; input: DE = fixed-memory owner. Caller holds a critical section.
        ; Restores the bank which was mapped on entry. Preserves IX/IY.
__bank_free_owner::
        push    iy
        push    de
        pop     iy
        ld      a,(__bank_current)
        push    af
        xor     a
.next:
        push    af
        call    __bank_map
        push    iy
        pop     de
        ld      hl,#0xc000
        call    _mem_free_owner
        pop     af
        inc     a
        ld      hl,#__bank_count
        cp      (hl)
        jr      c,.next
        pop     af
        call    __bank_map
        pop     iy
        ret
