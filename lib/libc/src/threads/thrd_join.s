        .module thrd_join
        .optsdcc -mz80 sdcccall(1)
        .globl  _thrd_join
        .globl  __threads_thrd_join_core
        .area   _CODE
_thrd_join::
        jp      __threads_thrd_join_core
