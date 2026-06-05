        ; Imaginary unit constant for float complex support.
        ; Stores {0.0f, 1.0f} as four little-endian 16-bit words.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module complex_i
        .area   _DATA
        .globl  __complex_I

        ; __complex_I
        ; inputs: none.
        ; outputs: exported data object __complex_I.
        ; clobbers: none.

__complex_I:
        .dw     0x0000
        .dw     0x0000
        .dw     0x0000
        .dw     0x3f80
