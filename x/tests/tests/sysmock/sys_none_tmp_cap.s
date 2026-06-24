        ;; sys_none_tmp_cap.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_tmp_cap
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_tmp_cap

        .area   _DATA
__sys_none_tmp_cap::
        .dw     0
