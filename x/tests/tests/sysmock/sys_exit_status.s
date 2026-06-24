        ;; sys_exit_status.s
        ;; Split from sys_exit.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_exit_status
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_exit_status
        .globl  __sys_exit_status

        .area   _DATA
__sys_exit_status::
___sys_exit_status::
        .dw     0

