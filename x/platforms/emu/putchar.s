        ;; putchar.s  (sys backend: emu)

        .module putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _putchar

        .equ    EMU_PORT_CONOUT,0xe1

        .area   _CODE
_putchar::
        ld      a,l
        out     (EMU_PORT_CONOUT),a
        ld      e,l
        ld      d,#0
        ret
