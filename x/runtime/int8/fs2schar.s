        ; float -> signed char (ieee-754 single) for the runtime
        ; implemented via ___fs2sint; returns low byte (two's complement
        ; truncation).
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fs2schar
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___fs2schar
        .globl  ___fs2sint

        ; ___fs2schar
        ; inputs:  float a in hl:de (same ABI chain as mk_f32 ->
        ; fs2sint)
        ; outputs: a = (signed char)a
        ; clobbers: af, bc, de, hl
___fs2schar:
        call    ___fs2sint              ; de = int16 result
        ld      a,e                     ; return low byte in A
        ret
