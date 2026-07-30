        ;; stdio_stderr_handle.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_stderr_handle
        .optsdcc -mz80 sdcccall(1)

        .globl  ___stdio_stderr_handle
        .globl  __stdio_stderr_handle
        .globl  __stdio_io_reset_stream
        .globl  __stdio_stderr_obj
        .globl  _stderr

        .area   _CODE
___stdio_stderr_handle::
__stdio_stderr_handle::
        ld      hl,#__stdio_stderr_obj
        ld      (_stderr),hl
        ld      a,#2
        push    hl
        call    __stdio_io_reset_stream
        pop     de
        ret
