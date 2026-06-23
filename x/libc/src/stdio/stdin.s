        ;; stdin.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdin
        .optsdcc -mz80 sdcccall(1)

        .globl  _stdin
        .globl  __stdio_stdin_obj

        .area   _DATA
_stdin::
        .dw     __stdio_stdin_obj
