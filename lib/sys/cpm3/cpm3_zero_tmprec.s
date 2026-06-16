        ;; cpm3_zero_tmprec.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_zero_tmprec
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_zero_tmprec
        .globl  __cpm3_tmp_rec
        .globl  __cpm3_zero_bytes

        .area   _CODE
__cpm3_zero_tmprec::
        ld      hl,#__cpm3_tmp_rec
        ld      b,#4
        jp      __cpm3_zero_bytes

        ;; Copy __cpm3_tmp_rec into __cpm3_tmp_rec2.
