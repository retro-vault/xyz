        ;; sys_none_mount_table.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_mount_table
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_mount_table

MOUNT_COUNT     .equ 4
MOUNT_SIZE      .equ 8

        .area   _DATA
__sys_none_mount_table::
        .ds     MOUNT_COUNT * MOUNT_SIZE
