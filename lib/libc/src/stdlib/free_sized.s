        ;; free_sized.s
        ;; Split from heap_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module free_sized
        .optsdcc -mz80 sdcccall(1)

        .globl  _free_sized
        .globl  _free

        .area   _CODE
_free_sized::
        ld      a,h
        or      l
        ret     z
        jp      _free

