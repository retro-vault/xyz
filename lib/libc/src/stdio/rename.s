        ;; rename.s
        ;;
        ;; Rename a file through the platform rename hook.

        .module rename
        .optsdcc -mz80 sdcccall(1)

        .globl  _rename
        .globl  __sys_rename

        .area   _CODE

_rename::
        call    __sys_rename
        ex      de,hl
        push    hl
        pop     de
        ret
