        ;; putc.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module putc
        .optsdcc -mz80 sdcccall(1)

        .globl  _putc
        .globl  _fputc

        .area   _CODE
_putc::
        jp      _fputc

