        ;; remove.s
        ;;
        ;; Remove a named file through the platform unlink hook. The current
        ;; stdio layer keeps this separate from fclose(); the backend decides
        ;; whether open files may be removed.

        .module remove
        .optsdcc -mz80 sdcccall(1)

        .globl  _remove
        .globl  __sys_unlink

        .area   _CODE

_remove::
        call    __sys_unlink
        ex      de,hl
        push    hl
        pop     de
        ret
