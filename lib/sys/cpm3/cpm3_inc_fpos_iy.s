        ;; cpm3_inc_fpos_iy.s
        ;; Split from cpm3_cmp_fpos_size_iy.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_inc_fpos_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_inc_fpos_iy

FD_OFF_FPOS0    .equ 5

        .area   _CODE
__cpm3_inc_fpos_iy::
        ld      a,FD_OFF_FPOS0(iy)
        inc     a
        ld      FD_OFF_FPOS0(iy),a
        ret     nz
        ld      a,FD_OFF_FPOS0 + 1(iy)
        inc     a
        ld      FD_OFF_FPOS0 + 1(iy),a
        ret     nz
        ld      a,FD_OFF_FPOS0 + 2(iy)
        inc     a
        ld      FD_OFF_FPOS0 + 2(iy),a
        ret     nz
        ld      a,FD_OFF_FPOS0 + 3(iy)
        inc     a
        ld      FD_OFF_FPOS0 + 3(iy),a
        ret

        ;; HL = pointer to a 32-bit little-endian value. Return A = low 7-bit
        ;; remainder, and place value>>7 into __cpm3_tmp_rec.
