        ;; cpm3_console_putchar.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_console_putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_console_putchar

BDOS            .equ 5
C_WRITE         .equ 2

        .area   _CODE
__cpm3_console_putchar::
        push    ix
        push    iy
        push    bc
        push    de
        push    hl
        ld      c,#C_WRITE
        call    BDOS
        pop     hl
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ret

