        ; unsigned char to float (ieee-754 single) for the runtime
        ; implemented via ___uint2fs.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module uchar2fs
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___uchar2fs
        .globl  ___uint2fs

        ; ___uchar2fs
        ; inputs:  a = value (0..255)
        ; outputs: hl:de = (float)a
        ; clobbers: af, bc, de, hl
___uchar2fs:
        ; widen to 16-bit unsigned in HL
        ld      l,a
        xor     a
        ld      h,a
        jp      ___uint2fs              ; tail-call
