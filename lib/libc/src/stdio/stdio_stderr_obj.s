        ;; stdio_stderr_obj.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_stderr_obj
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_stderr_obj

        .area   _DATA
__stdio_stderr_obj::
        .db     2, 0, 0, 0

