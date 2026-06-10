        ;; close.s  (sys backend: zx-rom)
        ;;
        ;; ROM target has no generic filesystem hook yet. File closes fail.

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close

        .area   _CODE

_close::
        ld      de,#0xffff
        ret
