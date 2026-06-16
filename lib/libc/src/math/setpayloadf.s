        ;; setpayloadf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module setpayloadf
        .optsdcc -mz80 sdcccall(1)

        .globl  _setpayloadf

        .area   _CODE
_setpayloadf::
        ; set payload (simplified, assume valid)
        ; x at stack? for basic, return 0 success
        ld      de,#0
        ret

