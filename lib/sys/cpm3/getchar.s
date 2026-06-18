        ;; getchar.s  (sys backend: CP/M 3)
        ;;
        ;; int getchar(void) — read one character from the console.
        ;; Returns the character (in DE), or -1 (0xFFFF) at end of input (^Z).

        .module getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _getchar

        .equ    BDOS,5
        .equ    C_READ,1

        .area   _CODE
_getchar::
        call    __cpm3_console_getchar
        cp      #0x1a                   ; ^Z = end of input
        jr      z,__cpm3_getchar_eof
        ld      e,a
        ld      d,#0x00
        ret
__cpm3_getchar_eof:
        ld      de,#0xffff
        ret

__cpm3_console_getchar:
        push    ix
        push    iy
        push    bc
        push    de
        push    hl
        ld      c,#C_READ
        call    BDOS
        pop     hl
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ret
