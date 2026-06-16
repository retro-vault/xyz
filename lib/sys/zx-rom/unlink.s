        ;; unlink.s  (sys backend: zx-rom)
        ;;
        ;; ROM target has no generic filesystem remove hook yet.

        .module unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  _unlink

        .area   _CODE

_unlink::
        ld      de,#0xffff
        ret
