        ;; putchar.s  (sys backend: none — template)
        ;;
        ;; int putchar(int c) — write one character to the console.
        ;;   HL = c        (sdcccall(1))   returns DE = c
        ;;
        ;; This is the FIRST hook to implement: it makes puts()/printf() visible.
        ;; Console output goes here, NOT through write() (which is disk-only).

        .module putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _putchar

        .area   _CODE
_putchar::
        ;; TODO: output the byte in L to your console device, e.g. out (PORT),l
        ld      e,l                     ; return the character
        ld      d,#0
        ret
