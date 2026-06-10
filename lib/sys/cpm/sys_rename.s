        ;; sys_rename.s  (sys backend: cpm)
        ;;
        ;; CP/M rename translation is not wired into the Unix-like fd layer yet.

        .module sys_rename
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_rename

        .area   _CODE

__sys_rename::
        ld      de,#0xffff
        ret
