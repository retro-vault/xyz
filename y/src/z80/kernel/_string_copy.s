        ; Copy a NUL-terminated kernel object name.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _string_copy
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_copy

        .area   _CODE

        ; inputs: HL = destination, DE = source.
        ; outputs: DE = original destination; clobbers: af, hl.
__string_copy::
        push    hl
.copy:
        ld      a,(de)
        ld      (hl),a
        inc     de
        inc     hl
        or      a
        jr      nz,.copy
        pop     de
        ret
