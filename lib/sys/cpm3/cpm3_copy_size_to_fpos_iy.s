        ;; cpm3_copy_size_to_fpos_iy.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_copy_size_to_fpos_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_copy_size_to_fpos_iy

FD_OFF_FPOS0    .equ 5
FD_OFF_FSIZE0   .equ 9

        .area   _CODE
__cpm3_copy_size_to_fpos_iy::
        ld      a,FD_OFF_FSIZE0(iy)
        ld      FD_OFF_FPOS0(iy),a
        ld      a,FD_OFF_FSIZE0 + 1(iy)
        ld      FD_OFF_FPOS0 + 1(iy),a
        ld      a,FD_OFF_FSIZE0 + 2(iy)
        ld      FD_OFF_FPOS0 + 2(iy),a
        ld      a,FD_OFF_FSIZE0 + 3(iy)
        ld      FD_OFF_FPOS0 + 3(iy),a
        ret

        ;; IY = current entry. size = fpos when fpos grew past EOF.
