        ;; sys_none_open_table.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_open_table
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_open_table

OPEN_COUNT      .equ 4
OPEN_SIZE       .equ 5

        .area   _DATA
__sys_none_open_table::
        .ds     OPEN_COUNT * OPEN_SIZE

