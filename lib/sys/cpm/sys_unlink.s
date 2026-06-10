        ;; sys_unlink.s  (sys backend: cpm)
        ;;
        ;; CP/M remove translation is not wired into the Unix-like fd layer yet.

        .module sys_unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_unlink

        .area   _CODE

__sys_unlink::
        ld      de,#0xffff
        ret
