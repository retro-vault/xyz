        ;; lseek.s  (sys backend: generic z80)
        ;;
        ;; Generic bare-metal backend has no filesystem. Seeks fail.

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek

        .area   _CODE

_lseek::
        ld      de,#0xffff
        ld      hl,#0xffff
        ret
