        ;; stdout.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdout
        .optsdcc -mz80 sdcccall(1)

        .globl  _stdout
        .globl  __stdio_stdout_obj

        .area   _DATA
_stdout::
        .dw     __stdio_stdout_obj
