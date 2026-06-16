        ;; sys_none_tmp_count.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_tmp_count
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_tmp_count

        .area   _DATA
__sys_none_tmp_count::
        .dw     0
