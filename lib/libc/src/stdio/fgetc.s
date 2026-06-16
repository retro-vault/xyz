        ;; fgetc.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fgetc
        .optsdcc -mz80 sdcccall(1)

        .globl  _fgetc
        .globl  __stdio_io_getc_core

        .area   _CODE
_fgetc::
        call    __stdio_io_getc_core
        push    hl
        pop     de
        ret

