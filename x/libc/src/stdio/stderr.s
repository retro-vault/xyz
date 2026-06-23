        ;; stderr.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stderr
        .optsdcc -mz80 sdcccall(1)

        .globl  _stderr
        .globl  __stdio_stderr_obj

        .area   _DATA
_stderr::
        .dw     __stdio_stderr_obj

