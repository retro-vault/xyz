        ;; getc.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module getc
        .optsdcc -mz80 sdcccall(1)

        .globl  _getc
        .globl  _fgetc

        .area   _CODE
_getc::
        jp      _fgetc

