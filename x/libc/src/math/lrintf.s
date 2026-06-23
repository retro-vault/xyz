        ;; lrintf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module lrintf
        .optsdcc -mz80 sdcccall(1)

        .globl  _lrintf
        .globl  ___fs2slong
        .globl  _rintf

        .area   _CODE
_lrintf::
        call    _rintf
        jp      ___fs2slong

