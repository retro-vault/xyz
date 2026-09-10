        ; Compare two NUL-terminated kernel object names.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _string_compare
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_compare

        .area   _CODE

        ; inputs: HL = left string, DE = right string.
        ; outputs: DE = zero when equal, nonzero otherwise.
        ; clobbers: af, de, hl.
__string_compare::
.compare:
        ld      a,(de)
        cp      (hl)
        jr      nz,.different
        or      a
        jr      z,.equal
        inc     de
        inc     hl
        jr      .compare
.different:
        ld      de,#1
        ret
.equal:
        ld      de,#0
        ret
