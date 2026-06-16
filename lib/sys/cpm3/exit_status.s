        ;; exit_status.s
        ;; Split from sys_exit.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module exit_status
        .optsdcc -mz80 sdcccall(1)

        .globl  ___exit_status
        .globl  __exit_status

        .area   _DATA
__exit_status::
___exit_status::
        .dw     0

