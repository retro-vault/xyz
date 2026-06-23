        ;; stdio_io_require_stream.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_require_stream
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_require_stream

        .area   _CODE
__stdio_io_require_stream::
        ld      a,h
        or      l
        ret     nz
        ld      hl,#0xffff
        ret

        ;; HL = FILE*. Clear EOF+ERR bits.
