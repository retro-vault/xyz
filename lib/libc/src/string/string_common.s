        ; string_common.s
        ;
        ; Shared helper routines for the libc string implementation.
        ; The public entry points stay small by routing common return-value,
        ; scanning, copying, and set-membership logic through this file.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih




        .module string_common
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_return_zero

        .area   _CODE
__string_return_zero::
        ld      de,#0x0000
        ret

        ; __string_return_hl
        ; inputs: HL = pointer / size / scalar result
        ; outputs: DE = HL
