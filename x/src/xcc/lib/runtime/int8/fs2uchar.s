        ; float -> unsigned char (ieee-754 single) for the runtime
        ; implemented via ___fs2uint; returns low byte (truncate).
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fs2uchar
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___fs2uchar
        .globl  ___fs2uint

        ; ___fs2uchar
        ; inputs:  float a in hl:de (same ABI chain as ___fs2uint)
        ; outputs: a = (unsigned char)a
        ; clobbers: af, bc, de, hl
___fs2uchar:
        call    ___fs2uint              ; de = uint16 result
        ld      a,e                     ; return low byte in A
        ret
