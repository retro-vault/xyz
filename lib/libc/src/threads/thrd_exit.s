        .module thrd_exit
        .optsdcc -mz80 sdcccall(1)
        .globl  _thrd_exit
        .globl  __threads_thrd_exit_core
        .area   _CODE
_thrd_exit::
        jp      __threads_thrd_exit_core
