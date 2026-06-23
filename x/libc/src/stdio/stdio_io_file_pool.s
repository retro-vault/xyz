        ;; stdio_io_file_pool.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_file_pool
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_file_pool

FILE_FREE_FD    .equ 0xff

        .area   _DATA
__stdio_io_file_pool::
        .db     FILE_FREE_FD,0,0,0
        .db     FILE_FREE_FD,0,0,0
        .db     FILE_FREE_FD,0,0,0
        .db     FILE_FREE_FD,0,0,0
