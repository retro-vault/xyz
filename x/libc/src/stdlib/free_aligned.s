        ;; free_aligned.s
        ;; Split from heap_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module free_aligned
        .optsdcc -mz80 sdcccall(1)

        .globl  _free_aligned
        .globl  _free

        .area   _CODE
_free_aligned::
        ld      a,h
        or      l
        ret     z
        jp      _free
