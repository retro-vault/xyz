        .module thrd_sleep
        .optsdcc -mz80 sdcccall(1)
        .globl  _thrd_sleep
        .globl  __threads_thrd_sleep_core
        .area   _CODE
_thrd_sleep::
        jp      __threads_thrd_sleep_core
