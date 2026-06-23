        ;; rename.s  (sys backend: sim (in-RAM simulator))
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.

        .module rename
        .optsdcc -mz80 sdcccall(1)

        .globl  _rename
        .globl  __sys_none_rename

        .area   _CODE

_rename::
        jp      __sys_none_rename
