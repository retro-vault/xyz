        ; Copy a NUL-terminated kernel object name.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _string_copy
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_copy

        .area   _CODE

        ; inputs: hl = destination, de = source, b = capacity minus NUL
        ; outputs: bounded, NUL-terminated copy
        ; clobbers: af, b, de, hl; preserves c, ix and iy
__string_copy::
.copy:
        ld      a,(de)
        or      a
        jr      z,.end
        ld      (hl),a
        inc     de
        inc     hl
        djnz    .copy
        xor     a
.end:
        ld      (hl),a
        ret
