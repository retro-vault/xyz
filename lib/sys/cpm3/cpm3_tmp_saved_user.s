        ;; cpm3_tmp_saved_user.s
        ;; Split from cpm3_get_current_user.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_tmp_saved_user
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_tmp_saved_user

        .area   _DATA
__cpm3_tmp_saved_user::
        .db     0
