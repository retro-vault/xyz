        ;; copysign.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module copysign
        .optsdcc -mz80 sdcccall(1)

        .globl  _copysign
        .globl  _copysignl
        .globl  __lgd_load_arg0_raw

        .area   _CODE
_copysign::
_copysignl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_raw
        exx
        res     7,h
        bit     7,19(ix)
        jr      z,lgd_copysign_done
        set     7,h
lgd_copysign_done:
        exx
        pop     ix
        ret

