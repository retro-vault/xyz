        ;; cpm3_cmp_fpos_size_iy.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).




        .module cpm3_cmp_fpos_size_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_cmp_fpos_size_iy

FD_OFF_FPOS0    .equ 5
FD_OFF_FSIZE0   .equ 9

        .area   _CODE
__cpm3_cmp_fpos_size_iy::
        ld      a,FD_OFF_FPOS0 + 3(iy)
        cp      FD_OFF_FSIZE0 + 3(iy)
        jr      c,__cpm3_cmp_ret
        ret     nz
        ld      a,FD_OFF_FPOS0 + 2(iy)
        cp      FD_OFF_FSIZE0 + 2(iy)
        jr      c,__cpm3_cmp_ret
        ret     nz
        ld      a,FD_OFF_FPOS0 + 1(iy)
        cp      FD_OFF_FSIZE0 + 1(iy)
        jr      c,__cpm3_cmp_ret
        ret     nz
        ld      a,FD_OFF_FPOS0(iy)
        cp      FD_OFF_FSIZE0(iy)
__cpm3_cmp_ret:
        ret

        ;; IY = current entry. fpos = 0.
