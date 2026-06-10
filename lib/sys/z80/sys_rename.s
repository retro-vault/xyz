        ;; sys_rename.s  (sys backend: generic z80)
        ;;
        ;; Generic bare-metal backend has no filesystem rename hook.

        .module sys_rename
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_rename

        .area   _CODE

__sys_rename::
        ld      de,#0xffff
        ret
