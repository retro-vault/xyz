        ;; cpm3_prepare_record_iy.s
        ;; Split from cpm3_cmp_fpos_size_iy.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_prepare_record_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_prepare_record_iy
        .globl  __cpm3_shift7_from_ptr
        .globl  __cpm3_tmp_rec

FCB_OFF_RREC0   .equ 33
FCB_OFF_RREC1   .equ 34
FCB_OFF_RREC2   .equ 35
FD_OFF_BUFREC0  .equ 13
FD_OFF_FCB      .equ 17
FD_OFF_FPOS0    .equ 5

        .area   _CODE
__cpm3_prepare_record_iy::
        push    iy
        pop     hl
        ld      de,#FD_OFF_FPOS0
        add     hl,de
        call    __cpm3_shift7_from_ptr
        ld      a,(__cpm3_tmp_rec)
        ld      FD_OFF_BUFREC0(iy),a
        ld      FD_OFF_FCB + FCB_OFF_RREC0(iy),a
        ld      a,(__cpm3_tmp_rec + 1)
        ld      FD_OFF_BUFREC0 + 1(iy),a
        ld      FD_OFF_FCB + FCB_OFF_RREC1(iy),a
        ld      a,(__cpm3_tmp_rec + 2)
        ld      FD_OFF_BUFREC0 + 2(iy),a
        ld      FD_OFF_FCB + FCB_OFF_RREC2(iy),a
        ret

        ;; IY = current entry. Load the current file record into DMA for reading.
        ;; Returns A = 0 on success.
