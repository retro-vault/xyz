        ;; stdio_stdin_obj.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_stdin_obj
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_stdin_obj

        .area   _DATA
__stdio_stdin_obj::
        .db     0, 0, 0, 0
