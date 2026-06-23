        ; shared float unpack helpers for the runtime
        ;
        ; expects an IX frame with:
        ; -1(ix) = a3, -2(ix) = a2
        ; 7(ix) = b3,  6(ix) = b2
        ;

        .module fpunpack
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fp_unpack_sign_exps

        ; __fp_unpack_sign_exps
        ; inputs: IX frame with a3/a2 at -1/-2(ix), b3/b2 at 7/6(ix)
        ; outputs: -5(ix)=sign(a)^sign(b), C=exp(a), B=exp(b)
        ; clobbers: af, bc

__fp_unpack_sign_exps:
        ; result sign
        ld      a,-1(ix)
        xor     7(ix)
        and     #0x80
        ld      -5(ix),a

        ; exponent of A -> C
        ld      a,-1(ix)
        and     #0x7F
        rlca
        ld      c,a
        bit     7,-2(ix)
        jr      z,.ea_done
        set     0,c
.ea_done:
        ; exponent of B -> B
        ld      a,7(ix)
        and     #0x7F
        rlca
        ld      b,a
        bit     7,6(ix)
        jr      z,.eb_done
        set     0,b
.eb_done:
        ret
