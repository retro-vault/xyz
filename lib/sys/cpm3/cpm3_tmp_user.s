        ;; cpm3_tmp_user.s
        ;; Split from cpm3_fill_spaces.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_tmp_user
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_tmp_user

        .area   _DATA
__cpm3_tmp_user::
        .db     0
