        ;; llroundf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module llroundf
        .optsdcc -mz80 sdcccall(1)

        .globl  _llroundf
        .globl  ___db2sll
        .globl  ___fs2db
        .globl  _roundf

        .area   _CODE
_llroundf::
        call    _roundf
        call    ___fs2db
        jp      ___db2sll

