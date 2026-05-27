        ; float helper utilities for the runtime.
        ; these are small shared utility entry points only.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fp
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        ; __fp_zero32
        ; inputs: n/a
        ; outputs: de:hl = 0x00000000
        ; clobbers: af, de, hl
        .globl  __fp_zero32

        ; __fp_zero32

__fp_zero32:
.zero32:
        xor     a
        ld      h,a
        ld      l,a
        ld      d,a
        ld      e,a
        ret

        ; __fp_zero16
        ; inputs: n/a
        ; outputs: hl = 0x0000
        ; clobbers: af, hl
        .globl  __fp_zero16
        ; __fp_zero16
__fp_zero16:
.zero16:
        xor     a
        ld      h,a
        ld      l,a
        ret
