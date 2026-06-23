        ;; setpayloadsigf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module setpayloadsigf
        .optsdcc -mz80 sdcccall(1)

        .globl  _setpayloadsigf

        .area   _CODE
_setpayloadsigf::
        ld      de,#0
        ret

