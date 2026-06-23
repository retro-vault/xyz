        .module thrd_yield
        .optsdcc -mz80 sdcccall(1)
        .globl  _thrd_yield
        .globl  __threads_thrd_yield_core
        .area   _CODE
_thrd_yield::
        jp      __threads_thrd_yield_core
