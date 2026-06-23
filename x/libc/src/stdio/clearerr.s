        ;; clearerr.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module clearerr
        .optsdcc -mz80 sdcccall(1)

        .globl  _clearerr
        .globl  __stdio_io_clear_flags
        .globl  __stdio_io_require_stream

        .area   _CODE
_clearerr::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_clearerr_done
        call    __stdio_io_clear_flags
__stdio_io_clearerr_done:
        ret

