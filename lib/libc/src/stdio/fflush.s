        ;; fflush.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fflush
        .optsdcc -mz80 sdcccall(1)

        .globl  _fflush

        .area   _CODE
_fflush::
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

