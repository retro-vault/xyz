        ;; sys_unlink.s  (sys backend: none / bare metal)
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.

        .module sys_unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_unlink
        .globl  __sys_none_unlink

        .area   _CODE

__sys_unlink::
        jp      __sys_none_unlink
