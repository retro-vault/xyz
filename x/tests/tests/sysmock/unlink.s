        ;; unlink.s  (sys backend: sim (in-RAM simulator))
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.

        .module unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  _unlink
        .globl  __sys_none_unlink

        .area   _CODE

_unlink::
        jp      __sys_none_unlink
