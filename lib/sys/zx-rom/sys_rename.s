        ;; sys_rename.s  (sys backend: zx-rom)
        ;;
        ;; ROM target has no generic filesystem rename hook yet.

        .module sys_rename
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_rename

        .area   _CODE

__sys_rename::
        ld      de,#0xffff
        ret
