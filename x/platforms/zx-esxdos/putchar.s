        .module putchar
        .optsdcc -mz80 sdcccall(1)
        .globl  _putchar
        .globl  _zx_console_putc_a
        .area   _CODE
_putchar::
        push    hl
        ld      a,l
        call    _zx_console_putc_a
        pop     de
        ret
