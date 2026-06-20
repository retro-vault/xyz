        ;; getchar.s  (sys backend: emu)

        .module getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _getchar

        .equ    EMU_PORT_CONIN_STATUS,0xe2
        .equ    EMU_PORT_CONIN_DATA,0xe3

        .area   _CODE
_getchar::
        in      a,(EMU_PORT_CONIN_STATUS)
        or      a
        jr      z,.eof
        in      a,(EMU_PORT_CONIN_DATA)
        ld      e,a
        ld      d,#0
        ret
.eof:
        ld      de,#0xffff
        ret
