        ;; open.s  (sys backend: generic z80)
        ;;
        ;; Generic bare-metal backend has no filesystem. File opens fail.

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open

        .area   _CODE

_open::
        ld      de,#0xffff
        ret
