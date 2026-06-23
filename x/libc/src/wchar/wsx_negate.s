        ;; wsx_negate.s
        ;; Split from wcstox_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module wsx_negate
        .optsdcc -mz80 sdcccall(1)

        .globl  __wsx_negate

        .area   _CODE
__wsx_negate::
        ld      b,#8
wsxn_cpl:
        ld      a,(hl)
        cpl
        ld      (hl),a
        inc     hl
        djnz    wsxn_cpl
        ld      bc,#-8
        add     hl,bc
        ld      b,#8
        scf
wsxn_inc:
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        inc     hl
        djnz    wsxn_inc
        ret
