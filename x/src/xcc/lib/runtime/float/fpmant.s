        ; shared float mantissa unpack helper for the runtime
        ;
        ; expects an IX frame with:
        ; -4(ix)..-1(ix): saved operand a bytes a0..a3
        ; 4(ix).. 7(ix): operand b bytes b0..b3
        ;
        ; writes:
        ; -7(ix).. -9(ix): mant_a low..high (implicit 1 restored in
        ; high)
        ; -10(ix)..-12(ix): mant_b low..high (implicit 1 restored in
        ; high)
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module fpmant
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fp_unpack_mant24_ab

        ; __fp_unpack_mant24_ab
        ; inputs: IX frame with a at -4..-1(ix), b at 4..7(ix)
        ; outputs: mant_a to -7..-9(ix), mant_b to -10..-12(ix)
        ; clobbers: af

__fp_unpack_mant24_ab:
        ; mantissa A (from saved register operand)
        ld      a,-4(ix)
        ld      -7(ix),a
        ld      a,-3(ix)
        ld      -8(ix),a
        ld      a,-2(ix)
        and     #0x7F
        or      #0x80
        ld      -9(ix),a

        ; mantissa B (from stack operand)
        ld      a,4(ix)
        ld      -10(ix),a
        ld      a,5(ix)
        ld      -11(ix),a
        ld      a,6(ix)
        and     #0x7F
        or      #0x80
        ld      -12(ix),a
        ret
