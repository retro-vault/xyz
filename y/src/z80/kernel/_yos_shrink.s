        ; Release the tail of a banked-user-heap block for the public table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_shrink
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_shrink
        .globl  __bank_shrink
        .globl  __bank_current
        .globl  __bank_map

        .area   _CODE

        ; Stack on entry: return, bank, address-low, address-high, size word.
        ; output: HL = payload address, E = logical bank, D = zero; null is
        ;         HL=0000h, E=0. The far return makes this caller-clean.
        ; The caller's execution bank is restored before returning.
        ; Clobbers AF/BC/DE/HL; preserves IX/IY.
__yos_shrink::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,(__bank_current)
        push    af
        ld      l,5(ix)
        ld      h,6(ix)
        ld      a,h
        or      l
        jr      z,.null
        ld      e,7(ix)
        ld      d,8(ix)
        ld      a,4(ix)
        push    af
        call    __bank_shrink
        ex      de,hl
        pop     af
        ld      e,a
        ld      d,#0
        jr      .restore
.null:
        ld      e,#0
.restore:
        pop     af
        call    __bank_map
        pop     ix
        ld      a,h
        or      l
        ret     nz
        ld      e,#0
        ret
