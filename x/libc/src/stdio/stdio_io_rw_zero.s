        ;; stdio_io_rw_zero.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_rw_zero
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_rw_zero

        .area   _CODE
__stdio_io_rw_zero::
        ld      hl,#0x0000
        ld      sp,ix
        pop     ix
        ret

