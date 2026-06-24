        ;; sys_none_tmp_open.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_tmp_open
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_tmp_open

        .area   _DATA
__sys_none_tmp_open::
        .dw     0
