        ;; cpm3_tmp_rec.s
        ;; Split from cpm3_fd_slot_ptr.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_tmp_rec
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_tmp_rec

        .area   _DATA
__cpm3_tmp_rec::
        .db     0, 0, 0, 0
