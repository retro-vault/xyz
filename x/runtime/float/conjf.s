        ; Placeholder complex conjugate helper for the merged runtime.
        ; Keeps the current stub behaviour from the old monolithic
        ; runtime.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        ; This helper is still a stub because the full complex return
        ; path is not implemented yet. The code is preserved only so the
        ; runtime layout matches the old exports.

        .module conjf
        .area   _CODE
        .globl  conjf

        ; conjf
        ; inputs: 4(ix)..11(ix) = complex value.
        ; outputs: returns only the real-part words in DE:HL for now.
        ; clobbers: AF, IX.

conjf:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 4(ix)
        ld      h, 5(ix)
        push    hl
        ld      l, 6(ix)
        ld      h, 7(ix)
        push    hl
        ld      l, 10(ix)
        ld      h, 11(ix)
        ld      a, h
        xor     a, #0x80
        ld      h, a
        pop     de
        pop     hl
        pop     ix
        ret
