        ; shared float pack helper for the runtime
        ;

        .module fppack
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fp_pack_norm

        ; low16
        ; inputs: B=sign mask, C=biased exponent, L=mantissa hi7,
        ; DE=mantissa
        ; outputs: HLDE = packed IEEE-754 single
        ; clobbers: af, hl

__fp_pack_norm:
        ld      a,c
        rrca
        and     #0x80
        or      l
        ld      l,a

        ld      a,c
        srl     a
        or      b
        ld      h,a
        ret
