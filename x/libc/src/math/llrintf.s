        ;; llrintf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module llrintf
        .optsdcc -mz80 sdcccall(1)

        .globl  _llrintf
        .globl  ___db2sll
        .globl  ___fs2db
        .globl  _rintf

        .area   _CODE
_llrintf::
        call    _rintf
        call    ___fs2db
        jp      ___db2sll

        ;; float scalblnf(float x, long n)
        ;; x in HL:DE, n at 4(ix)..7(ix) as signed long
