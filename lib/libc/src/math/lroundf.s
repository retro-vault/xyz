        ;; lroundf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module lroundf
        .optsdcc -mz80 sdcccall(1)

        .globl  _lroundf
        .globl  ___fs2slong
        .globl  _roundf

        .area   _CODE
_lroundf::
        call    _roundf
        jp      ___fs2slong

