        ;; putchar.s  (sys backend: CP/M 3)
        ;;
        ;; int putchar(int c) — write one character to the console.
        ;; '\n' is expanded to CR/LF.  Returns the character (in DE).

        .module putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _putchar
        .globl  __cpm3_console_putchar

        .area   _CODE
_putchar::
        ld      a,l
        cp      #'\n'
        jr      nz,__cpm3_putchar_emit
        ld      e,#'\r'
        call    __cpm3_console_putchar
        ld      e,#'\n'
        call    __cpm3_console_putchar
        ld      e,#'\n'
        ld      d,#0x00
        ret
__cpm3_putchar_emit:
        ld      e,a
        call    __cpm3_console_putchar
        ld      d,#0x00
        ret
