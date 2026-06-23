        ;; getchar.s  (sys backend: none — template)
        ;;
        ;; int getchar(void) — read one character from the console.
        ;;   returns DE = character, or 0xFFFF (-1) at end of input.
        ;;
        ;; Console input goes here, NOT through read() (which is disk-only).

        .module getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _getchar

        .area   _CODE
_getchar::
        ;; TODO: read a byte from your console; return it in DE, 0xFFFF = EOF.
        ld      de,#0xffff
        ret
