        ;; sfp_mul10.s
        ;; Split from strtod_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sfp_mul10
        .optsdcc -mz80 sdcccall(1)

        .globl  sfp_mul10
        .globl  __dbmul

        .area   _CODE
sfp_mul10::
        ld      hl,#0x4024
        push    hl
        ld      hl,#0x0000
        push    hl
        push    hl
        push    hl
        call    __dbmul
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        ret

