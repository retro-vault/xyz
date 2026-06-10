        ;; sys_unlink.s  (sys backend: zx-rom)
        ;;
        ;; ROM target has no generic filesystem remove hook yet.

        .module sys_unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_unlink

        .area   _CODE

__sys_unlink::
        ld      de,#0xffff
        ret
