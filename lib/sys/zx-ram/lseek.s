        ;; lseek.s  (sys backend: zx-ram)
        ;;
        ;; RAM target has no generic filesystem hook yet. Seeks fail.

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek

        .area   _CODE

_lseek::
        ld      de,#0xffff
        ld      hl,#0xffff
        ret
