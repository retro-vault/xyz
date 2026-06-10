        ;; open.s  (sys backend: zx-rom)
        ;;
        ;; ROM target has no generic filesystem hook yet. File opens fail.

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open

        .area   _CODE

_open::
        ld      de,#0xffff
        ret
