        ;; nan.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module nan
        .optsdcc -mz80 sdcccall(1)

        .globl  _nan
        .globl  _nanl

        .area   _CODE
_nan::
_nanl::
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x7ff8
        exx
        ret

