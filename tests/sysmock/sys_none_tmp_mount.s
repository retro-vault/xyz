        ;; sys_none_tmp_mount.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_tmp_mount
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_tmp_mount

        .area   _DATA
__sys_none_tmp_mount::
        .dw     0
