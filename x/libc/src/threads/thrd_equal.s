        .module thrd_equal
        .optsdcc -mz80 sdcccall(1)
        .globl  _thrd_equal
        .globl  __threads_thrd_equal_core
        .area   _CODE
_thrd_equal::
        jp      __threads_thrd_equal_core
