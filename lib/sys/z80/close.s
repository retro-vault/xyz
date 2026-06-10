        ;; close.s  (sys backend: generic z80)
        ;;
        ;; Generic bare-metal backend has no filesystem. File closes fail.

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close

        .area   _CODE

_close::
        ld      de,#0xffff
        ret
