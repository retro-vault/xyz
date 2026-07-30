        ;; stdio_stdin_handle.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_stdin_handle
        .optsdcc -mz80 sdcccall(1)

        .globl  ___stdio_stdin_handle
        .globl  __stdio_stdin_handle
        .globl  __stdio_io_reset_stream
        .globl  __stdio_stdin_obj
        .globl  _stdin

        .area   _CODE
___stdio_stdin_handle::
__stdio_stdin_handle::
        ld      hl,#__stdio_stdin_obj
        ld      (_stdin),hl
        xor     a
        push    hl
        call    __stdio_io_reset_stream
        pop     de
        ret
