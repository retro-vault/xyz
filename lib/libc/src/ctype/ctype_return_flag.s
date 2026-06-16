        ;; ctype_return_flag.s
        ;; Split from ctype_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ctype_return_flag
        .optsdcc -mz80 sdcccall(1)

        .globl  __ctype_return_flag

        .area   _CODE
__ctype_return_flag::
        ld      de,#0x0000
        ret     nz
        inc     e
        ret

        ;; __ctype_return_hl
        ;; Return an unchanged promoted int through DE without rebuilding it bytewise.
