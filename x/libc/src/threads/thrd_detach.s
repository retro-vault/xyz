        .module thrd_detach
        .optsdcc -mz80 sdcccall(1)
        .globl  _thrd_detach
        .globl  __threads_thrd_detach_core
        .area   _CODE
_thrd_detach::
        jp      __threads_thrd_detach_core
