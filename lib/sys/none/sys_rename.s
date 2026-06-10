        ;; sys_rename.s  (sys backend: none / bare metal)
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.

        .module sys_rename
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_rename
        .globl  __sys_none_rename

        .area   _CODE

__sys_rename::
        jp      __sys_none_rename
