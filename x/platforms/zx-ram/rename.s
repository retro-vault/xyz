        ;; rename.s  (sys backend: zx-ram)
        ;;
        ;; RAM target has no generic filesystem rename hook yet.

        .module rename
        .optsdcc -mz80 sdcccall(1)

        .globl  _rename

        .area   _CODE

_rename::
        ld      de,#0xffff
        ret
