        ; Banked-user-heap free adapter for the public YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_free
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_free
        .globl  __bank_free
        .globl  __bank_current
        .globl  __bank_map

        .area   _CODE

        ; Stack on entry: return, logical bank, address-low, address-high.
        ; The void sdcccall(1) entry removes its three-byte far argument.
        ; The caller's execution bank is restored before returning.
        ; Clobbers AF/BC/DE/HL; preserves IX/IY.
__yos_free::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,(__bank_current)
        push    af
        ld      l,5(ix)
        ld      h,6(ix)
        ld      a,4(ix)
        call    __bank_free
        pop     af
        call    __bank_map
        pop     ix
        pop     hl                      ; return address
        pop     bc                      ; address bytes
        inc     sp                      ; bank byte
        jp      (hl)
