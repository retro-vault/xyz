        ;; cpm3_copy_bytes.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_copy_bytes
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_copy_bytes

        .area   _CODE
__cpm3_copy_bytes::
        ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     de
        djnz    __cpm3_copy_bytes
        ret

        ;; Upper-case ASCII in A.
