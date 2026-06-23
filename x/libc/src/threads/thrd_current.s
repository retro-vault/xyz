        .module thrd_current
        .optsdcc -mz80 sdcccall(1)
        .globl  _thrd_current
        .globl  __threads_thrd_current_core
        .area   _CODE
_thrd_current::
        jp      __threads_thrd_current_core
