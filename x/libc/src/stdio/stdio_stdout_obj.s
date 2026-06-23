        ;; stdio_stdout_obj.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_stdout_obj
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_stdout_obj

        .area   _DATA
__stdio_stdout_obj::
        .db     1, 0, 0, 0
