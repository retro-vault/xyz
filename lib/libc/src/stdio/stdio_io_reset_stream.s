        ;; stdio_io_reset_stream.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_reset_stream
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_reset_stream

        .area   _CODE
__stdio_io_reset_stream::
        ld      (hl),a
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ret

        ;; HL = FILE*. Mark the slot free.
