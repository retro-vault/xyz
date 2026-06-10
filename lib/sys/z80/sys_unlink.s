        ;; sys_unlink.s  (sys backend: generic z80)
        ;;
        ;; Generic bare-metal backend has no filesystem remove hook.

        .module sys_unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_unlink

        .area   _CODE

__sys_unlink::
        ld      de,#0xffff
        ret
