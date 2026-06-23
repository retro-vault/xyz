        ;; ffs_scan_byte.s
        ;; Split from ffs.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ffs_scan_byte
        .optsdcc -mz80 sdcccall(1)

        .globl  __ffs_scan_byte

        .area   _CODE
__ffs_scan_byte::
        ld      b,#1
ffs_scan_byte_loop:
        rra
        jr      c,ffs_scan_byte_done
        inc     b
        jr      ffs_scan_byte_loop
ffs_scan_byte_done:
        ret
