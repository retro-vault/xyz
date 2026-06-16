        ;; ctype_return_true.s
        ;; Split from ctype_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ctype_return_true
        .optsdcc -mz80 sdcccall(1)

        .globl  __ctype_return_true

        .area   _CODE
__ctype_return_true::
        ld      de,#0x0001
        ret

        ;; __ctype_return_flag
        ;; Convert a prior equality or range test into libc's 0/1 integer result.
        ;; Callers arrange for Z to mean "classification matched".
