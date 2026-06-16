        ;; threads_cnd_destroy_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_cnd_destroy_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_cnd_destroy_core

        .area   _CODE
__threads_cnd_destroy_core:
        ld      a,h
        or      l
        ret     z
        ld      (hl),#0
        ret

