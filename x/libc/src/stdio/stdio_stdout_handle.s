        ;; stdio_stdout_handle.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_stdout_handle
        .optsdcc -mz80 sdcccall(1)

        .globl  ___stdio_stdout_handle
        .globl  __stdio_stdout_handle
        .globl  __stdio_io_reset_stream
        .globl  __stdio_stdout_obj
        .globl  _stdout

        .area   _CODE
___stdio_stdout_handle::
__stdio_stdout_handle::
        ld      hl,#__stdio_stdout_obj
        ld      (_stdout),hl
        ld      a,#1
        push    hl
        call    __stdio_io_reset_stream
        pop     de
        ret
