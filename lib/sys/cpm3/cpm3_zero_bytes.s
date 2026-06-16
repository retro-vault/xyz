        ;; cpm3_zero_bytes.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_zero_bytes
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_zero_bytes

        .area   _CODE
__cpm3_zero_bytes::
        xor     a
__cpm3_zero_bytes_loop:
        ld      (hl),a
        inc     hl
        djnz    __cpm3_zero_bytes_loop
        ret

        ;; HL = ptr, B = byte count. Fill with spaces.
